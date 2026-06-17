// AUDIT 1
#include <globals.h>
#include <ESP32-targz.h>
#include <Update.h>
#include "esp_ota_ops.h"


#define APP_DIRECTORY   "/apps"
#define TEMP_DIR        "/apps/temp"
#define PREFS_NAMESPACE "AppLoader"
#if !OTA_APP // POCKETMAGE_OS
static String currentLine = "";

enum AppLoaderState {MENU, SWAP_OR_EDIT, INSTALLING, SWAP};
AppLoaderState CurrentAppLoaderState = MENU;

uint8_t selectedSlot = 0; //1:A, 2:B, etc.

// ---------- Globals ----------
volatile uint8_t g_installProgress = 0; // 0-100 (0-50: extract, 50-100: intall)
volatile bool g_installDone = false;
volatile bool g_installFailed = false;

// ---------- Utilities ----------
static bool ensureDir(fs::FS &fs, const char *path) {
    if (fs.exists(path)) return true;
    return fs.mkdir(path);
}

static bool rmRF(fs::FS &fs, const char *path) {
    File entry = fs.open(path);
    if (!entry) return true; // nothing to delete
    if (!entry.isDirectory()) {
        entry.close();
        return fs.remove(path);
    }

    File child;
    while ((child = entry.openNextFile())) {
        String childPath = String(path) + "/" + child.name();
        child.close();
        if (!rmRF(fs, childPath.c_str())) { entry.close(); return false; }
    }
    entry.close();
    return fs.rmdir(path);
}

bool copyFile(fs::FS &fs, const char *src, const char *dst) {
  File in = fs.open(src, "r");
  if (!in) return false;

  File out = fs.open(dst, "w");
  if (!out) { in.close(); return false; }

  uint8_t buf[1024];
  while (in.available()) {
    size_t r = in.read(buf, sizeof(buf));
    if (out.write(buf, r) != r) {
      in.close(); out.close();
      return false;
    }
    vTaskDelay(1); // feed watchdog
  }

  in.close();
  out.close();
  return true;
}

// ---------- Place this at the top of the .cpp file, before installTask ----------
void copyDirRecursive(File src, const String &assetsSrc, const String &assetsDst) {
    while (true) {
        File entry = src.openNextFile();
        if (!entry) break;

        String name = entry.name();
        
        // Skip macOS metadata files
        if (name.startsWith("._")) {
            entry.close();
            continue;
        }

        String relative = name.substring(assetsSrc.length());
        String dstFile = assetsDst + relative;

        if (entry.isDirectory()) {
            ensureDir(*global_fs, dstFile.c_str());
            copyDirRecursive(entry, assetsSrc, assetsDst);
        } else {
            File dst = global_fs->open(dstFile.c_str(), FILE_WRITE);
            if (dst) {
                uint8_t buf[512];
                size_t len;
                while ((len = entry.read(buf, sizeof(buf))) > 0) {
                    dst.write(buf, len);
                }
                dst.close();
                Serial.printf("Copied %s -> %s\n", name.c_str(), dstFile.c_str());
            } else {
                Serial.printf("Failed to open destination file %s\n", dstFile.c_str());
            }
        }
        entry.close();
        
        vTaskDelay(1); 
    }
}

bool copyAssetsFlat(fs::FS &fs, const char *srcDir, const char *dstDir) {
  ensureDir(fs, dstDir);

  File dir = fs.open(srcDir);
  if (!dir || !dir.isDirectory()) return false;

  File f;
  while ((f = dir.openNextFile())) {
    String name = String(f.name());
    
    // Skip macOS metadata files
    if (name.startsWith("._")) {
        f.close();
        continue;
    }

    String srcPath = String(srcDir) + "/" + name;
    String dstPath = String(dstDir) + "/" + name;

    if (f.isDirectory()) {
      ensureDir(fs, dstPath.c_str());
      copyAssetsFlat(fs, srcPath.c_str(), dstPath.c_str());
    } else {
      if (!copyFile(fs, srcPath.c_str(), dstPath.c_str())) {
        f.close();
        dir.close();
        return false;
      }
    }
    f.close();
    vTaskDelay(1);
  }
  dir.close();
  return true;
}


