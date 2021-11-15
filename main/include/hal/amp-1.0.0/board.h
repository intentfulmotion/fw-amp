// Status LED
#define STATUS_LED          13
#define STATUS_ENABLED      true

// I/O Channels
#define STRIP_ONE_DATA      22
#define STRIP_ONE_CLK       21
#define STRIP_TWO_DATA      33
#define STRIP_TWO_CLK       25
#define STRIP_THREE_DATA    26
#define STRIP_THREE_CLK     27
#define STRIP_FOUR_DATA     14
#define STRIP_FOUR_CLK      12

// Input
#define BUTTON_INPUT        GPIO_NUM_36

// Power Management
#define POWER_HOLD          GPIO_NUM_32
#define VBAT_SENSE          GPIO_NUM_39
#define BAT_CHRG            GPIO_NUM_34
#define BAT_DONE            GPIO_NUM_35

// IMU
#define IMU_CLK             GPIO_NUM_18
#define IMU_MISO            GPIO_NUM_19
#define IMU_MOSI            GPIO_NUM_23
#define IMU_CS              GPIO_NUM_5

#define BLE_ENABLED
#define HAS_BUTTON
#define HAS_INTERNAL_IMU
#define MANAGES_INTERNAL_POWER