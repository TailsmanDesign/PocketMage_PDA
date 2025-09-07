#ifndef GLOBALS_H
#define GLOBALS_H


// LIBRARIES
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2lib.h>
#include <Adafruit_TCA8418.h>
#include <vector>
#include <Buzzer.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include <Preferences.h>
#include <Adafruit_MPR121.h>
#include <esp_cpu.h>
#include <RTClib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <assets.h>
#include <config.h>

/* // migrated to pocketmage_eink.h
// FONTS
// 3x7
#include "Fonts/Font3x7FixedNum.h"
#include "Fonts/Font4x5Fixed.h"
#include "Fonts/Font5x7Fixed.h"

// 9x7
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
// 12x7
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
*/

/* // migrated to pocketmage_oled.h
// U8G2 FONTS
//U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
//u8g2_font_4x6_mf
//u8g2_font_5x7_mf
//u8g2_font_spleen5x8_mf
//u8g2_font_boutique_bitmap_9x9_tf
//u8g2_font_courR08_tf.h
*/

// ===================== DISPLAY =====================
// E-ink display
extern GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display;
// OLED 
extern U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2;

// ===================== INPUT DEVICES =====================
// Keypad controller
extern Adafruit_TCA8418 keypad; // Matrix keypad
extern char keysArray[4][10];       // Normal key layout
extern char keysArraySHFT[4][10];   // Shift key layout
extern char keysArrayFN[4][10];     // Function key layout

// Touch slider (capacitive)
extern Adafruit_MPR121 cap; // Touch slider
extern volatile long int dynamicScroll;      // Dynamic scroll offset
extern volatile long int prev_dynamicScroll; // Previous scroll offset
extern int lastTouch;                        // Last touch event
extern unsigned long lastTouchTime;          // Last touch time

// ===================== AUDIO =====================
// Buzzer for sound feedback
extern Buzzer buzzer;

// ===================== RTC =====================
// Real-time clock
extern RTC_PCF8563 rtc; // RTC chip
extern const char daysOfTheWeek[7][12]; // Day names

// ===================== USB & STORAGE =====================
// USB mass storage controller
extern USBMSC msc;           // USB MSC object
extern bool mscEnabled;      // Is USB MSC active?
extern sdmmc_card_t* card;   // SD card pointer

// ===================== SYSTEM SETTINGS =====================
// Persistent preferences
extern Preferences prefs;        // NVS preferences
extern int TIMEOUT;              // Auto sleep timeout (seconds)
extern bool DEBUG_VERBOSE;       // Extra debug output
extern bool SYSTEM_CLOCK;        // Show clock on screen
extern bool SHOW_YEAR;           // Show year in clock
extern bool SAVE_POWER;          // Enable power saving mode
extern bool ALLOW_NO_MICROSD;    // Allow running without SD card
extern bool HOME_ON_BOOT;        // Start home app on boot
extern int OLED_BRIGHTNESS;      // OLED brightness (0-255)
extern int OLED_MAX_FPS;         // OLED max FPS

extern String OTA1_APP;
extern String OTA2_APP;
extern String OTA3_APP;
extern String OTA4_APP;

// ===================== SYSTEM STATE =====================
// E-Ink refresh control
// extern volatile int einkRefresh;     // Partial/full refresh counter
extern int OLEDFPSMillis;            // Last OLED FPS update time
extern int KBBounceMillis;           // Last keyboard debounce time
extern volatile int timeoutMillis;   // Timeout tracking
extern volatile int prevTimeMillis;  // Previous time for timeout
extern volatile bool TCA8418_event;  // Keypad interrupt event
extern volatile bool PWR_BTN_event;  // Power button event
extern volatile bool SHFT;           // Shift key state
extern volatile bool FN;             // Function key state
extern volatile bool newState;       // App state changed
extern bool noTimeout;               // Disable timeout
extern volatile bool OLEDPowerSave;  // OLED power save mode
extern volatile bool disableTimeout; // Disable timeout globally
extern volatile int battState;       // Battery state
// extern volatile int prevBattState;   // Previous battery state
extern unsigned int flashMillis;     // Flash timing
extern int prevTime;                 // Previous time (minutes)
extern uint8_t prevSec;              // Previous seconds
extern TaskHandle_t einkHandlerTaskHandle; // E-Ink handler task

// ===================== KEYBOARD STATE =====================
// extern char currentKB[4][10];        // Current keyboard layout
extern volatile bool SDCARD_INSERT;  // SD card inserted event
extern bool noSD;                    // No SD card present
extern volatile bool SDActive;       // SD card active

// ===================== FILES & TEXT =====================
extern String editingFile;           // Currently edited file
//extern const GFXfont *currentFont;   // Current font
//extern uint8_t maxCharsPerLine;      // Max chars per line (display)
//extern uint8_t maxLines;             // Max lines per screen
//extern uint8_t fontHeight;           // Font height in pixels
//extern uint8_t lineSpacing;          // Line spacing in pixels
extern String workingFile;           // Working file name
extern String filesList[MAX_FILES];  // List of files