static String basenameNoExt(const String &path, const char *ext = ".tar") {
  int slash = path.lastIndexOf('/');
  String name = (slash >= 0) ? path.substring(slash + 1) : path;
  if (name.endsWith(ext)) return name.substring(0, name.length() - (int)strlen(ext));
  return name;
}

// Join two paths safely (ensures exactly one slash between them)
static String pathJoin(const String &a, const String &b) {
  if (a.length() == 0) return b;
  if (b.length() == 0) return a;
  if (a.endsWith("/")) {
    if (b.startsWith("/")) return a + b.substring(1); // avoid double slash
    return a + b;
  } else {
    if (b.startsWith("/")) return a + b;
    return a + "/" + b;
  }
}

// ---------- Saving/Loading appInfo ----------
#define APP_ICON_BYTES 200  // 40x40 monochrome = 200 bytes

struct AppInfo {
  char name[32];       // App name
  char tarPath[64];    // Path to .tar file
  char iconPath[64];   // Path to extracted icon.bmp (in /apps/temp or similar)
};

bool saveAppInfo(int otaIndex, const AppInfo &info) {
  String key = "OTA" + String(otaIndex);
  prefs.begin("PocketMage", false);
  bool ok = prefs.putBytes(key.c_str(), &info, sizeof(info)) == sizeof(info);
  prefs.end();
  return ok;
}

bool loadAppInfo(int otaIndex, AppInfo &info) {
  String key = "OTA" + String(otaIndex);
  prefs.begin("PocketMage", true);
  size_t n = prefs.getBytes(key.c_str(), &info, sizeof(info));
  prefs.end();
  return n == sizeof(info);
}

