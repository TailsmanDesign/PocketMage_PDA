//  .d88888b  888888ba   //
//  88.    "' 88    `8b  //
//  `Y88888b. 88     88  //
//        `8b 88     88  //
//  d8'   .8P 88    .8P  //
//   Y88888P  8888888P   //
#pragma region COMMON

#include <pocketmage.h>
#include <globals.h>
#include <config.h> // for FULL_REFRESH_AFTER
#include <SD_MMC.h>
#include <SD.h>
#include <SPI.h>

static constexpr const char* TAG = "SD";

extern bool SAVE_POWER;

// Initialization of sd classes
static PocketmageSDMMC pm_sdmmc;
static PocketmageSDSPI pm_sdspi;
static PocketmageSDAUTO pm_sdauto;

// Helpers
static int countVisibleChars(String input) {
  int count = 0;

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    // Check if the character is a visible character or space
    if (c >= 32 && c <= 126) {  // ASCII range for printable characters and space
      count++;
    }
  }

  return count;
}

// Setup for SD Class
// @ dependencies:
//   - setupOled()
//   - setupBZ()
//   - setupEINK()
void setupSD() {
  // ---------- File templates ----------
  static const char* GUIDE_BACKGROUND =
    "How to add custom backgrounds:\n"
    "1. Make a background that is 1 bit (black OR white) and 320x240 pixels.\n"
    "2. Export your background as a .bmp file.\n"
    "3. Use image2cpp to convert your image to a .bin file.\n"
    "   Settings: Invert Image Colors = TRUE, Swap Bits in Byte = FALSE.\n"
    "4. Place the .bin file in this folder.\n"
    "5. Enjoy your new custom wallpapers!";

  static const char* GUIDE_COMMANDS =
    "# PocketMage Keystrokes Guide\n" 
    "This is a guide on common key combinations and commands on the PocketMage PDA device. " 
    "The guide is split up into sections based on application.\n" "\n" "---\n" 
    "## General Keystrokes (work in almost any app)\n" 
    "- (FN) + ( < ) | Exit or back button\n" 
    "- (FN) + ( > ) | Save document\n" 
    "- (FN) + ( o ) | Clear Line\n" 
    "- (FN) + (Key) | FN layer keymapping (legends on the PCB)\n" 
    "- (SHFT) + (key) | Capital letter\n" 
    "- ( o ) OR (ENTER) | Select button\n" 
    "\n" 
    "---\n" 
    "## While Sleeping\n" 
    "### Bypass home and directly enter an app\n" 
    "You can bypass the home menu and enter directly into an app and wake up with one keystroke. " 
    "Pressing the buttons below while PocketMage is sleeping will wake the device and boot into the corresponding app.\n" 
    "\n" 
    "- ( SPACE ) - Return to previous app (saved state from last sleep)\n" 
    "- ( H ) - Home\n" 
    "- ( U ) - USB\n" 
    "- ( F ) - Filewiz\n" 
    "- ( T ) - Tasks\n" 
    "- ( N ) - TXT\n" 
    "- ( S ) - Settings\n" 
    "- ( C ) - Calendar\n" 
    "- ( J ) - Journal\n" 
    "- ( D ) - Dictionary (lexicon)\n" 
    "- ( L ) - Loader\n" 
    "\n" 
    "---\n" 
    "## Home App\n" 
    "### Entering an OS app\n" 
    "Type an app's name to enter that app. For example, to enter calendar, type \"calendar\". " 
    "You can type the name as it appears on the screen or use a shortcut. " "For example, typing \"cal\" also enters the calendar.\n" 
    "\n" 
    "### Entering a 3rd party app\n" 
    "For 3rd party apps, type the letter of the slot that app is installed in. " 
    "For example if you have the Calc app installed in the first app slot, type \"a\" to enter the app.\n" 
    "\n" 
    "### Other commands\n" 
    "Many other commands can be done from the homescreen, including all of the settings commands " 
    "and some other fun ones for you to discover!\n" 
    "\n" 
    "---\n" 
    "## TXT App\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "- (FN) + ( > ) | Save document\n" 
    "- (FN) + ( o ) | Enter filesystem (loading files)\n" 
    "- (SHFT) + ( o ) | New blank text document\n" 
    "- (FN) + (Key) | FN layer keymapping (legends on the PCB)\n" 
    "- (SHFT) + (key) | Capital letter\n" 
    "- (ENTER) | Create a new line\n" 
    "- (SHFT) + ( < ) | Change text style (body, heading, etc.)\n" 
    "- (SHFT) + ( > ) | Change formatting (bold, italics, etc.)\n" 
    "- Scroll Bar | Swipe up or down to scroll through the document\n" 
    "\n" 
    "---\n" 
    "## FILEWIZ\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "- ( < ) AND ( > ) | Scroll left and right\n" 
    "- ( o ) OR (ENTER) | Select file or folder\n" 
    "- ( 0 ) TO ( 9 ) | Select recent file\n" 
    "- ( BKSP ) | Go back a filesystem level\n" 
    "\n" 
    "---\n" 
    "## USB\n" 
    "Plug in the PocketMage to your PC to view the files. Eject and exit the app when you're finished.\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "---\n" 
    "## Settings\n" 
    "Type the setting as it appears on the screen to change it. Some examples are given below. " 
    "Note: all settings are case-insensitive, meaning that you can type in all lowercase. " 
    "All of these settings are also available from the home menu command bar if you memorize them.\n" 
    "- TimeSet [HH]:[MM] -> TimeSet 15:46\n" 
    "- DateSet YYYYMMDD -> DateSet 20251230\n" 
    "- ShowYear [bool] -> ShowYear t\n" 
    "- Timeout [int] -> Timeout 300\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "---\n" 
    "## Tasks\n" 
    "- ( N ) | Create a new task, follow on-screen prompts\n" 
    "- (ENTER) | Enter information into prompt\n" 
    "- ( 0 ) TO ( 9 ) | Select task for editing\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "---\n" 
    "## Calendar\n" 
    "Type commands to navigate dates or create events. All commands are case-insensitive.\n" 
    "\n" 
    "### Month View\n" 
    "- jan 2025 / feb 2030 / etc. | Jump to month and year\n" 
    "- 20251225 | Jump to exact date (YYYYMMDD)\n" 
    "- 14 | Jump to a day in the current month\n" 
    "- ( N ) | New event\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "### Week View\n" 
    "- sun, mon, tue, wed, thu, fri, sat | Jump to weekday in the viewed week\n" 
    "- ( N ) | New event\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "### Day View\n" 
    "- ( N ) | New event for selected day\n" 
    "- 1, 2, 3, ... | Open event by index\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "### Repeating Events\n" 
    "- no | No repeat\n" 
    "- daily | Repeat every day\n" 
    "- weekly xx | Repeat every week, xx is one or more of mo, tu, we, th, fr, sa, su\n" 
    "- monthly xx | Repeat monthly, xx is the day of the month (1-31) or ordinal weekday (ex. 2tu)\n" 
    "- yearly xx | Repeat every year, xx is month and day of the month (ex. apr22)\n" 
    "\n" 
    "---\n" 
    "## Journal\n" 
    "Type a date to open or create a journal entry. Commands are case-insensitive.\n" 
    "- ( T ) | Open today’s journal entry\n" 
    "- YYYYMMDD - Example: 20250314 | Open/create entry for exact date\n" 
    "- jan 1 / feb 12 / etc. | Open/create entry for given month and day (uses current year)\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "---\n" 
    "## Lexicon\n" 
    "Type a word to search the dictionary. Matches are loaded from the SD card. Commands are case-insensitive.\n" 
    "- Type any word | Search for definitions (example: abandon)\n" 
    "- (ENTER) | Execute search\n" 
    "- ( < ) OR ( > ) | Previous / next definition\n" 
    "- (FN) + ( < ) | Exit app\n" 
    "\n" 
    "---\n" 
    "## App loader\n" 
    "Manage and install .tar apps to OTA slots. Commands are case-insensitive.\n" 
    "- A / B / C / D | Select OTA slot to edit\n" 
    "- ( S ) | Swap app in selected slot (choose a .tar file)\n" 
    "- ( D ) | Delete app in selected slot\n" 
    "- (FN) + ( < ) | Exit app / return to menu\n" 
    "- Progress Bar | Shows extraction (0–50%) and installation (50–100%) status\n" 
    "\n" 
    "---\n" 
    "## Sleep Modes\n" 
    "When on battery, save power and look at a random screensaver. " 
    "When charging, view a clock, upcoming tasks, and weather (work in progress)\n" 
    "### Sleep (when not plugged into usb)\n" 
    "- sleep button to enter sleep\n" 
    "- any key on keyboard to wake\n" 
    "### Now-Later (when usb is plugged in)\n" 
    "- sleep button to enter now-later\n" 
    "- sleep button to wake\n";

  // ---------- SDMMC mode ----------
  // Load compatibility mode
  prefs.begin("PocketMage", true);
  SD_SPI_COMPATIBILITY = prefs.getBool("SD_SPI_CMPT", false);
  ALLOW_NO_MICROSD = prefs.getBool("ALLOW_NO_SD", true);
  prefs.end();
  Serial.print("SD_SPI_CMPT" + String(SD_SPI_COMPATIBILITY));
  delay(100);

  if (!SD_SPI_COMPATIBILITY) {
    // Set global filesystem
    global_fs = &SD_MMC;

    pocketmage::setCpuSpeed(240);
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0);

    bool sdOK = false;
    bool startedSD = false;
    sdcard_type_t cardType = CARD_NONE;
    for (int attempt = 1; attempt <= 25; attempt++) {
        if (SD_MMC.begin("/sdcard", true)) {
            startedSD = true;
            delay(120); 
            cardType = SD_MMC.cardType();
            if (cardType != CARD_NONE) {
                sdOK = true;
                break;
            }
        }
        SD_MMC.end();
        delay(200);
    }

    if (!sdOK) {
        ESP_LOGE(TAG, "MOUNT FAILED");
        if (startedSD) {
            OLED().oledWord(
                String("SD Not Detected! [") +
                (cardType == CARD_MMC  ? "MMC"  :
                  cardType == CARD_SD   ? "SD"   :
                  cardType == CARD_SDHC ? "SDHC" :
                                          "NONE") + "]",
                false, false
            );
        } else {
          OLED().oledWord("SD Not Detected! [START_FAIL]", false, false);
          delay(3000);
          OLED().oledWord("Entering Compatibility Mode", false, false);
          prefs.begin("PocketMage", false);
          prefs.putBool("SD_SPI_CMPT", true);
          prefs.end();
          delay(3000);
          esp_restart();
        }

        delay(5000);
        if (ALLOW_NO_MICROSD) {
          OLED().oledWord("All Work Will Be Lost!", false, false);
          delay(5000);
          PM_SDMMC().setNoSD(true);
          return;
        } else {
          OLED().oledWord("Insert SD Card and Reboot!", false, false);
          delay(5000);
          OLED().setPowerSave(1);
          BZ().playJingle(Jingles::Shutdown);
          esp_deep_sleep_start();
          return;
        }
    }

    prefs.begin("PocketMage", false);
    prefs.putBool("SD_SPI_CMPT", false);
    prefs.end();

    // ---------- Filesystem setup ----------
    const char* dirs[] = {"/sys", "/notes", "/journal", "/dict", "/apps",
                          "/apps/temp", "/assets", "/assets/backgrounds"};
    for (auto dir : dirs) if (!global_fs->exists(dir)) global_fs->mkdir(dir);

    // Create system guides
    if (!global_fs->exists("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt")) {
      File f = global_fs->open("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt", FILE_WRITE);
      if (f) { f.print(GUIDE_BACKGROUND); f.close(); }
    }

    if (!global_fs->exists("/sys/COMMAND_MANUAL.txt")) {
      File f = global_fs->open("/sys/COMMAND_MANUAL.txt", FILE_WRITE);
      if (f) { f.print(GUIDE_COMMANDS); f.close(); }
    }

    // Ensure system files exist
    const char* sysFiles[] = {"/sys/events.txt", "/sys/tasks.txt", "/sys/SDMMC_META.txt"};
    for (auto file : sysFiles) {
      if (!global_fs->exists(file)) {
        File f = global_fs->open(file, FILE_WRITE);
        if (f) f.close();
      }
    }
  }

  // ---------- SDSPI mode ----------
  else {
      // Set global filesystem
      global_fs = &SD;

      pocketmage::setCpuSpeed(240);

      hspi = new SPIClass(HSPI);
      hspi->begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
      pinMode(hspi->pinSS(), OUTPUT);  //HSPI SS
      if (!SD.begin(SD_CS, *hspi, 40000000)) { // adjust SPI frequency as needed
          ESP_LOGE(TAG, "SPI SD Mount Failed");
          OLED().oledWord("SPI SD Not Detected!", false, false);
          delay(2000);

          if (ALLOW_NO_MICROSD) {
              OLED().oledWord("All Work Will Be Lost!", false, false);
              delay(5000);
              PM_SDSPI().setNoSD(true);
              return;
          } else {
              OLED().oledWord("Compatibility Mode Failed. Retrying...", false, false);
              prefs.begin("PocketMage", false);
              prefs.putBool("SD_SPI_CMPT", false);
              prefs.end();
              delay(2000);
              OLED().setPowerSave(1);
              BZ().playJingle(Jingles::Shutdown);
              esp_deep_sleep_start();
              return;
          }
      }
      OLED().oledWord("SD Started In Compatibility Mode", false, false);

      // ---------- Filesystem setup ----------
      const char* dirs[] = {"/sys", "/notes", "/journal", "/dict", "/apps",
                            "/apps/temp", "/assets", "/assets/backgrounds"};
      for (auto dir : dirs) if (!global_fs->exists(dir)) global_fs->mkdir(dir);

      // Create system guides
      if (!global_fs->exists("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt")) {
          File f = global_fs->open("/assets/backgrounds/HOWTOADDBACKGROUNDS.txt", FILE_WRITE);
          if (f) { f.print(GUIDE_BACKGROUND); f.close(); }
      }

      if (!global_fs->exists("/sys/COMMAND_MANUAL.txt")) {
          File f = global_fs->open("/sys/COMMAND_MANUAL.txt", FILE_WRITE);
          if (f) { f.print(GUIDE_COMMANDS); f.close(); }
      }

      // Ensure system files exist
      const char* sysFiles[] = {"/sys/events.txt", "/sys/tasks.txt", "/sys/SDMMC_META.txt"};
      for (auto file : sysFiles) {
          if (!global_fs->exists(file)) {
              File f = global_fs->open(file, FILE_WRITE);
              if (f) f.close();
          }
      }
  }
}

