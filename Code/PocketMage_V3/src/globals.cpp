#include "globals.h"
#include "sdmmc_cmd.h"

// ===================== SPI BUSSES =====================
SPIClass *vspi = NULL;
SPIClass *hspi = NULL;
fs::FS* global_fs = nullptr;

// ===================== SYSTEM STATE =====================
Preferences prefs;                       // NVS preferences // note add power button logic in app + prefs to immediate sleep 
int OLEDFPSMillis = 0;                   // Last OLED FPS update time
int KBBounceMillis = 0;                  // Last keyboard debounce time
volatile bool newState = false;          // App state changed
volatile bool disableTimeout = OTA_APP ? true: false;    // Disable timeout globally, OTA_APP: disable timeout by default
bool fileLoaded = false;    
unsigned int flashMillis = 0;            // Flash timing

String OTA1_APP;
String OTA2_APP;
String OTA3_APP;
String OTA4_APP;

// ===================== SYSTEM CONFIG =====================
int TIMEOUT;                             // Auto sleep timeout (seconds)
bool DEBUG_VERBOSE;                      // Extra debug output
bool SYSTEM_CLOCK;                       // Show clock on screen
bool SHOW_YEAR;                          // Show year in clock
bool SAVE_POWER;                         // Enable power saving mode
bool ALLOW_NO_MICROSD;                   // Allow running without SD card
bool HOME_ON_BOOT;                       // Start home app on boot
int OLED_BRIGHTNESS;                     // OLED brightness (0-255)
int OLED_MAX_FPS;                        // OLED max FPS
bool SD_SPI_COMPATIBILITY;               // SD card compatibility mode

// ===================== APP STATES =====================
const String appStateNames[] = { "txt", "filewiz", "usb", "bt", "settings", "tasks", "calendar", "journal", "lexicon", "terminal" , "loader" }; // App state names
#if !OTA_APP // POCKETMAGE_OS
const unsigned char *appIcons[11] = { _homeIcons2, _homeIcons3, _homeIcons4, _homeIcons5, _homeIcons6, taskIconTasks0, _homeIcons7, _homeIcons8, _homeIcons9, _homeIcons11, _homeIcons10}; // App icons
#endif
AppState CurrentAppState;                // Current app state

// ===================== TASKS APP =====================
std::vector<std::vector<String>> tasks;  // Task list

// ===================== HOME APP =====================
HOMEState CurrentHOMEState = HOME_HOME;  // Current home state