void loadAndDrawAppIcon(int x, int y, int otaIndex, bool showName, int maxNameChars) {
  pocketmage::setCpuSpeed(240);

  AppInfo app;
  if (!loadAppInfo(otaIndex, app)) {
    // Ensure CPU speed is reset if we abort early
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return;
  }

  bool iconLoaded = false;
  uint8_t buf[40 * 5]; // 40x40 1-bit = 200 bytes

  if (global_fs->exists(app.iconPath)) {
    File f = global_fs->open(app.iconPath, "r");
    if (f) {
      if (f.read(buf, sizeof(buf)) == sizeof(buf)) {
        iconLoaded = true;
      }
      f.close();
    }
  }

  display.fillRect(x, y, 40, 40, GxEPD_WHITE);

  if (iconLoaded) {
    display.drawBitmap(x, y, buf, 40, 40, GxEPD_BLACK);
  } else {
    display.drawBitmap(x, y, noIcon, 40, 40, GxEPD_BLACK);
  }

  if (showName) {
    // Make a copy and truncate
    String appNameStr = String(app.name);
    if (appNameStr.length() > maxNameChars) {
        appNameStr = appNameStr.substring(0, maxNameChars);
    }

    u8g2f.setFont(u8g2_font_ncenR10_tf);
    u8g2f.setFontMode(1);
    u8g2f.setForegroundColor(GxEPD_BLACK);

    int w = u8g2f.getUTF8Width(appNameStr.c_str());

    int tx = x + (40 - w) / 2;
    int ty = y + 40 + 13;

    u8g2f.setCursor(tx, ty);
    u8g2f.print(appNameStr);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void cleanupAppsTemp(String binPath) {
  // --- Cleanup TEMP_DIR, keep *_ICON.bin only ---
  File root = global_fs->open(TEMP_DIR);
  if (root && root.isDirectory()) {
    File entry;
    while ((entry = root.openNextFile())) {
      String name = String(entry.name());
      String fullPath = pathJoin(TEMP_DIR, name);
      entry.close();
      if (!name.endsWith("_ICON.bin")) {
        global_fs->remove(fullPath);
      }
    }
    root.close();
  }
}
bool cleanupAppsTempRecursive(fs::FS &fs, const String &dirPath) {
    File dir = fs.open(dirPath);
    if (!dir || !dir.isDirectory()) return false;

    File entry;
    while ((entry = dir.openNextFile())) {
        String name = entry.name();
        String fullPath = pathJoin(dirPath, name);

        if (entry.isDirectory()) {
            entry.close();
            cleanupAppsTempRecursive(fs, fullPath); // recurse
            // try to remove directory if empty
            fs.rmdir(fullPath.c_str());
        } else {
            entry.close();
            if (!name.endsWith("_ICON.bin")) {
                fs.remove(fullPath.c_str());
            }
        }
    }
    dir.close();
    return true;
}


// ---------- Install Task ----------

struct InstallTaskParams {
    char tarRelName[128];
    int otaIndex; // 1..4
};

static void installTask(void *param) {
  pocketmage::setCpuSpeed(240);

  InstallTaskParams *p = (InstallTaskParams *)param;
  g_installProgress = 0;
  g_installDone = false;
  g_installFailed = false;

  String tarPath = pathJoin(APP_DIRECTORY, p->tarRelName);

  // --- Check TAR exists ---
  if (!global_fs->exists(tarPath.c_str())) {
    Serial.printf("Tar not found: %s\n", tarPath.c_str());
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  // --- Ensure directories ---
  if (!ensureDir(*global_fs, APP_DIRECTORY) ||
    !ensureDir(*global_fs, TEMP_DIR)) {
    Serial.println("Failed to prepare TEMP_DIR");
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  // --- TAR extraction ---
  TarUnpacker unpacker;
  unpacker.haltOnError(true);
  unpacker.setTarProgressCallback([](uint8_t progress) {
    uint8_t mappedProgress = progress / 2; // 0–50% for extraction
    
    // Never let the bar go backwards.
    if (mappedProgress > g_installProgress) {
        g_installProgress = mappedProgress;
    }
  });

  if (!unpacker.tarExpander(*global_fs, tarPath.c_str(), *global_fs, TEMP_DIR)) {
    Serial.printf("Extraction failed (err=%d)\n", unpacker.tarGzGetError());

    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  g_installProgress = 50; // halfway

// --- Determine main .bin and base name ---
String binPath = "";
String base = "";
  // --- Determine icon path ---
String iconPath = "";
String expectedIcon = base + "_ICON.bin";

File tempRoot = global_fs->open(TEMP_DIR);
if (tempRoot && tempRoot.isDirectory()) {
    File entry;
    String expectedIcon; // will be set once base is known
    while ((entry = tempRoot.openNextFile())) {
        String name = String(entry.name());

        // Skip macOS metadata files
        if (name.startsWith("._")) {
            entry.close();
            continue;
        }

        // --- Main .bin ---
        if (binPath.length() == 0 && name.endsWith(".bin") && !name.endsWith("_ICON.bin")) {
            binPath = pathJoin(TEMP_DIR, name);

            // Derive base from main .bin
            int dot = name.lastIndexOf('.');
            if (dot > 0) base = name.substring(0, dot);

            // Expected icon for later
            expectedIcon = base + "_ICON.bin";

            entry.close();
            continue; // continue to check icon in same loop
        }

        // --- Icon file ---
        if (iconPath.length() == 0 && expectedIcon.length() > 0 && name.equalsIgnoreCase(expectedIcon)) {
            iconPath = pathJoin(TEMP_DIR, name);
            entry.close();
            continue;
        }
        entry.close();
    }
    tempRoot.close();
}

if (binPath.length() == 0 || base.length() == 0) {
    Serial.printf("Bin not found after extraction in %s\n", TEMP_DIR);
    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
}

Serial.printf("App base name determined: '%s'\n", base.c_str());


// Wait up to ~200 ms for SD_MMC to see the files
int waitMs = 0;
while (waitMs < 200) {
    File tempRoot = global_fs->open(TEMP_DIR);
    bool found = false;
    if (tempRoot && tempRoot.isDirectory()) {
        File entry;
        while ((entry = tempRoot.openNextFile())) {
            String name = String(entry.name());

            // Skip macOS metadata files
            if (name.startsWith("._")) {
                entry.close();
                continue;
            }

            if (name.endsWith(".bin") && !name.endsWith("_ICON.bin")) {
                binPath = pathJoin(TEMP_DIR, name);
                found = true;
            }
            entry.close();
            if (found) break;
        }
        tempRoot.close();
    }
    if (found && global_fs->exists(binPath.c_str())) break;

    vTaskDelay(10 / portTICK_PERIOD_MS);
    waitMs += 10;
}

Serial.println("Listing /apps/temp:");
tempRoot = global_fs->open(TEMP_DIR);
if (tempRoot && tempRoot.isDirectory()) {
    File entry;
    while ((entry = tempRoot.openNextFile())) {
        Serial.printf("  %s%s\n", entry.name(), entry.isDirectory() ? "/" : "");
        entry.close();
    }
    tempRoot.close();
}

if (binPath.length() == 0 || !global_fs->exists(binPath.c_str())) {
    Serial.printf("Bin not found after extraction: %s\n", binPath.c_str());
    cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
}

vTaskDelay(pdMS_TO_TICKS(100)); // Safe wait

// --- Copy assets folder ---
String assetsSrc = pathJoin(TEMP_DIR, "assets");
String assetsDst = pathJoin("/assets", base);   // <-- correct target path

if (global_fs->exists(assetsSrc.c_str())) {
    rmRF(*global_fs, assetsDst.c_str()); // clean old assets
    Serial.printf("Copying assets: %s -> %s\n", assetsSrc.c_str(), assetsDst.c_str());
    if (!copyAssetsFlat(*global_fs, assetsSrc.c_str(), assetsDst.c_str())) {
        Serial.println("Failed to copy assets!");
        g_installFailed = true;
        g_installDone = true;
        cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
            if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
        delete p;
        vTaskDelete(NULL);
    }
}


  // --- OTA flashing ---
  const esp_partition_t *partition = esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + p->otaIndex),
    nullptr);

  if (!partition) {
    Serial.printf("OTA_%d partition not found\n", p->otaIndex);

    cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  File f = global_fs->open(binPath, "r");
  if (!f) {
    Serial.printf("Failed to open: %s\n", binPath.c_str());

    cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  uint32_t sz = f.size();
  Serial.printf("Flashing %s (%u bytes) -> OTA_%d @ 0x%08x\n",
          binPath.c_str(), sz, p->otaIndex, partition->address);

  esp_ota_handle_t ota_handle;
  esp_err_t err = esp_ota_begin(partition, sz, &ota_handle);
  if (err != ESP_OK) {
    Serial.printf("esp_ota_begin failed: %s\n", esp_err_to_name(err));
    f.close();
    
    cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    g_installFailed = true;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
  }

  uint8_t buf[4096];
  uint32_t written = 0;
  while (f.available()) {
    size_t rd = f.read(buf, sizeof(buf));
    err = esp_ota_write(ota_handle, buf, rd);
    if (err != ESP_OK) {
      Serial.printf("esp_ota_write failed: %s\n", esp_err_to_name(err));
      esp_ota_abort(ota_handle);
      f.close();

      cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
        if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

      g_installFailed = true;
      g_installDone = true;
      delete p;
      vTaskDelete(NULL);
    }
    written += rd;
    g_installProgress = 50 + (written * 50 / sz); // 50–100% flashing
  }

  f.close();
  err = esp_ota_end(ota_handle);
  if (err != ESP_OK) {
    Serial.printf("esp_ota_end failed: %s\n", esp_err_to_name(err));
    g_installFailed = true;
  } else {
    Serial.println("Flash OK");


if (iconPath.length() == 0) {
    Serial.printf("Icon not found for app '%s'\n", base.c_str());
} else {
    Serial.printf("Icon found: %s\n", iconPath.c_str());
}



    // --- Save AppInfo ---
    AppInfo info = {};
    strncpy(info.name, base.c_str(), sizeof(info.name)-1);
    strncpy(info.tarPath, tarPath.c_str(), sizeof(info.tarPath)-1);
    strncpy(info.iconPath, iconPath.c_str(), sizeof(info.iconPath)-1);

    if (!saveAppInfo(p->otaIndex, info)) {
      Serial.printf("Failed to save AppInfo for OTA_%d\n", p->otaIndex);
    }
  }

  cleanupAppsTempRecursive(*global_fs, TEMP_DIR);
  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  g_installProgress = 100;
  g_installDone = true;

  delete p;
  vTaskDelete(NULL);
}

// ---------- Async API ----------
bool installAppTarToOtaAsync(const char *tarRelName, int otaIndex) {
    auto *params = new InstallTaskParams;
    strncpy(params->tarRelName, tarRelName, sizeof(params->tarRelName) - 1);
    params->tarRelName[sizeof(params->tarRelName) - 1] = '\0';
    params->otaIndex = otaIndex;

    BaseType_t res = xTaskCreate(
        installTask,
        "installTask",
        12288, // stack size (increase if extraction is large)
        params,
        1,
        NULL
    );

    if (res != pdPASS) {
        Serial.println("Failed to create install task");
        delete params;
        return false;
    }
    return true;
}

// ---------- Helpers ----------
bool setBootToOtaSlot(int otaIndex /*1..4*/) {
  if (otaIndex < 1 || otaIndex > 4) return false;
  const esp_partition_t *partition =
      esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + otaIndex),
                                nullptr);
  if (!partition) return false;
  esp_err_t err = esp_ota_set_boot_partition(partition);
  if (err != ESP_OK) {
    Serial.printf("esp_ota_set_boot_partition failed: %d\n", (int)err);
    return false;
  }
  return true;
}

void rebootToAppSlot(int otaIndex) {
  if (!setBootToOtaSlot(otaIndex)) {
    Serial.printf("Failed to set OTA_%d as boot partition\n", otaIndex);
    return;
  }
  Serial.printf("Rebooting to OTA_%d...\n", otaIndex);
  delay(100); // allow Serial to flush
  esp_restart(); // immediate reboot
}

String getInstalledAppForOta(int otaIndex) {
  if (otaIndex < 1 || otaIndex > 4) return String();
  prefs.begin("PocketMage", true); // read-only
  String app = prefs.getString((String("OTA") + otaIndex).c_str(), "");
  prefs.end();
  return app;
}

void drawProgressBar(uint8_t progress) {
  uint progressPx = map(progress, 0, 100, 1, 216);

  u8g2.clearBuffer();
  // Draw rounded rectangle border
  u8g2.drawRFrame(20, 3, 216, 16, 5);
  // Draw progress bar
  if (progressPx > 10) u8g2.drawRBox(20, 3, progressPx, 16, 5);

  // Show text
  String progressText = "";
  if (progress < 52) progressText = "Extracting";
  else               progressText = "Installing";
  u8g2.setFont(u8g2_font_7x13B_tf);
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(progressText.c_str()))/2,
               u8g2.getDisplayHeight()-3,progressText.c_str());

  u8g2.sendBuffer();
}

// ---------- Operations ----------

void APPLOADER_INIT() {
  currentLine = "";
  CurrentAppState = APPLOADER;
  CurrentAppLoaderState = MENU;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

void processKB_APPLOADER() {
  int currentMillis = millis();
  String outPath = "";
  char inchar = 0;

  switch (CurrentAppLoaderState) {
    case MENU:
      // 1. Drain the hardware buffer continuously at loop speed
      inchar = KB().updateKeypress();

      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          // HANDLE INPUTS
          if (inchar == 12 || inchar == 23) { // Home or App Switcher Kill Signal
            HOME_INIT();
          }
          else if (inchar == 'a' || inchar == 'A' || inchar == '1') {
            selectedSlot = 1;
            CurrentAppLoaderState = SWAP_OR_EDIT;
            KB().setKeyboardState(NORMAL);
          }
          else if (inchar == 'b' || inchar == 'B' || inchar == '2') {
            selectedSlot = 2;
            CurrentAppLoaderState = SWAP_OR_EDIT;
            KB().setKeyboardState(NORMAL);
          }
          else if (inchar == 'c' || inchar == 'C' || inchar == '3') {
            selectedSlot = 3;
            CurrentAppLoaderState = SWAP_OR_EDIT;
            KB().setKeyboardState(NORMAL);
          }
          else if (inchar == 'd' || inchar == 'D' || inchar == '4') {
            selectedSlot = 4;
            CurrentAppLoaderState = SWAP_OR_EDIT;
            KB().setKeyboardState(NORMAL);
          }
          // All other keys are ignored in the MENU state
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        OLED().oledWord("Choose slot: (A)(B)(C)(D)");
      }
      break;

    case SWAP_OR_EDIT:
      // 1. Drain the hardware buffer continuously at loop speed
      inchar = KB().updateKeypress();

      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          if (inchar == 12 || inchar == 23) { // Home or App Switcher Kill Signal
            selectedSlot = 0;
            CurrentAppLoaderState = MENU;
            newState = true; // Trigger e-ink redraw of the menu
          }
          else if (inchar == 'S' || inchar == 's' || inchar == '!') {
            CurrentAppLoaderState = SWAP;
          }
          else if (inchar == 'D' || inchar == 'd' || inchar == '$') {
            // Clear the slot
            prefs.begin("PocketMage", false);
            prefs.remove(("OTA" + String(selectedSlot)).c_str());
            prefs.end();

            const esp_partition_t *partition =
              esp_partition_find_first(ESP_PARTITION_TYPE_APP,
              (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + selectedSlot),
              nullptr);

            if (partition) {
              esp_err_t err = esp_partition_erase_range(partition, 0, partition->size);
              if (err == ESP_OK) {
                Serial.printf("OTA_%d erased\n", selectedSlot);
              }
            }

            OLED().sysMessage("App removed", 2000);

            newState = true;
            CurrentAppLoaderState = MENU;
          }
        }
      }
        
      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        OLED().oledWord("(S)wap app or (D)elete app");
      }
      break;

    case SWAP:
      outPath = fileWizardMini(false, APP_DIRECTORY, 0);
      
      // Catch standard exits as well as the App Switcher 23 kill signal string
      if (outPath == "_EXIT_" || outPath == "_APP_SWITCH_" || outPath == "_RETURN_") {
        // Return to menu
        CurrentAppLoaderState = MENU;
        newState = true;
        break;
      }
      else if (outPath != "") {
        // Reject MacOS metadata files disguised as tar files
        if (outPath.indexOf("/._") != -1) {
            OLED().sysMessage("Cannot install MacOS metadata files!", 2000);
            CurrentAppLoaderState = MENU;
            newState = true; // Force background redraw just in case
            break;
        }

        // Ensure file is a .tar
        if (outPath.endsWith(".tar") || outPath.endsWith(".TAR")) {
          // Strip leading APP_DIRECTORY + '/' so installer gets relative path
          String relName = outPath;
          if (relName.startsWith(APP_DIRECTORY "/")) {
            relName = relName.substring(strlen(APP_DIRECTORY) + 1);
          }

          Serial.printf("Installing app from %s (rel=%s) into OTA_%d\n",
                        outPath.c_str(), relName.c_str(), selectedSlot);

          installAppTarToOtaAsync(relName.c_str(), selectedSlot);
          CurrentAppLoaderState = INSTALLING;
        } else {
          OLED().sysMessage("Not a .tar file!", 2000);
          CurrentAppLoaderState = MENU;
          newState = true;
        }
      }
      break;

    case INSTALLING:
      // Update OLED progress
      if (!g_installDone) {
        drawProgressBar(g_installProgress);
      } else {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (g_installFailed) {
          OLED().sysMessage("Install Failed!", 2000);
        } 
        else {
          OLED().sysMessage("Install Complete!", 2000);
        }
        newState = true;
        CurrentAppLoaderState = MENU;
      }
      break;
  }
}

void einkHandler_APPLOADER() {
  switch (CurrentAppLoaderState) {
    case MENU:
      if (newState) {
        newState = false;
        EINK().resetDisplay(false);
        display.drawBitmap(0, 0, _appLoader, 320, 218, GxEPD_BLACK);

        loadAndDrawAppIcon(42 , 146, 1, true, 7);  // OTA1
        loadAndDrawAppIcon(106, 146, 2, true, 7);  // OTA2
        loadAndDrawAppIcon(174, 146, 3, true, 7);  // OTA3
        loadAndDrawAppIcon(238, 146, 4, true, 7);  // OTA4

        EINK().drawStatusBar("Type Letter A-D:");

        //EINK().multiPassRefresh(2);
        EINK().refresh();
      }
      break;
  }
}
#endif