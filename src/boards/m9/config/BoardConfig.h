#pragma once

// Elecrow ThinkNode M9: ESP32-S3R8, LR1110, ST7789, keyboard and d-pad.

#define DEVICE_NAME              "rsM9"
#define DEVICE_AP_PREFIX         "rsm9"
#define BOARD_MODEL_NAME         "Elecrow ThinkNode M9"
#define BOARD_RADIO_NAME         "LR1110"
#define BOARD_CONFIRM_INPUT_NAME "center key"
#define BOARD_DEFAULT_BRIGHTNESS 100

#define RSDECK_VERSION_MAJOR  2
#define RSDECK_VERSION_MINOR  1
#define RSDECK_VERSION_PATCH  0
#define RSDECK_VERSION_STRING "2.1.0"
#define BOARD_RELEASE_REPO    "ratspeak/ratspeak-handheld"
#define HAS_CONTACT_RENAME    true

#define HAS_DISPLAY       true
#define HAS_KEYBOARD      true
#define HAS_TOUCH         false
#define HAS_TRACKBALL     false
#define HAS_SCROLLWHEEL   false
#define HAS_LORA          true
#define HAS_WIFI          true
#define HAS_SD            true
#define HAS_AUDIO         true
#define HAS_GPS           true
#define HAS_BATTERY_MODEL true

#define NVS_NS_IDENTITY      "rsm9_id"
#define NVS_NS_MSG           "rsm9_msg"
#define SD_PATH_ROOT         "/rsm9"
#define SD_PATH_CONFIG_DIR   "/rsm9/config"
#define SD_PATH_USER_CONFIG  "/rsm9/config/user.json"
#define SD_PATH_MESSAGES     "/rsm9/messages"
#define SD_PATH_CONTACTS     "/rsm9/contacts"
#define SD_PATH_IDENTITY_DIR "/rsm9/identity"
#define SD_PATH_IDENTITY     "/rsm9/identity/identity.key"
#define SD_PATH_IMPORT_IDENTITY "/rsm9/identity/import.identity"
#define SD_PATH_IMPORT_ID    "/rsm9/identity/import.key"
#define SD_PATH_TRANSPORT    "/rsm9/transport"

// Active-low peripheral and backlight rails.
#define BOARD_POWER_PIN      18
#define BOARD_POWER_ACTIVE   LOW

// LR1110 and the display share FSPI.
#define LORA_CS              39
#define LORA_IRQ             42
#define LORA_RST             45
#define LORA_BUSY            41
#define LORA_RXEN            -1
#define LORA_TXEN            -1
#define LORA_TCXO_VOLTAGE    3.3f
#define LORA_USE_DCDC_REGULATOR false
#define LORA_DEFAULT_FREQ     915000000
#define LORA_DEFAULT_BW       250000
#define LORA_DEFAULT_SF       11
#define LORA_DEFAULT_CR       5
#define LORA_DEFAULT_TX_POWER 22
#define LORA_DEFAULT_PREAMBLE 18

#define SPI_SCK              40
#define SPI_MISO             38
#define SPI_MOSI             47

// ST7789, native 240x320 portrait; the UI uses rotation 1 (320x240).
#define TFT_CS               16
#define TFT_DC               15
#define TFT_RST              14
#define TFT_BL               17
#define TFT_WIDTH            320
#define TFT_HEIGHT           240
#define TFT_SPI_FREQ         27000000

// Sensor/RTC bus and dedicated keyboard-controller bus.
#define I2C_SDA               7
#define I2C_SCL               6
#define KB_I2C_SDA           20
#define KB_I2C_SCL           21
#define KB_I2C_ADDR          0x6C

#define SD_CS                48

#define GPS_TX                2
#define GPS_RX                3
#define GPS_EN               11
#define GPS_RST               5
#define GPS_BAUD         115200

#define BAT_ADC_PIN          13
#define BUZZER_PIN            9

#define MAX_PACKET_SIZE     255
#define SPI_FREQUENCY   8000000
