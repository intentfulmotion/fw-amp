#include <hal/amp-1.0.0/amp-storage.h>

const char* AmpStorage::storage = "amp";

esp_vfs_spiffs_conf_t AmpStorage::conf = {
  .base_path = "/spiffs",
  .partition_label = NULL,
  .max_files = 5,
  .format_if_mount_failed = true
};

bool AmpStorage::init() {
  // NVS
  auto err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }

  // SPIFFS
  err = esp_vfs_spiffs_register(&conf);
  return err == ESP_OK;
}

void AmpStorage::deinit() {
  esp_vfs_spiffs_unregister(conf.partition_label);
}

std::string AmpStorage::getHardwareRevision() {
  uint32_t hardwareRevisionBytes = __builtin_bswap32(REG_READ(EFUSE_BLK3_RDATA0_REG));
  uint8_t HARDWARE_REVISION_MAJOR = (hardwareRevisionBytes >> (8*3)) & 0xff;
  uint8_t HARDWARE_REVISION_MINOR = (hardwareRevisionBytes >> (8*2)) & 0xff;
  uint8_t HARDWARE_REVISION_REVISION = (hardwareRevisionBytes >> (8*1)) & 0xff;

  char hardwareRevisionBuffer[12];
  sprintf(hardwareRevisionBuffer, "%d.%d.%d", HARDWARE_REVISION_MAJOR, HARDWARE_REVISION_MINOR, HARDWARE_REVISION_REVISION);
  return std::string(hardwareRevisionBuffer);
}

std::string AmpStorage::getSerialNumber() {
  uint32_t hardwareRevisionBytes = __builtin_bswap32(REG_READ(EFUSE_BLK3_RDATA0_REG));
  uint32_t serialNumberBytes = __builtin_bswap32(REG_READ(EFUSE_BLK3_RDATA1_REG));
  uint8_t SERIAL_NUMBER_REV = (hardwareRevisionBytes) & 0xff;
  uint8_t SERIAL_NUMBER_WEEK = (serialNumberBytes >> (8*2)) & 0xff;
  uint8_t SERIAL_NUMBER_YEAR = (serialNumberBytes >> (8*3)) & 0xff;
  uint16_t SERIAL_NUMBER_UNIQUE = serialNumberBytes;

  char serialNumberBuffer[16];
  sprintf(serialNumberBuffer, "A%02x-%02d%02d-%04x", SERIAL_NUMBER_REV, SERIAL_NUMBER_YEAR, SERIAL_NUMBER_WEEK, SERIAL_NUMBER_UNIQUE);
  return std::string(serialNumberBuffer);
}

std::string AmpStorage::getDefaultName() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(&mac[0]);

  return string_format("Amp-%02X%02X", mac[4], mac[5]);
}

bool AmpStorage::fileExists(std::string filename) {
  struct stat st;
  int result = stat(filename.c_str(), &st);
  return result == 0;
}

FILE* AmpStorage::openFile(std::string filename, std::string attributes) {
  return fopen(filename.c_str(), attributes.c_str());
}

std::string AmpStorage::readFile(std::string filename) {
  FILE* file = fopen(filename.c_str(), "r");

  if (file == NULL) {
    ESP_LOGE(STORAGE_TAG, "Could not open file: %s", filename.c_str());
    return "";
  }

  struct stat st;
  stat(filename.c_str(), &st);
  auto size = st.st_size;

  char* buffer[size + 1];
  fread((uint8_t*)&buffer, 1, size, file);
  std::string data((char*)buffer, size + 1);

  return data;
}

FILE* AmpStorage::writeFile(std::string filename) {
  struct stat st;
  if (stat(filename.c_str(), &st) == 0)
    unlink(filename.c_str());

  return fopen(filename.c_str(), "w+");
}

void AmpStorage::saveDeviceName(std::string name) {
  saveString("name", name);
}

std::string AmpStorage::getDeviceName() {
  std::string name = getString("name");

  if (name.length() == 0)
    name = getDefaultName();
  
  return name;
}

float AmpStorage::getTurnZero() {
  return getFloat("turnZero");
}

void AmpStorage::saveTurnZero(float value) {
  saveFloat("turnZero", value);
}

void AmpStorage::saveAccelBias(Vector3D *bias) {
  saveFloat("accel-bias-x", bias->x);
  saveFloat("accel-bias-y", bias->y);
  saveFloat("accel-bias-z", bias->z);  
}

void AmpStorage::getAccelBias(Vector3D *bias) {
  bias->x = getFloat("accel-bias-x", 0.00f);
  bias->y = getFloat("accel-bias-y", 0.00f);
  bias->z = getFloat("accel-bias-z", 0.00f);
}

float AmpStorage::getFloat(const char *key, float defaultValue) {
  nvs_handle handle;

  union {
    float decimal;
    uint32_t raw;
  } converter;

  // open NVS
  auto err = nvs_open(storage, NVS_READWRITE, &handle);

  // read as uint32_t
  err = nvs_get_u32(handle, key, &converter.raw);

  nvs_close(handle);

  if (err != ESP_OK) {
    ESP_LOGW(STORAGE_TAG, "Unable to get %s from NVS", key);
    return defaultValue;
  }

  return converter.decimal;
}

void AmpStorage::saveFloat(const char* key, float value) {
  nvs_handle handle;

  union {
    float decimal;
    uint32_t raw;
  } converter;

  converter.decimal = value;

  // open NVS
  auto err = nvs_open(storage, NVS_READWRITE, &handle);

  // set uint32_t cast of value
  err = nvs_set_u32(handle, key, converter.raw);

  // commit changes
  err = nvs_commit(handle);

  nvs_close(handle);
  
  if (err != ESP_OK)
    ESP_LOGW(STORAGE_TAG, "Unable to save %s to NVS", key);
}

void AmpStorage::saveString(std::string key, std::string value) {
  nvs_handle handle;
  
  auto err = nvs_open(storage, NVS_READWRITE, &handle);
  err = nvs_set_str(handle, key.c_str(), value.c_str());

  err = nvs_commit(handle);

  nvs_close(handle);

  if (err != ESP_OK)
    ESP_LOGW(STORAGE_TAG, "Unable to save %s:%s to NVS", key.c_str(), value.c_str());
}

std::string AmpStorage::getString(std::string key) {
  nvs_handle handle;

  // open NVS
  auto err = nvs_open(storage, NVS_READWRITE, &handle);

  // read as string
  size_t size;
  err = nvs_get_str(handle, key.c_str(), NULL, &size);

  char* valueRaw = (char*) malloc(size);
  err = nvs_get_str(handle, key.c_str(), valueRaw, &size);

  nvs_close(handle);

  if (err != ESP_OK) {
    ESP_LOGW(STORAGE_TAG, "Unable to get %s from NVS", key.c_str());
    return "";
  }

  return std::string(valueRaw);
}