// SDAUTO decides automatically between SPI and SDMMC based on which works for a specific card.
#pragma region SDAUTO
PocketmageSDAUTO& PM_SDAUTO() { return pm_sdauto; }

void PocketmageSDAUTO::saveFile() {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().saveFile();
  else PM_SDMMC().saveFile();
}
void PocketmageSDAUTO::writeMetadata(const String& path) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().writeMetadata(path);
  else PM_SDMMC().writeMetadata(path);
}
void PocketmageSDAUTO::loadFile(bool showOLED) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().loadFile(showOLED);
  else PM_SDMMC().loadFile(showOLED);
}
void PocketmageSDAUTO::delFile(String fileName) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().delFile(fileName);
  else PM_SDMMC().delFile(fileName);
}
void PocketmageSDAUTO::deleteMetadata(String path) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().deleteMetadata(path);
  else PM_SDMMC().deleteMetadata(path);
}
void PocketmageSDAUTO::renFile(String oldFile, String newFile) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().renFile(oldFile, newFile);
  else PM_SDMMC().renFile(oldFile, newFile);
}
void PocketmageSDAUTO::renMetadata(String oldPath, String newPath) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().renMetadata(oldPath, newPath);
  else PM_SDMMC().renMetadata(oldPath, newPath);
}
void PocketmageSDAUTO::copyFile(String oldFile, String newFile) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().copyFile(oldFile, newFile);
  else PM_SDMMC().copyFile(oldFile, newFile);
}
void PocketmageSDAUTO::appendToFile(String path, String inText) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().appendToFile(path, inText);
  else PM_SDMMC().appendToFile(path, inText);
}

