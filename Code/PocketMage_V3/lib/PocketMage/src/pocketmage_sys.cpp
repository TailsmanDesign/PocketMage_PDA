//  888888ba                    dP                  dP                                          //
//  88    `8b                   88                  88                                          //
// a88aaaa8P' .d8888b. .d8888b. 88  .dP  .d8888b. d8888P 88d8b.d8b. .d8888b. .d8888b. .d8888b.  //
//  88        88'  `88 88'  `"" 88888"   88ooood8   88   88'`88'`88 88'  `88 88'  `88 88ooood8  //
//  88        88.  .88 88.  ... 88  `8b. 88.  ...   88   88  88  88 88.  .88 88.  .88 88.  ...  //
//  dP        `88888P' `88888P' dP   `YP `88888P'   dP   dP  dP  dP `88888P8 `8888P88 `88888P'  //
//                                                                                .88           //
//                                                                            d8888P            //

#include <Preferences.h>
#include <RTClib.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPI.h>
#include <config.h>
#include <esp_log.h>
#include <globals.h>

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"

static constexpr const char* TAG = "SYSTEM";

volatile bool PWR_BTN_event = false;
bool noTimeout = false;  // Disable timeout
bool mscEnabled = false;
bool sinkEnabled = false;
volatile bool SDActive = false;
volatile int battState = 0;  // Bary state

///////////////////////////////////////////////////////////////////////////////
//            Use this function in apps to return to PocketMage OS           //
bool rebootToPocketMage() {
  const esp_partition_t* partition =
      esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                               ESP_PARTITION_SUBTYPE_APP_OTA_0,  // instead of FACTORY
                               nullptr);
  if (!partition) {
    Serial.println("OTA0 partition not found");
    return false;
  }

  esp_err_t err = esp_ota_set_boot_partition(partition);
  if (err != ESP_OK) {
    Serial.printf("esp_ota_set_boot_partition failed: %d\n", (int)err);
    return false;
  }

  Serial.println("Boot partition set to OTA0 (PocketMage OS). Restarting...");
  esp_restart();
  return true;
}
///////////////////////////////////////////////////////////////////////////////

