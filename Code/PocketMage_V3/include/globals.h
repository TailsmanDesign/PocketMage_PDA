#ifndef GLOBALS_H
#define GLOBALS_H

// LIBRARIES
#include <USBMSC.h>
#include <SD_MMC.h>
#include <SD.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <pocketmage.h>
// OTA_APP: remove assets.h + assets.cpp, and OS_APPS/, follow OTA_APP: tag instructions in codebase
#include <assets.h> // OTA_APP: remove

// ===================== SPI BUSSES =====================
extern SPIClass *vspi;
extern SPIClass *hspi;
extern fs::FS* global_fs;

// ===================== SYSTEM STATE =====================
extern Preferences prefs;                       // NVS preferencesv
extern TaskHandle_t einkHandlerTaskHandle;      // E-Ink handler task
extern int OLEDFPSMillis;                       // Last OLED FPS update time
extern int KBBounceMillis;                      // Last keyboard debounce time
extern volatile bool newState;                  // App state changed
extern volatile bool disableTimeout;            // Disable timeout globally
extern bool fileLoaded;     
extern unsigned int flashMillis;                // Flash timing

extern String OTA1_APP;
extern String OTA2_APP;
extern String OTA3_APP;
extern String OTA4_APP;

// ===================== KEYBOARD STATE =====================
enum KBState { NORMAL, SHIFT, FUNC, FN_SHIFT };    // Keyboard state

// ===================== APP STATES =====================
enum AppState { HOME, TXT, FILEWIZ, USB_APP, BT, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON, APPLOADER, TERMINAL };
extern const String appStateNames[];            // App state names
extern const unsigned char *appIcons[11];       // App icons
extern AppState CurrentAppState;                // Current app state

// ===================== TASKS APP =====================
extern std::vector<std::vector<String>> tasks;  // Task list

// ===================== HOME APP =====================
enum HOMEState { HOME_HOME, NOWLATER };         // Home app states
extern HOMEState CurrentHOMEState;              // Current home state

// ===================== PocketMage APP PROTOTYPES =====================
// <APP.cpp>
void APP_INIT();
void processKB_APP();
void einkHandler_APP();

//utils.cpp
void printDebug();
void checkTimeout();
void loadState(bool changeState = true);
void updateBattState();
String textPrompt(String promptText = "", String prefix = "");
#if !OTA_APP
void saveEditingFile(); // OTA_APP: Remove saveEditingFile
#endif
// <PocketMage>
void einkHandler(void *parameter);
void applicationEinkHandler();
void processKB();


// OTA_APP: Remove all pocketmage v3 prototypes below this line
#if !OTA_APP // POCKETMAGE_OS
// <FILEWIZ.cpp>
void FILEWIZ_INIT();
void processKB_FILEWIZ();
void einkHandler_FILEWIZ();
String fileWizardMini(bool allowRecentSelect = false, String rootDir = "/");

// <TXT.cpp>
void TXT_INIT(String inPath = "");
void TXT_INIT_JournalMode();
void processKB_TXT_NEW();
void einkHandler_TXT_NEW();
void saveMarkdownFile(const String& path);

// <HOME.cpp>
void HOME_INIT();
void einkHandler_HOME();
void processKB_HOME();
String commandSelect(String command);
void mageIdle(bool internalRefresh = true);
void resetIdle();

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
String settingCommandSelect(String command);

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
String getCurrentJournal();

// <APPLOADER.cpp>
void APPLOADER_INIT();
void processKB_APPLOADER();
void einkHandler_APPLOADER();
void rebootToAppSlot(int otaIndex);
void loadAndDrawAppIcon(int x, int y, int otaIndex, bool showName = true, int maxNameChars = 10);
#endif // POCKETMAGE_OS
#endif // GLOBALS_H

// <TERMINAL.cpp>
void TERMINAL_INIT();
void processKB_TERMINAL();
void einkHandler_TERMINAL();
// Wrench
const char* readCFile(const String& path);
void compileWrench(const char* wrenchCode);