// ===================== APP STATES =====================
enum KBState { NORMAL, SHIFT, FUNC };    // Keyboard state
extern KBState CurrentKBState;           // Current keyboard state

// extern uint8_t partialCounter;           // E-Ink partial refresh counter
// extern volatile bool forceSlowFullUpdate;// Force slow full update

enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON, APPLOADER };
extern const String appStateNames[];     // App state names
extern const unsigned char *appIcons[11]; // App icons
extern AppState CurrentAppState;         // Current app state

// ===================== TXT APP =====================
extern volatile bool newLineAdded;           // New line added in TXT
extern std::vector<String> allLines;         // All lines in TXT

// ===================== TASKS APP =====================
extern std::vector<std::vector<String>> tasks; // Task list

// ===================== HOME APP =====================
enum HOMEState { HOME_HOME, NOWLATER };       // Home app states
extern HOMEState CurrentHOMEState;            // Current home state

// ===================== FUNCTION PROTOTYPES =====================
// <sysFunc.cpp>
/* migrated to pocketmage_sys
void checkTimeout();
void  IRAM_ATTR PWR_BTN_irq();
void TCA8418_irq(); // migrated to pocketmage_keypad.h
char updateKeypress(); // migrated to pocketmage_keypad.h
void printDebug();
void saveFile();
void writeMetadata(const String& path);
void loadFile(bool showOLED = true);
void delFile(String fileName);
void deleteMetadata(String path);
void renFile(String oldFile, String newFile);
void renMetadata(String oldPath, String newPath);
void copyFile(String oldFile, String newFile);
void updateBattState();
void setCpuSpeed(int newFreq);
void playJingle(String jingle); // migrated to pocketmage_bz.h
void deepSleep(bool alternateScreenSaver = false);
void loadState(bool changeState = true);
void updateScrollFromTouch(); // migrated to pocketmage_touch.h
void setTimeFromString(String timeStr);
*/
//int stringToInt(String str);
//String removeChar(String str, char character);
//void appendToFile(String path, String inText);
//String vectorToString();
//void stringToVector(String inputText);

/* //migrated to pocketmage_sd.h
// microSD
void listDir(fs::FS &fs, const char *dirname);
void readFile(fs::FS &fs, const char *path);
String readFileToString(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
*/

/* //migrated to pocketmage_oled.h
// <OLEDFunc.cpp> 
void oledWord(String word, bool allowLarge = false, bool showInfo = true);
void oledLine(String line, bool doProgressBar = true, String bottomMsg = "");
void oledScroll();
void infoBar();
*/

/* // migrated to pocketmage_eink.h
// <einkFunc.cpp>
void refresh();
void statusBar(String input, bool fullWindow = false);
void einkTextPartial(String text, bool noRefresh = false);
int  countLines(String input, size_t maxLineLength = 29);
void einkTextDynamic(bool doFull_, bool noRefresh = false);
void setTXTFont(const GFXfont *font);
void setFastFullRefresh(bool setting);
void drawStatusBar(String input);
void multiPassRefresh(int passes);
*/

// <FILEWIZ.cpp>
void FILEWIZ_INIT();
void processKB_FILEWIZ();
void einkHandler_FILEWIZ();
String fileWizardMini(bool allowRecentSelect = false);

// <TXT.cpp>
void TXT_INIT();
void processKB_TXT_NEW();
void einkHandler_TXT_NEW();

// <HOME.cpp>
void HOME_INIT();
void einkHandler_HOME();
void processKB_HOME();

// <TASKS.cpp>
void TASKS_INIT();
void sortTasksByDueDate(std::vector<std::vector<String>> &tasks);
void updateTaskArray();
void einkHandler_TASKS();
void processKB_TASKS();

// <settings.cpp>
void SETTINGS_INIT();
void processKB_settings();
void einkHandler_settings();
void settingCommandSelect(String command);

// <USB.cpp>
void USB_INIT();
void processKB_USB();
void einkHandler_USB();

// <CALENDAR.cpp>
void CALENDAR_INIT();
void processKB_CALENDAR();
void einkHandler_CALENDAR();

// <LEXICON.cpp>
void LEXICON_INIT();
void processKB_LEXICON();
void einkHandler_LEXICON();

// <JOURNAL.cpp>
void JOURNAL_INIT();
void processKB_JOURNAL();
void einkHandler_JOURNAL();

// <APPLOADER.cpp>
void APPLOADER_INIT();
void processKB_APPLOADER();
void einkHandler_APPLOADER();

// <PocketMage>
void einkHandler(void *parameter); // moved from EinkFunc.cpp
void applicationEinkHandler();
void processKB();

#endif // GLOBALS_H