namespace pocketmage {
void setCpuSpeed(int newFreq) {
  // Return early if the frequency is already set
  if (getCpuFrequencyMhz() == newFreq)
    return;

  int validFreqs[] = {240, 160, 80, 40, 20, 10};
  bool isValid = false;

  for (int i = 0; i < sizeof(validFreqs) / sizeof(validFreqs[0]); i++) {
    if (newFreq == validFreqs[i]) {
      isValid = true;
      break;
    }
  }

  if (isValid) {
    setCpuFrequencyMhz(newFreq);
    ESP_LOGV(TAG, "CPU Speed changed to: %d MHz", newFreq);
  }
}

void deepSleep(bool alternateScreenSaver) {
  // Put OLED to sleep
  u8g2.setPowerSave(1);

  // Stop the einkHandler task
  if (einkHandlerTaskHandle != NULL) {
    vTaskDelete(einkHandlerTaskHandle);
    einkHandlerTaskHandle = NULL;
  }

  // Shutdown Jingle
  BZ().playJingle(Jingles::Shutdown);

  if (alternateScreenSaver == false) {
    SDActive = true;
    pocketmage::setCpuSpeed(240);
    delay(50);

    // Check if there are custom screensavers
    File dir = global_fs->open("/assets/backgrounds");
    std::vector<String> binFiles;

    if (dir) {
      File file;
      while ((file = dir.openNextFile())) {
        String name = file.name();
        if (name.endsWith(".bin"))
          binFiles.push_back(name);
        file.close();
      }
      dir.close();
    }

    display.setFullWindow();

    // Use custom screensavers
    if (!binFiles.empty()) {
      int fileIndex = esp_random() % binFiles.size();
      String path = "/assets/backgrounds/" + binFiles[fileIndex];
      File f = global_fs->open(path);
      if (f) {
        static uint8_t buf[320 * 240];  // Declare as static to avoid stack overflow :D
        f.read(buf, sizeof(buf));
        f.close();

        // Show file
        display.drawBitmap(0, 0, buf, 320, 240, GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(5, display.height() - 5);
        display.print(binFiles[fileIndex].c_str());
      }
    }
    // Use standard screensavers
    else {
      int numScreensavers = sizeof(ScreenSaver_allArray) / sizeof(ScreenSaver_allArray[0]);
      int randomScreenSaver_ = esp_random() % numScreensavers;

      display.drawBitmap(0, 0, ScreenSaver_allArray[randomScreenSaver_], 320, 240, GxEPD_BLACK);
    }

    if (SAVE_POWER)
      pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    SDActive = false;

    EINK().multiPassRefresh(2);
  } else {
    // Display alternate screensaver
    EINK().forceSlowFullUpdate(true);
    EINK().refresh();
    delay(100);
  }
  // essential to display next app correctly
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  // Put E-Ink to sleep
  display.hibernate();

  // Save last state
  prefs.begin("PocketMage", false);
  prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
  prefs.putString("editingFile", PM_SDAUTO().getEditingFile());
  prefs.putBool("Seamless_Reboot", false);
  prefs.end();
  // Sleep the ESP32
  esp_deep_sleep_start();
}

// returns true if reboot flag set, false if skipped by user
bool setRebootFlagOTA() {
  if (OTA_APP) {
    ESP_LOGE(TAG, "Entering OTA reboot mode");
    OLED().oledWord("WARNING: Rebooting to PocketMage OS!");
    PWR_BTN_event = false;
    int i = millis();
    int j = millis();
    while ((j - i) <= 3000) {  // 3 sec
      // exit immediately if power button pressed again
      if (PWR_BTN_event) {
        ESP_LOGE(TAG, "Exiting setReboot continuing to returning true");
        PWR_BTN_event = false;
        break;
      }
      j = millis();
      if (digitalRead(KB_IRQ) == 0) {
        OLED().oledWord("Good Save!");
        delay(500);
        CLOCK().setPrevTimeMillis(millis());
        keypad.flush();
        return false;
      }
    }
    // timed out of loop, set reboot flag
    ESP_LOGE(TAG, "setting reboot flag for OTA");
    prefs.begin("PocketMage", false);
    prefs.putBool("OTA_Reboot", true);
    prefs.end();
    return true;
  }
  // PocketMage OS, no reboot flag needed
  ESP_LOGE(TAG, "Running in PocketMage OS, no reboot needed");
  return true;
}

// checks if reboot flag is set, clears flag and reboots to PocketMage OS
void checkRebootOTA() {
  if (OTA_APP) {
    ESP_LOGE(TAG, "Checking OTA reboot flag");
    prefs.begin("PocketMage", false);
    if (prefs.getBool("OTA_Reboot", false) == true) {
      prefs.putBool("OTA_Reboot", false);
      prefs.end();
      rebootToPocketMage();
    }
    prefs.end();
  }
  ESP_LOGE(TAG, "In pocketmageOS, skipping Checking OTA reboot flag");
}

void IRAM_ATTR PWR_BTN_irq() {
  PWR_BTN_event = true;
}

// Hard reset to home
void hardReset(void* parameter) {
  vTaskDelay(pdMS_TO_TICKS(250));
  static long heldSince = millis();
  for (;;) {
    if (digitalRead(PWR_BTN) == HIGH) {
      heldSince = millis();
    }

    // Hold power button for 3s to return home
    if ((millis() - heldSince) > 3000) {
      OLED().oledWord("Process broken");
      delay(1000);
      HOME_INIT();
    }
  }
}
}

void PocketMage_INIT() {
  // Check if in OTA app
  pocketmage::checkRebootOTA();

  // Check if seamless restart
  ESP_LOGE(TAG, "Checking OTA reboot flag");
  bool seamlessReboot = false;
  prefs.begin("PocketMage", false);
  if (prefs.getBool("Seamless_Reboot", false) == true) {
    seamlessReboot = true;
    prefs.putBool("Seamless_Reboot", false);
  }
  prefs.end();

  // Serial, I2C, SPIA
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  vspi = new SPIClass(FSPI /*VSPI*/);
  vspi->begin(SPI_SCK, -1, SPI_MOSI, -1);
  pinMode(vspi->pinSS(), OUTPUT);

  // WAKE INTERRUPT SETUP
  pinMode(KB_IRQ, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);
  ESP_LOGE(TAG, "set wakeup pin");

  // OLED SETUP
  setupOled();

  // SHOW "PocketMage" while DEVICE BOOTS
  if (!OTA_APP && !seamlessReboot)
    OLED().oledWord("   PocketMage   ", true, false);

  // KEYBOARD SETUP
  setupKB(KB_IRQ);
  ESP_LOGD(TAG, "setup keyboard");
  // EINK HANDLER SETUP
  setupEink();
  ESP_LOGD(TAG, "setup eink");

  // SD CARD SETUP
  setupSD();
  ESP_LOGD(TAG, "setup sd");

  // POWER SETUP
  pinMode(PWR_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PWR_BTN), pocketmage::PWR_BTN_irq, FALLING);
  pinMode(CHRG_SENS, INPUT);
  pinMode(BAT_SENS, INPUT);
  if (!PowerSystem.init(I2C_SDA, I2C_SCL)) {
    ESP_LOGV(TAG, "MP2722 Failed to Init");
  }
  // Start hardreset task
  xTaskCreatePinnedToCore(pocketmage::hardReset,    // Function name
                          "hardReset",              // Task name
                          1024,                     // Stack size
                          NULL,                     // Parameters
                          0,                        // Priority
                          &einkHandlerTaskHandle,   // Task handle
                          1                         // Core ID
  );