// ===================== low level functions =====================
void PocketmageSDAUTO::listDir(fs::FS &fs, const char *dirname) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().listDir(fs, dirname);
  else PM_SDMMC().listDir(fs, dirname);
}
void PocketmageSDAUTO::readFile(fs::FS &fs, const char *path) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().readFile(fs, path);
  else PM_SDMMC().readFile(fs, path);
}
String PocketmageSDAUTO::readFileToString(fs::FS &fs, const char *path) {
  if (SD_SPI_COMPATIBILITY) return PM_SDSPI().readFileToString(fs, path);
  return PM_SDMMC().readFileToString(fs, path);
}
void PocketmageSDAUTO::writeFile(fs::FS &fs, const char *path, const char *message) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().writeFile(fs, path, message);
  else PM_SDMMC().writeFile(fs, path, message);
}
void PocketmageSDAUTO::appendFile(fs::FS &fs, const char *path, const char *message) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().appendFile(fs, path, message);
  else PM_SDMMC().appendFile(fs, path, message);
}
void PocketmageSDAUTO::renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().renameFile(fs, path1, path2);
  else PM_SDMMC().renameFile(fs, path1, path2);
}
void PocketmageSDAUTO::deleteFile(fs::FS &fs, const char *path) {
  if (SD_SPI_COMPATIBILITY) PM_SDSPI().deleteFile(fs, path);
  else PM_SDMMC().deleteFile(fs, path);
}
bool PocketmageSDAUTO::readBinaryFile(const char* path, uint8_t* buf, size_t len) {
  if (SD_SPI_COMPATIBILITY) return PM_SDSPI().readBinaryFile(path, buf, len);
  return PM_SDMMC().readBinaryFile(path, buf, len);
}
size_t PocketmageSDAUTO::getFileSize(const char* path) {
  if (SD_SPI_COMPATIBILITY) return PM_SDSPI().getFileSize(path);
  return PM_SDMMC().getFileSize(path);
}
#pragma endregion


