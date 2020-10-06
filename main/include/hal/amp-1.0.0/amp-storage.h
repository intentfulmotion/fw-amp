#pragma once
#include <common.h>
#include <soc/efuse_reg.h>
#include <esp_efuse.h>
#include "esp_vfs.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_spiffs.h>
#include <models/motion.h>

static const char* STORAGE_TAG = "storage";

class AmpStorage {
  static const char* storage;
  static esp_vfs_spiffs_conf_t conf;

  public:
    bool init();
    void deinit();

    static std::string getHardwareRevision();
    static std::string getSerialNumber();
    static std::string getDefaultName();

    static void saveDeviceName(std::string name);
    static std::string getDeviceName();

    FILE* openFile(std::string filename, std::string attributes = "r");
    std::string readFile(std::string filename);
    FILE* writeFile(std::string filename);

    static float getTurnZero();
    static void saveTurnZero(float value);

    static void saveAccelBias(Vector3D *bias);
    static void getAccelBias(Vector3D *bias);

    static void saveFloat(const char* key, float value);
    static float getFloat(const char* key, float defaultValue = 0);

    static void saveString(std::string key, std::string value);
    static std::string getString(std::string key);
};