  // SET CPU CLOCK FOR POWER SAVE MODE
  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  else
    pocketmage::setCpuSpeed(240);

  // CAPACATIVE TOUCH SETUP
  setupTouch();
  ESP_LOGD(TAG, "setup touch");
  // RTC SETUP
  setupClock();
  ESP_LOGD(TAG, "setup clock");
  // Set "random" seed
  randomSeed(analogRead(BAT_SENS));

  // Load State
  loadState();
  ESP_LOGD(TAG, "loaded state");

  // STARTUP JINGLE
  setupBZ();
  ESP_LOGD(TAG, "setup buzzer");
  if (!seamlessReboot)
    BZ().playJingle(Jingles::Startup);
}

// ===================== GLOBAL TEXT HELPERS =====================
volatile bool newLineAdded = true;  // New line added in TXT
std::vector<String> allLines;       // All lines in TXT

String vectorToString() {
  String result;
  EINK().setTXTFont(EINK().getCurrentFont());

  for (size_t i = 0; i < allLines.size(); i++) {
    result += allLines[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(allLines[i], 0, 0, &x1, &y1, &charWidth, &charHeight);

    // Add newline only if the line doesn't fully use the available space
    if (charWidth < display.width() && i < allLines.size() - 1) {
      result += '\n';
    }
  }

  return result;
}

void stringToVector(String inputText) {
  EINK().setTXTFont(EINK().getCurrentFont());
  allLines.clear();
  String currentLine_;

  for (size_t i = 0; i < inputText.length(); i++) {
    char c = inputText[i];

    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    display.getTextBounds(currentLine_, 0, 0, &x1, &y1, &charWidth, &charHeight);

    // Check if new line needed
    if ((c == '\n' || charWidth >= display.width() - 5) && !currentLine_.isEmpty()) {
      if (currentLine_.endsWith(" ")) {
        allLines.push_back(currentLine_);
        currentLine_ = "";
      } else {
        int lastSpace = currentLine_.lastIndexOf(' ');
        if (lastSpace != -1) {
          // Split line at last space
          String partialWord = currentLine_.substring(lastSpace + 1);
          currentLine_ = currentLine_.substring(0, lastSpace);
          allLines.push_back(currentLine_);
          currentLine_ = partialWord;  // Start new line with partial word
        } else {
          // No spaces, whole line is a single word
          allLines.push_back(currentLine_);
          currentLine_ = "";
        }
      }
    }

    if (c != '\n') {
      currentLine_ += c;
    }
  }

  // Push last line if not empty
  if (!currentLine_.isEmpty()) {
    allLines.push_back(currentLine_);
  }
}

String removeChar(String str, char character) {
  String result = "";
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != character) {
      result += str[i];
    }
  }
  return result;
}

int stringToInt(String str) {
  str.trim();  // Remove leading/trailing whitespace

  if (str.length() == 0)
    return -1;

  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      return -1;  // Invalid character found
    }
  }

  return str.toInt();  // Safe to convert
}