// SDMMC is Espressif's built-in hardware for SD cards on ESP32
#pragma region SDMMC
// Access for other apps
PocketmageSDMMC& PM_SDMMC() { return pm_sdmmc; }
    
void PocketmageSDMMC::saveFile() {
  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("SAVE FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      String textToSave = vectorToString();
      ESP_LOGV(TAG, "Text to save: %s", textToSave.c_str());

      if (PM_SDMMC().getEditingFile() == "" || PM_SDMMC().getEditingFile() == "-")
      PM_SDMMC().setEditingFile("/temp.txt");
      keypad.disableInterrupts();
      if (!PM_SDMMC().getEditingFile().startsWith("/"))
      PM_SDMMC().setEditingFile("/" + PM_SDMMC().getEditingFile());
      //OLED().oledWord("Saving File: "+ editingFile);
      PM_SDMMC().writeFile(SD_MMC, (PM_SDMMC().getEditingFile()).c_str(), textToSave.c_str());
      //OLED().oledWord("Saved: "+ editingFile);

      // Write MetaData
      PM_SDMMC().writeMetadata(PM_SDMMC().getEditingFile());

      // delay(1000);
      keypad.enableInterrupts();
      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
}  
void PocketmageSDMMC::writeMetadata(const String& path) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  File file = global_fs->open(path);
  if (!file || file.isDirectory()) {
      OLED().oledWord("META WRITE ERR");
      delay(1000);
      ESP_LOGE(TAG, "Invalid file for metadata: %s", path);
      return;
  }
  // Get file size
  size_t fileSizeBytes = file.size();
  file.close();

  // Format size string
  String fileSizeStr = String(fileSizeBytes) + " Bytes";

  // Get line and char counts
  int charCount = countVisibleChars(PM_SDMMC().readFileToString(SD_MMC, path.c_str()));

  String charStr = String(charCount) + " Char";
  // Get current time from RTC
  DateTime now = CLOCK().nowDT();
  char timestamp[20];
  sprintf(timestamp, "%04d%02d%02d-%02d%02d", now.year(), now.month(), now.day(), now.hour(),
          now.minute());

  // Compose new metadata line
  String newEntry = path + "|" + timestamp + "|" + fileSizeStr + "|" + charStr;

  const char* metaPath = SYS_METADATA_FILE;
  // Read existing entries and rebuild the file without duplicates
  File metaFile = global_fs->open(metaPath, FILE_READ);
  String updatedMeta = "";
  bool replaced = false;

  if (metaFile) {
      while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (line.startsWith(path + "|")) {
          updatedMeta += newEntry + "\n";
          replaced = true;
      } else if (line.length() > 1) {
          updatedMeta += line + "\n";
      }
      }
      metaFile.close();
  }

  if (!replaced) {
      updatedMeta += newEntry + "\n";
  }
  // Write back the updated metadata
  metaFile = global_fs->open(metaPath, FILE_WRITE);
  if (!metaFile) {
      ESP_LOGE(TAG, "Failed to open metadata file for writing: %s", metaPath);
      return;
  }
  metaFile.print(updatedMeta);
  metaFile.close();
  ESP_LOGI(TAG, "Metadata updated");

  if (SAVE_POWER)
  pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}  
