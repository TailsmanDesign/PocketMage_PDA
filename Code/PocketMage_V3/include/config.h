#ifndef CONFIG_H
#define CONFIG_H

// OTA App
// Is this an OTA APP? // OTA_APP: set -DOTA_APP_FLAG=1 in platformio.ini
#if OTA_APP_FLAG
#define OTA_APP true
#else
#define OTA_APP false
#endif

// CONFIGURATION & SETTINGS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|
#define KB_COOLDOWN 50                          // Keypress cooldown
#define FULL_REFRESH_AFTER 5                    // Full refresh after N partial refreshes (CHANGE WITH CAUTION)
#define MAX_FILES 10                            // Number of files to store
#define FORMAT_SPIFFS_IF_FAILED true            // Format the SPIFFS filesystem if mount fails
#define SLEEPMODE "TEXT"                        // TEXT, SPLASH, CLOCK
#define TXT_APP_STYLE 1                         // 0: Old Style (NOT SUPPORTED), 1: New Style
#define SET_CLOCK_ON_UPLOAD false               // Should system clock be set automatically on code upload?
#define TOUCH_TIMEOUT_MS 1200                   // Delay after scrolling to return to typing mode (ms)
#define SYS_METADATA_FILE "/sys/SDMMC_META.txt" // File path to the file system metadata file
#define POWER_SAVE_FREQ 40                      // CPU freq for power save mode
#define IDLE_TIME 20000                         // time to wait for mage idle (ms)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|

// PIN DEFINITION

#define I2C_SCL       35
#define I2C_SDA       36
#define MPR121_ADDR   0x5A
#define MP2722_ADDR   0x3F
#define USB_MUX_PIN   7

#define KB_IRQ        8
//#define PWR_BTN       38  // V3.0
#define PWR_BTN       0     // V3.2
#define BAT_SENS      4
#define CHRG_SENS     39
#define RTC_INT       1 

// SPI
#define SPI_MOSI      14
#define SPI_SCK       15

// OLED
#define OLED_CS       47
#define OLED_DC       46
#define OLED_RST      45

// E Ink
#define EPD_CS        2
#define EPD_DC        21
#define EPD_RST       9
#define EPD_BUSY      37

// SDMMC mode
#define SD_CLK        12
#define SD_CMD        11
#define SD_D0         13

// SDSPI mode
#define SD_CS         10
#define SD_MOSI       11
#define SD_SCK        12
#define SD_MISO       13

#define BZ_PIN        17

// ===================== SYSTEM SETTINGS =====================
// Persistent preferences
extern int TIMEOUT;              // Auto sleep timeout (seconds)
extern bool DEBUG_VERBOSE;       // Extra debug output
extern bool SYSTEM_CLOCK;        // Show clock on screen
extern bool SHOW_YEAR;           // Show year in clock
extern bool SAVE_POWER;          // Enable power saving mode
extern bool ALLOW_NO_MICROSD;    // Allow running without SD card
extern bool HOME_ON_BOOT;        // Start home app on boot
extern int OLED_BRIGHTNESS;      // OLED brightness (0-255)
extern int OLED_MAX_FPS;         // OLED max FPS
extern bool SD_SPI_COMPATIBILITY;

#endif