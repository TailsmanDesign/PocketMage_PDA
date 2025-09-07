#include "globals.h"
#include "sdmmc_cmd.h"

// ===================== DISPLAY =====================
// Main e-ink display object
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
// Fast full update flag for e-ink
volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;
// 256x32 SPI OLED display object
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, OLED_CS, OLED_DC, OLED_RST);

// ===================== INPUT DEVICES =====================

// Matrix keypad controller
Adafruit_TCA8418 keypad;
// Key layouts
/* migrated to pocketmage_kb.h
char keysArray[4][10] = {
    { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' },
    { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',   8 },  // 8:BKSP
    {   9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.',  13 },  // 9:TAB, 13:CR
    {   0,  17,  18, ' ', ' ', ' ',  19,  20,  21,   0 }   // 17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};
char keysArraySHFT[4][10] = {
    { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' },
    { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',   8 },
    {   9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',',  13 },
    {   0,  17,  18, ' ', ' ', ' ',  28,  29,  30,   0 }
};
char keysArrayFN[4][10] = {
    { '1', '2', '3', '4', '5', '6', '7',  '8',  '9', '0' },
    { '#', '!', '$', ':', ';', '(', ')', '\'', '\"',   8 },
    {  14, '%', '_', '&', '+', '-', '/',  '?',  ',',  13 },
    {   0,  17,  18, ' ', ' ', ' ',  12,    7,    6,   0 }
};
*/
// Capacitive touch slider
Adafruit_MPR121 cap = Adafruit_MPR121();
volatile long int dynamicScroll = 0;         // Dynamic scroll offset
volatile long int prev_dynamicScroll = 0;    // Previous scroll offset
int lastTouch = -1;                          // Last touch event
unsigned long lastTouchTime = 0;             // Last touch time

// ===================== AUDIO =====================
// Buzzer for sound feedback
Buzzer buzzer(17);

// ===================== RTC =====================
// Real-time clock chip
RTC_PCF8563 rtc;
// Day names
const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// ===================== USB & STORAGE =====================
// USB mass storage controller
USBMSC msc;
bool mscEnabled = false;          // Is USB MSC active?
sdmmc_card_t* card = nullptr;     // SD card pointer

// ===================== SYSTEM SETTINGS =====================
// Persistent preferences (NVS)
Preferences prefs;
int TIMEOUT;              // Auto sleep timeout (seconds)
bool DEBUG_VERBOSE;       // Extra debug output
bool SYSTEM_CLOCK;        // Show clock on screen
bool SHOW_YEAR;           // Show year in clock
bool SAVE_POWER;          // Enable power saving mode
bool ALLOW_NO_MICROSD;    // Allow running without SD card
bool HOME_ON_BOOT;        // Start home app on boot
int OLED_BRIGHTNESS;      // OLED brightness (0-255)
int OLED_MAX_FPS;         // OLED max FPS

String OTA1_APP;
String OTA2_APP;
String OTA3_APP;
String OTA4_APP;

// ===================== SYSTEM STATE =====================
// E-Ink refresh control
// volatile int einkRefresh = FULL_REFRESH_AFTER; // Partial/full refresh counter
int OLEDFPSMillis = 0;            // Last OLED FPS update time
int KBBounceMillis = 0;           // Last keyboard debounce time
volatile int timeoutMillis = 0;   // Timeout tracking
volatile int prevTimeMillis = 0;  // Previous time for timeout
volatile bool TCA8418_event = false;  // Keypad interrupt event
volatile bool PWR_BTN_event = false;  // Power button event
volatile bool SHFT = false;           // Shift key state
volatile bool FN = false;             // Function key state
volatile bool newState = false;       // App state changed
bool noTimeout = false;               // Disable timeout
volatile bool OLEDPowerSave = false;  // OLED power save mode
volatile bool disableTimeout = false; // Disable timeout globally
volatile int battState = 0;           // Battery state
// volatile int prevBattState = 0;       // Previous battery state
unsigned int flashMillis = 0;         // Flash timing
int prevTime = 0;                     // Previous time (minutes)
uint8_t prevSec = 0;                  // Previous seconds
TaskHandle_t einkHandlerTaskHandle = NULL; // E-Ink handler task
uint8_t partialCounter = 0;           // Counter for partial refreshes
volatile bool forceSlowFullUpdate = false; // Force slow full update

// ===================== KEYBOARD STATE =====================
// char currentKB[4][10];            // Current keyboard layout
KBState CurrentKBState = NORMAL;  // Current keyboard state

// ===================== FILES & TEXT =====================
volatile bool SDCARD_INSERT = false;  // SD card inserted event
bool noSD = false;                    // No SD card present
volatile bool SDActive = false;       // SD card active
String editingFile;                   // Currently edited file
// const GFXfont *currentFont = (GFXfont *)&FreeSerif9pt7b; // Current font
// uint8_t maxCharsPerLine = 0;          // Max chars per line (display)
// uint8_t maxLines = 0;                 // Max lines per screen
// uint8_t fontHeight = 0;               // Font height in pixels
// uint8_t lineSpacing = 6;              // Line spacing in pixels
String workingFile = "";              // Working file name
String filesList[MAX_FILES];          // List of files

// ===================== APP STATES =====================
const String appStateNames[] = { "txt", "filewiz", "usb", "bt", "settings", "tasks", "calendar", "journal", "lexicon", "script" , "loader" }; // App state names
const unsigned char *appIcons[11] = { _homeIcons2, _homeIcons3, _homeIcons4, _homeIcons5, _homeIcons6, taskIconTasks0, _homeIcons7, _homeIcons8, _homeIcons9, _homeIcons11, _homeIcons10}; // App icons
AppState CurrentAppState;             // Current app state

// ===================== TXT APP =====================
volatile bool newLineAdded = true;           // New line added in TXT
std::vector<String> allLines;                // All lines in TXT

// ===================== TASKS APP =====================
std::vector<std::vector<String>> tasks;      // Task list

// ===================== HOME APP =====================
HOMEState CurrentHOMEState = HOME_HOME;      // Current home state