void PocketmageSDMMC::loadFile(bool showOLED) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("LOAD FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      keypad.disableInterrupts();
      if (showOLED)
      OLED().oledWord("Loading File");
      if (!PM_SDMMC().getEditingFile().startsWith("/"))
      PM_SDMMC().setEditingFile("/" + PM_SDMMC().getEditingFile());
      String textToLoad = PM_SDMMC().readFileToString(SD_MMC, (PM_SDMMC().getEditingFile()).c_str());
      ESP_LOGV(TAG, "Text to load: %s", textToLoad.c_str());

      stringToVector(textToLoad);
      keypad.enableInterrupts();
      if (showOLED) {
      OLED().oledWord("File Loaded");
      delay(200);
      }
      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
}  
void PocketmageSDMMC::delFile(String fileName) {
  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("DELETE FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      keypad.disableInterrupts();
      // OLED().oledWord("Deleting File: "+ fileName);
      if (!fileName.startsWith("/"))
      fileName = "/" + fileName;
      PM_SDMMC().deleteFile(SD_MMC, fileName.c_str());
      // OLED().oledWord("Deleted: "+ fileName);

      // Delete MetaData
      PM_SDMMC().deleteMetadata(fileName);

      delay(1000);
      keypad.enableInterrupts();
      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
}  
void PocketmageSDMMC::deleteMetadata(String path) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* metaPath = SYS_METADATA_FILE;

  // Open metadata file for reading
  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
      ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
      return;
  }

  // Store lines that don't match the given path
  std::vector<String> keptLines;
  while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (!line.startsWith(path + "|")) {
      keptLines.push_back(line);
      }
  }
  metaFile.close();

  // Delete the original metadata file
  global_fs->remove(metaPath);

  // Recreate the file and write back the kept lines
  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
      ESP_LOGE(TAG, "Failed to recreate metadata file. %s", writeFile.path());
      return;
  }

  for (const String& line : keptLines) {
      writeFile.println(line);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata entry deleted (if it existed).");
}  
void PocketmageSDMMC::renFile(String oldFile, String newFile) {
  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("RENAME FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      keypad.disableInterrupts();
      // OLED().oledWord("Renaming "+ oldFile + " to " + newFile);
      if (!oldFile.startsWith("/"))
      oldFile = "/" + oldFile;
      if (!newFile.startsWith("/"))
      newFile = "/" + newFile;
      PM_SDMMC().renameFile(SD_MMC, oldFile.c_str(), newFile.c_str());
      OLED().oledWord(oldFile + " -> " + newFile);
      delay(1000);

      // Update MetaData
      PM_SDMMC().renMetadata(oldFile, newFile);

      keypad.enableInterrupts();
      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
}  
void PocketmageSDMMC::renMetadata(String oldPath, String newPath) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);
  const char* metaPath = SYS_METADATA_FILE;

  // Open metadata file for reading
  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
      ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
      return;
  }

  std::vector<String> updatedLines;

  while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (line.startsWith(oldPath + "|")) {
      // Replace old path with new path at the start of the line
      int separatorIndex = line.indexOf('|');
      if (separatorIndex != -1) {
          // Keep rest of line after '|'
          String rest = line.substring(separatorIndex);
          line = newPath + rest;
      } else {
          // Just replace whole line with new path if malformed
          line = newPath;
      }
      }
      updatedLines.push_back(line);
  }

  metaFile.close();

  // Delete old metadata file
  global_fs->remove(metaPath);

  // Recreate file and write updated lines
  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
      ESP_LOGE(TAG, "Failed to recreate metadata file. %s", writeFile.path());
      return;
  }

  for (const String& l : updatedLines) {
      writeFile.println(l);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata updated for renamed file.");

  if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
} 
void PocketmageSDMMC::copyFile(String oldFile, String newFile) {
  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("COPY FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      keypad.disableInterrupts();
      OLED().oledWord("Loading File");
      if (!oldFile.startsWith("/"))
      oldFile = "/" + oldFile;
      if (!newFile.startsWith("/"))
      newFile = "/" + newFile;
      String textToLoad = PM_SDMMC().readFileToString(SD_MMC, (oldFile).c_str());
      PM_SDMMC().writeFile(SD_MMC, (newFile).c_str(), textToLoad.c_str());
      OLED().oledWord("Saved: " + newFile);

      // Write MetaData
      PM_SDMMC().writeMetadata(newFile);

      delay(1000);
      keypad.enableInterrupts();

      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
} 
void PocketmageSDMMC::appendToFile(String path, String inText) {
  if (PM_SDMMC().getNoSD()) {
      OLED().oledWord("OP FAILED - No SD!");
      delay(5000);
      return;
  } else {
      SDActive = true;
      pocketmage::setCpuSpeed(240);
      delay(50);

      keypad.disableInterrupts();
      PM_SDMMC().appendFile(SD_MMC, path.c_str(), inText.c_str());

      // Write MetaData
      PM_SDMMC().writeMetadata(path);

      keypad.enableInterrupts();

      if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
      SDActive = false;
  }
}

// ===================== low level functions =====================
// Low-Level SDMMC Operations switch to using internal fs::FS*
void PocketmageSDMMC::listDir(fs::FS &fs, const char *dirname) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Listing directory %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open directory: %s", root.path());
      return;
    }
    if (!root.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Not a directory: %s", root.path());
      
      return;
    }

    // Reset fileIndex and initialize filesList with "-"
    fileIndex_ = 0; // Reset fileIndex
    for (int i = 0; i < MAX_FILES; i++) {
      filesList_[i] = "-";
    }

    File file = root.openNextFile();
    while (file && fileIndex_ < MAX_FILES) {
      if (!file.isDirectory()) {
        String fileName = String(file.name());
        
        // Check if file is in the exclusion list
        bool excluded = false;
        for (const String &excludedFile : excludedFiles_) {
          if (fileName.equals(excludedFile) || ("/"+fileName).equals(excludedFile)) {
            excluded = true;
            break;
          }
        }

        if (!excluded) {
          filesList_[fileIndex_++] = fileName; // Store file name if not excluded
        }
      }
      file = root.openNextFile();
    }

    // for (int i = 0; i < fileIndex_; i++) { // Only print valid entries
    //   Serial.println(filesList_[i]);       // NOTE: This prints out valid files
    // }

    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDMMC::readFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Reading file %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", file.path());
      return;
    }

    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
