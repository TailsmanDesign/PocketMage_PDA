#include <pocketmage.h>
#include <ESP32-targz.h>
#include <Update.h>
#include "esp_ota_ops.h"

#define APP_DIRECTORY   "/apps"
#define TEMP_DIR        "/apps/temp"
#define PREFS_NAMESPACE "AppLoader"

static String currentLine = "";

enum AppLoaderState {MENU, SWAP_OR_EDIT, INSTALLING, EDIT};
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

static String basenameNoExt(const String &path, const char *ext = ".tar") {
    int slash = path.lastIndexOf('/');
    String name = (slash >= 0) ? path.substring(slash + 1) : path;
    if (name.endsWith(ext)) return name.substring(0, name.length() - (int)strlen(ext));
    return name;
}

// ---------- Install Task ----------
struct InstallTaskParams {
    const char *tarRelName;
    int otaIndex; // 1..4
};

static void installTask(void *param) {
    InstallTaskParams *p = (InstallTaskParams *)param;
    g_installProgress = 0;
    g_installDone = false;
    g_installFailed = false;

    String tarPath = String(APP_DIRECTORY) + "/" + p->tarRelName;

    // --- Check TAR exists ---
    if (!SD_MMC.exists(tarPath.c_str())) {
        Serial.printf("Tar not found: %s\n", tarPath.c_str());
        g_installFailed = true;
        g_installDone = true;
        delete p;
        vTaskDelete(NULL);
    }

    // --- Ensure directories ---
    if (!ensureDir(SD_MMC, APP_DIRECTORY) ||
        !rmRF(SD_MMC, TEMP_DIR) ||
        !ensureDir(SD_MMC, TEMP_DIR)) {
        Serial.println("Failed to prepare TEMP_DIR");
        g_installFailed = true;
        g_installDone = true;
        delete p;
        vTaskDelete(NULL);
    }

    // --- TAR extraction ---
    TarUnpacker unpacker;
    unpacker.haltOnError(true);
    unpacker.setTarProgressCallback([](uint8_t progress){
        g_installProgress = progress / 2; // 0-50% for extraction
    });

    if (!unpacker.tarExpander(SD_MMC, tarPath.c_str(), SD_MMC, TEMP_DIR)) {
        Serial.printf("Extraction failed (err=%d)\n", unpacker.tarGzGetError());
        g_installFailed = true;
        g_installDone = true;
        delete p;
        vTaskDelete(NULL);
    }

    g_installProgress = 50; // halfway

    // --- Prepare BIN path ---
    String base = basenameNoExt(p->tarRelName, ".tar");
    String binPath = String(TEMP_DIR) + "/" + base + ".bin";

    if (!SD_MMC.exists(binPath.c_str())) {
        Serial.printf("Bin not found after extraction: %s\n", binPath.c_str());
        g_installFailed = true;
        g_installDone = true;
        delete p;
        vTaskDelete(NULL);
    }

    // --- OTA flashing ---
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + p->otaIndex),
        nullptr
    );

    if (!partition) {
        Serial.printf("OTA_%d partition not found\n", p->otaIndex);
        g_installFailed = true;
        g_installDone = true;
        delete p;
        vTaskDelete(NULL);
    }

    File f = SD_MMC.open(binPath, "r");
    if (!f) {
        Serial.printf("Failed to open: %s\n", binPath.c_str());
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
            g_installFailed = true;
            g_installDone = true;
            delete p;
            vTaskDelete(NULL);
        }
        written += rd;
        g_installProgress = 50 + (written * 50 / sz); // 50-100% flashing
    }

    f.close();
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_ota_end failed: %s\n", esp_err_to_name(err));
        g_installFailed = true;
    } else {
        Serial.println("Flash OK");
        OLED().oledWord("App Install Complete!");
    }

    if (err == ESP_OK) {
      Serial.println("Flash OK");
      OLED().oledWord("App Install Complete!");

      // Save the installed app to Preferences
      prefs.begin("PocketMage", false); // RW
      prefs.putString((String("OTA") + p->otaIndex).c_str(), p->tarRelName);
      prefs.end();
    }

    // --- Cleanup ---
    rmRF(SD_MMC, TEMP_DIR);

    g_installProgress = 100;
    g_installDone = true;
    delete p;
    vTaskDelete(NULL);
}

// ---------- Async API ----------
bool installAppTarToOtaAsync(const char *tarRelName, int otaIndex) {
    auto *params = new InstallTaskParams{tarRelName, otaIndex};

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

String getInstalledAppForOta(int otaIndex) {
  if (otaIndex < 1 || otaIndex > 4) return String();
  prefs.begin("PocketMage", true); // read-only
  String app = prefs.getString((String("OTA") + otaIndex).c_str(), "");
  prefs.end();
  return app;
}

// ---------- Example usage in main loop ----------
/*
void loop() {
    // Start install once
    if (!installStarted) {
        installAppTarToOtaAsync("MyCoolApp.tar", 2);
        installStarted = true;
    }

    // Update OLED progress
    if (!g_installDone) {
        //drawProgressBar(g_installProgress);
    } else {
        if (g_installFailed) {
            OLED().oledWord("Install failed!");
        } else {
            OLED().oledWord("Install complete!");
        }
    }

    delay(50);
}
*/

void APPLOADER_INIT() {
  currentLine = "";
  CurrentAppState = APPLOADER;
  CurrentAppLoaderState = MENU;
  CurrentKBState  = NORMAL;
  newState = true;
}

void processKB_APPLOADER() {
  int currentMillis = millis();

  switch (CurrentAppLoaderState) {
    case MENU:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          currentLine.toLowerCase();
          if (currentLine == "a") {
            // edit a
            selectedSlot = 1;
          }
          else if (currentLine == "b") {
            // edit b
            selectedSlot = 2;
          }
          else if (currentLine == "c") {
            // edit c
            selectedSlot = 3;
          }
          else if (currentLine == "d") {
            // edit d
            selectedSlot = 4;
          }
          CurrentAppLoaderState = SWAP_OR_EDIT;
          CurrentKBState = NORMAL;

          currentLine = "";
        }                                      
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          currentLine += " ";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // Home recieved
        else if (inchar == 12) {
          HOME_INIT();
        }
        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, false);
        }
      }
      break;
    case SWAP_OR_EDIT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        // Swap


        // Delete
        
        
        // Home recieved
        else if (inchar == 12) {
          selectedSlot = 0;
          CurrentAppLoaderState = MENU;
          currentLine = "";
        }
        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, false);
        }
      }
      break;
  }
}

void einkHandler_APPLOADER() {
  switch (CurrentAppLoaderState) {
    case MENU:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.drawBitmap(0, 0, _appLoader, 320, 218, GxEPD_BLACK);

        EINK().drawStatusBar("Type Letter A-D:");

        EINK().multiPassRefresh(2);
      }
      break;
  }
}