String PocketmageSDMMC::readFileToString(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return "";
  }
  else { 
    pocketmage::setCpuSpeed(240);
    delay(50);

    noTimeout = true;
    ESP_LOGI(tag, "Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", path);
      OLED().oledWord("Load Failed");
      delay(500);
      return "";  // Return an empty string on failure
    }

    ESP_LOGI(tag, "Reading from file: %s", file.path());
    String content = file.readString();

    file.close();
    EINK().setFullRefreshAfter(FULL_REFRESH_AFTER); //Force a full refresh
    noTimeout = false;
    return content;  // Return the complete String
  }
}
void PocketmageSDMMC::writeFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Writing file: %s\r\n", path);
    delay(200);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open %s for writing", path);
      return;
    }
    if (file.print(message)) {
      ESP_LOGV(tag, "File written %s", path);
    } 
    else {
      ESP_LOGE(tag, "Write failed for %s", path);
    }
    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDMMC::appendFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open for appending: %s", path);
      return;
    }
    if (file.println(message)) {
      ESP_LOGV(tag, "Message appended to %s", path);
    } 
    else {
      ESP_LOGE(tag, "Append failed: %s", path);
    }
    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDMMC::renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Renaming file %s to %s\r\n", path1, path2);

    if (fs.rename(path1, path2)) {
      ESP_LOGV(tag, "Renamed %s to %s\r\n", path1, path2);
    } 
    else {
      ESP_LOGE(tag, "Rename failed: %s to %s", path1, path2);
    }
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDMMC::deleteFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
      ESP_LOGV(tag, "File deleted: %s", path);
    } 
    else {
      ESP_LOGE(tag, "Delete failed for %s", path);
    }
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
bool PocketmageSDMMC::readBinaryFile(const char* path, uint8_t* buf, size_t len) {
  if (noSD_) {
      OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return false;
  }

  setCpuFrequencyMhz(240  );
  if (noTimeout)
    noTimeout = true;
    
  File f = global_fs->open(path, "r");
  if (!f || f.isDirectory()) {
    if (noTimeout)
      noTimeout = false;
    ESP_LOGE(tag, "Failed to open file: %s", path);
    return false;
  }

  size_t n = f.read(buf, len);
  f.close();

  if (noTimeout)
    noTimeout = false;
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  return n == len;
}
size_t PocketmageSDMMC::getFileSize(const char* path) {
  if (noSD_)
    return 0;

  File f = global_fs->open(path, "r");
  if (!f)
    return 0;
  size_t size = f.size();
  f.close();
  return size;
}
#pragma endregion


// SDSPI uses standard SPI communication for compatibility
#pragma region SDSPI
// Access for other apps
PocketmageSDSPI& PM_SDSPI() { return pm_sdspi; }

void PocketmageSDSPI::saveFile() {
  if (getNoSD()) {
    OLED().oledWord("SAVE FAILED - No SD!");
    delay(5000);
    return;
  }

  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  String textToSave = vectorToString();

  if (getEditingFile().isEmpty() || getEditingFile() == "-")
    setEditingFile("/temp.txt");

  keypad.disableInterrupts();

  if (!getEditingFile().startsWith("/"))
    setEditingFile("/" + getEditingFile());

  writeFile(SD, getEditingFile().c_str(), textToSave.c_str());
  writeMetadata(getEditingFile());

  keypad.enableInterrupts();

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  SDActive = false;
}
void PocketmageSDSPI::writeMetadata(const String& path) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  File file = global_fs->open(path);
  if (!file || file.isDirectory()) {
    OLED().oledWord("META WRITE ERR");
    delay(1000);
    ESP_LOGE(TAG, "Invalid file for metadata: %s", path.c_str());
    return;
  }

  // Get file size
  size_t fileSizeBytes = file.size();
  file.close();

  // Format size string
  String fileSizeStr = String(fileSizeBytes) + " Bytes";

  // Get line and char counts
  int charCount =
      countVisibleChars(PM_SDSPI().readFileToString(SD, path.c_str()));
  String charStr = String(charCount) + " Char";

  // Get current time from RTC
  DateTime now = CLOCK().nowDT();
  char timestamp[20];
  sprintf(timestamp, "%04d%02d%02d-%02d%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute());

  // Compose new metadata line
  String newEntry = path + "|" + timestamp + "|" + fileSizeStr + "|" + charStr;

  const char* metaPath = SYS_METADATA_FILE;

  // Read existing metadata
  File metaFile = global_fs->open(metaPath, FILE_READ);
  String updatedMeta;
  bool replaced = false;

  if (metaFile) {
    while (metaFile.available()) {
      String line = metaFile.readStringUntil('\n');
      if (line.startsWith(path + "|")) {
        updatedMeta += newEntry + "\n";
        replaced = true;
      } else if (line.length() > 1) {
        updatedMeta += line + "\n";
      }
    }
    metaFile.close();
  }

  if (!replaced) {
    updatedMeta += newEntry + "\n";
  }

  // Write back metadata
  metaFile = global_fs->open(metaPath, FILE_WRITE);
  if (!metaFile) {
    ESP_LOGE(TAG, "Failed to open metadata file for writing: %s", metaPath);
    return;
  }

  metaFile.print(updatedMeta);
  metaFile.close();
  ESP_LOGI(TAG, "Metadata updated");

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  SDActive = false;
}
void PocketmageSDSPI::loadFile(bool showOLED) {
  if (getNoSD()) {
    OLED().oledWord("LOAD FAILED - No SD!");
    delay(5000);
    return;
  }

  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();

  if (showOLED)
    OLED().oledWord("Loading File");

  if (!getEditingFile().startsWith("/"))
    setEditingFile("/" + getEditingFile());

  String text = readFileToString(SD, getEditingFile().c_str());
  stringToVector(text);

  keypad.enableInterrupts();

  if (showOLED) {
    OLED().oledWord("File Loaded");
    delay(200);
  }

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  SDActive = false;
}
void PocketmageSDSPI::delFile(String fileName) {
  if (PM_SDSPI().getNoSD()) {
    OLED().oledWord("DELETE FAILED - No SD!");
    delay(5000);
    return;
  } else {
    SDActive = true;
    pocketmage::setCpuSpeed(240);
    delay(50);

    keypad.disableInterrupts();
    // OLED().oledWord("Deleting File: " + fileName);

    if (!fileName.startsWith("/"))
      fileName = "/" + fileName;

    PM_SDSPI().deleteFile(SD, fileName.c_str());

    // Delete metadata
    PM_SDSPI().deleteMetadata(fileName);

    delay(1000);
    keypad.enableInterrupts();

    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    SDActive = false;
  }
}
void PocketmageSDSPI::deleteMetadata(String path) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* metaPath = SYS_METADATA_FILE;

  // Open metadata file for reading
  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
    ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
    return;
  }

  // Store lines that don't match the given path
  std::vector<String> keptLines;
  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');
    if (!line.startsWith(path + "|")) {
      keptLines.push_back(line);
    }
  }
  metaFile.close();

  // Delete original metadata file
  global_fs->remove(metaPath);

  // Recreate file and write kept lines
  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
    ESP_LOGE(TAG, "Failed to recreate metadata file. %s", writeFile.path());
    return;
  }

  for (const String& line : keptLines) {
    writeFile.println(line);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata entry deleted (if it existed).");
}
void PocketmageSDSPI::renFile(String oldFile, String newFile) {
  if (PM_SDSPI().getNoSD()) {
    OLED().oledWord("RENAME FAILED - No SD!");
    delay(5000);
    return;
  } else {
    SDActive = true;
    pocketmage::setCpuSpeed(240);
    delay(50);

    keypad.disableInterrupts();

    if (!oldFile.startsWith("/"))
      oldFile = "/" + oldFile;
    if (!newFile.startsWith("/"))
      newFile = "/" + newFile;

    if (global_fs->rename(oldFile.c_str(), newFile.c_str())) {
      OLED().oledWord(oldFile + " -> " + newFile);
      delay(1000);

      // Update metadata
      PM_SDSPI().renMetadata(oldFile, newFile);
    } else {
      ESP_LOGE(TAG, "Rename failed: %s -> %s", oldFile.c_str(), newFile.c_str());
      OLED().oledWord("RENAME FAILED");
      delay(1000);
    }

    keypad.enableInterrupts();
    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    SDActive = false;
  }
}
void PocketmageSDSPI::renMetadata(String oldPath, String newPath) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* metaPath = SYS_METADATA_FILE;

  // Open metadata file for reading
  File metaFile = global_fs->open(metaPath, FILE_READ);
  if (!metaFile) {
    ESP_LOGE(TAG, "Metadata file not found: %s", metaPath);
    return;
  }

  std::vector<String> updatedLines;

  while (metaFile.available()) {
    String line = metaFile.readStringUntil('\n');

    if (line.startsWith(oldPath + "|")) {
      int separatorIndex = line.indexOf('|');
      if (separatorIndex != -1) {
        String rest = line.substring(separatorIndex);
        line = newPath + rest;
      } else {
        line = newPath;
      }
    }

    updatedLines.push_back(line);
  }

  metaFile.close();

  // Delete old metadata file
  global_fs->remove(metaPath);

  // Recreate file and write updated lines
  File writeFile = global_fs->open(metaPath, FILE_WRITE);
  if (!writeFile) {
    ESP_LOGE(TAG, "Failed to recreate metadata file: %s", metaPath);
    return;
  }

  for (const String& l : updatedLines) {
    writeFile.println(l);
  }

  writeFile.close();
  ESP_LOGI(TAG, "Metadata updated for renamed file.");

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}
void PocketmageSDSPI::copyFile(String oldFile, String newFile) {
  if (PM_SDSPI().getNoSD()) {
    OLED().oledWord("COPY FAILED - No SD!");
    delay(5000);
    return;
  } else {
    SDActive = true;
    pocketmage::setCpuSpeed(240);
    delay(50);

    keypad.disableInterrupts();
    OLED().oledWord("Loading File");

    if (!oldFile.startsWith("/"))
      oldFile = "/" + oldFile;
    if (!newFile.startsWith("/"))
      newFile = "/" + newFile;

    // Read source file
    File src = global_fs->open(oldFile.c_str(), FILE_READ);
    if (!src || src.isDirectory()) {
      ESP_LOGE(TAG, "Failed to open source file: %s", oldFile.c_str());
      OLED().oledWord("COPY FAILED");
      keypad.enableInterrupts();
      return;
    }

    // Write destination file
    File dst = global_fs->open(newFile.c_str(), FILE_WRITE);
    if (!dst) {
      ESP_LOGE(TAG, "Failed to open destination file: %s", newFile.c_str());
      src.close();
      OLED().oledWord("COPY FAILED");
      keypad.enableInterrupts();
      return;
    }

    while (src.available()) {
      dst.write(src.read());
    }

    src.close();
    dst.close();

    OLED().oledWord("Saved: " + newFile);

    // Write metadata
    PM_SDSPI().writeMetadata(newFile);

    delay(1000);
    keypad.enableInterrupts();

    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    SDActive = false;
  }
}
void PocketmageSDSPI::appendToFile(String path, String inText) {
  if (getNoSD()) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }

  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  keypad.disableInterrupts();

  if (!path.startsWith("/"))
    path = "/" + path;

  File file = global_fs->open(path.c_str(), FILE_APPEND);
  if (!file) {
    OLED().oledWord("APPEND FAILED");
    keypad.enableInterrupts();
    SDActive = false;
    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  file.print(inText);
  file.close();

  // Write MetaData
  writeMetadata(path);

  keypad.enableInterrupts();

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  SDActive = false;
}

// ===================== low level functions =====================
// Low-Level SDMMC Operations switch to using internal fs::FS*
void PocketmageSDSPI::listDir(fs::FS &fs, const char *dirname) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Listing directory %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open directory: %s", root.path());
      return;
    }
    if (!root.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Not a directory: %s", root.path());
      
      return;
    }

    // Reset fileIndex and initialize filesList with "-"
    fileIndex_ = 0; // Reset fileIndex
    for (int i = 0; i < MAX_FILES; i++) {
      filesList_[i] = "-";
    }

    File file = root.openNextFile();
    while (file && fileIndex_ < MAX_FILES) {
      if (!file.isDirectory()) {
        String fileName = String(file.name());
        
        // Check if file is in the exclusion list
        bool excluded = false;
        for (const String &excludedFile : excludedFiles_) {
          if (fileName.equals(excludedFile) || ("/"+fileName).equals(excludedFile)) {
            excluded = true;
            break;
          }
        }

        if (!excluded) {
          filesList_[fileIndex_++] = fileName; // Store file name if not excluded
        }
      }
      file = root.openNextFile();
    }

    // for (int i = 0; i < fileIndex_; i++) { // Only print valid entries
    //   Serial.println(filesList_[i]);       // NOTE: This prints out valid files
    // }

    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDSPI::readFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Reading file %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", file.path());
      return;
    }

    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
String PocketmageSDSPI::readFileToString(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return "";
  }
  else { 
    pocketmage::setCpuSpeed(240);
    delay(50);

    noTimeout = true;
    ESP_LOGI(tag, "Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory()) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open file for reading: %s", path);
      OLED().oledWord("Load Failed");
      delay(500);
      return "";  // Return an empty string on failure
    }

    ESP_LOGI(tag, "Reading from file: %s", file.path());
    String content = file.readString();

    file.close();
    EINK().setFullRefreshAfter(FULL_REFRESH_AFTER); //Force a full refresh
    noTimeout = false;
    return content;  // Return the complete String
  }
}
void PocketmageSDSPI::writeFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Writing file: %s\r\n", path);
    delay(200);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open %s for writing", path);
      return;
    }
    if (file.print(message)) {
      ESP_LOGV(tag, "File written %s", path);
    } 
    else {
      ESP_LOGE(tag, "Write failed for %s", path);
    }
    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDSPI::appendFile(fs::FS &fs, const char *path, const char *message) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      noTimeout = false;
      ESP_LOGE(tag, "Failed to open for appending: %s", path);
      return;
    }
    if (file.println(message)) {
      ESP_LOGV(tag, "Message appended to %s", path);
    } 
    else {
      ESP_LOGE(tag, "Append failed: %s", path);
    }
    file.close();
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDSPI::renameFile(fs::FS &fs, const char *path1, const char *path2) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Renaming file %s to %s\r\n", path1, path2);

    if (fs.rename(path1, path2)) {
      ESP_LOGV(tag, "Renamed %s to %s\r\n", path1, path2);
    } 
    else {
      ESP_LOGE(tag, "Rename failed: %s to %s", path1, path2);
    }
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
void PocketmageSDSPI::deleteFile(fs::FS &fs, const char *path) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return;
  }
  else {
    pocketmage::setCpuSpeed(240);
    delay(50);
    noTimeout = true;
    ESP_LOGI(tag, "Deleting file: %s\r\n", path);
    if (fs.remove(path)) {
      ESP_LOGV(tag, "File deleted: %s", path);
    } 
    else {
      ESP_LOGE(tag, "Delete failed for %s", path);
    }
    noTimeout = false;
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }
}
bool PocketmageSDSPI::readBinaryFile(const char* path, uint8_t* buf, size_t len) {
  if (noSD_) {
    OLED().oledWord("OP FAILED - No SD!");
    delay(5000);
    return false;
  }

  setCpuFrequencyMhz(240);
  if (noTimeout) noTimeout = true;

  File f = global_fs->open(path, FILE_READ);
  if (!f || f.isDirectory()) {
    noTimeout = false;
    ESP_LOGE(tag, "Failed to open file: %s", path);
    return false;
  }

  size_t n = f.read(buf, len);
  f.close();

  noTimeout = false;
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  return n == len;
}
size_t PocketmageSDSPI::getFileSize(const char* path) {
  if (noSD_)
    return 0;

  File f = global_fs->open(path, "r");
  if (!f)
    return 0;
  size_t size = f.size();
  f.close();
  return size;
}
#pragma endregion