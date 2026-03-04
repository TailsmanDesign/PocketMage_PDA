#include <globals.h>
static constexpr const char* TAG = "UTILS";

static uint8_t prevSec = 0;

void printDebug() {
  DateTime now = CLOCK().nowDT();
  if (now.second() != prevSec) {
    prevSec = now.second();
    float batteryVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;

    // Display GPIO states and system info
    ESP_LOGD(
        TAG, "PWR_BTN: %d, KB_INT: %d, CHRG: %d, RTC_INT: %d, BAT: %.2f, CPU_FRQ: %.1f, FFU: %d",
        digitalRead(PWR_BTN), digitalRead(KB_IRQ), digitalRead(CHRG_SENS), digitalRead(RTC_INT),
        batteryVoltage, (float)getCpuFrequencyMhz(), (int)GxEPD2_310_GDEQ031T10::useFastFullUpdate);

    // Display system time
    ESP_LOGD(TAG, "SYSTEM_CLOCK: %d/%d/%d (%s) %d:%d:%d", now.month(), now.day(), now.year(),
             daysOfTheWeek[now.dayOfTheWeek()], now.hour(), now.minute(), now.second());
  }
}

void checkTimeout() {
  int randomScreenSaver = 0;
  CLOCK().setTimeoutMillis(millis());
  ESP_LOGV(TAG, "checking timeout");
  // Trigger timeout deep sleep
  if (!disableTimeout) {
    if (CLOCK().getTimeDiff() >= TIMEOUT * 1000) {
      ESP_LOGD(TAG, "Device idle... Deep sleeping");

      // Give a chance to keep device awake
      OLED().oledWord("  Going to sleep!  ");
      int i = millis();
      int j = millis();
      while ((j - i) <= 4000) {  // 4 sec
        j = millis();
        if (digitalRead(KB_IRQ) == 0) {
          OLED().oledWord("Good Save!");
          delay(500);
          CLOCK().setPrevTimeMillis(millis());
          keypad.flush();
          return;
        }
      }
// OTA_APP: Remove saveEditingFile
// Save current work
#if !OTA_APP
      saveEditingFile();
#else
      // user skipped reboot flag if true, return to OS normally
      if (!pocketmage::setRebootFlagOTA()) {
        return;
      }
      display.setFullWindow();
#endif

      switch (CurrentAppState) {
// OTA_APP skip TXT case
#if !OTA_APP
        case TXT:
          if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "" && !OTA_APP) {
            /*
            EINK().setFullRefreshAfter(FULL_REFRESH_AFTER + 1);
            display.setFullWindow();
            EINK().einkTextDynamic(true, true);

            display.setFont(&FreeMonoBold9pt7b);

            display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
            display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
            display.setCursor(4, display.height() - 6);
            //display.drawBitmap(display.width() - 30, display.height() - 20, KBStatusallArray[6],
            30,
            //                20, GxEPD_BLACK);
            EINK().statusBar(editingFile, true);

            display.fillRect(320 - 86, 240 - 52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320 - 86, 240 - 52, sleep1, 87, 52, GxEPD_BLACK);

            // Put device to sleep with alternate sleep screen
            */
            pocketmage::deepSleep(true);
          } else
            pocketmage::deepSleep();
          break;
#endif
        default:
          pocketmage::deepSleep();
          break;
      }
    }
  } else {
    CLOCK().setPrevTimeMillis(millis());
  }

  // Power Button Event sleep
  if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
    PWR_BTN_event = false;
    ESP_LOGE(TAG, "Power Button Event: Sleeping now");
// OTA_APP: Remove saveEditingFile
// Save current work
#if !OTA_APP
    saveEditingFile();
#endif

    if (digitalRead(CHRG_SENS) == HIGH && !OTA_APP) {
      // Save last state

      prefs.begin("PocketMage", false);
      prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
      prefs.putString("editingFile", PM_SDAUTO().getEditingFile());
      prefs.end();

      CurrentAppState = HOME;
      CurrentHOMEState = NOWLATER;
// OTA_APP: remove updateTaskArray and sortTasksByDueDate
#if !OTA_APP
      updateTaskArray();
      sortTasksByDueDate(tasks);
#endif
      OLED().setPowerSave(true);
      disableTimeout = true;
      newState = true;

      // Shutdown Jingle
      BZ().playJingle(Jingles::Shutdown);

      // Clear screen
      display.setFullWindow();
      display.fillScreen(GxEPD_WHITE);

    } else {
      ESP_LOGD(TAG, "Not charging");
      switch (CurrentAppState) {
        // OTA_APP skip TXT case
        case TXT:
          if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "" && !OTA_APP) {
            ESP_LOGE(TAG, "text sleep mode");
            EINK().setFullRefreshAfter(FULL_REFRESH_AFTER + 1);
            display.setFullWindow();
            EINK().einkTextDynamic(true, true);
            display.setFont(&FreeMonoBold9pt7b);

            display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
            display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
            display.setCursor(4, display.height() - 6);
            // display.drawBitmap(display.width() - 30, display.height() - 20, KBStatusallArray[6],
            // 30,
            //                 20, GxEPD_BLACK);
            EINK().statusBar(PM_SDAUTO().getEditingFile(), true);

            display.fillRect(320 - 86, 240 - 52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320 - 86, 240 - 52, sleep1, 87, 52, GxEPD_BLACK);

            pocketmage::deepSleep(true);
          }
          // Sleep device normally
          else
            pocketmage::deepSleep();
          break;
        default:
          pocketmage::deepSleep();
          break;
      }
    }

  } else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    ESP_LOGE(TAG, "In NOWLATER state, returning home");
    // Load last state
    /*prefs.begin("PocketMage", true);
    SD().setEditingFile(prefs.getString("editingFile", "");
    if (HOME_ON_BOOT) CurrentAppState = HOME;
    else CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));
    prefs.end();*/
    loadState();
    keypad.flush();

    CurrentHOMEState = HOME_HOME;
    PWR_BTN_event = false;
    OLED().setPowerSave(false);
    display.fillScreen(GxEPD_WHITE);
    EINK().forceSlowFullUpdate(true);

    // Play startup jingle
    BZ().playJingle(Jingles::Startup);

    EINK().refresh();
    delay(200);
    newState = true;
  }
}

void loadState(bool changeState) {
  // LOAD PREFERENCES
  prefs.begin("PocketMage", true);  // Read-Only
  // Misc
  TIMEOUT = prefs.getInt("TIMEOUT", 120);
  DEBUG_VERBOSE = prefs.getBool("DEBUG_VERBOSE", true);
  SYSTEM_CLOCK = prefs.getBool("SYSTEM_CLOCK", true);
  SHOW_YEAR = prefs.getBool("SHOW_YEAR", true);
  SAVE_POWER = prefs.getBool("SAVE_POWER", true);
  ALLOW_NO_MICROSD = prefs.getBool("ALLOW_NO_SD", true);
  PM_SDAUTO().setEditingFile(prefs.getString("editingFile", ""));
  HOME_ON_BOOT = prefs.getBool("HOME_ON_BOOT", false);
  OLED_BRIGHTNESS = prefs.getInt("OLED_BRIGHTNESS", 255);
  OLED_MAX_FPS = prefs.getInt("OLED_MAX_FPS", 60);

  OTA1_APP = prefs.getString("OTA1", "-");
  OTA2_APP = prefs.getString("OTA2", "-");
  OTA3_APP = prefs.getString("OTA3", "-");
  OTA4_APP = prefs.getString("OTA4", "-");

  if (!changeState) {
    prefs.end();
    return;
  }

  u8g2.setContrast(OLED_BRIGHTNESS);

// OTA_APP: remove if statement
// Update State (if needed)
#if !OTA_APP  // POCKETMAGE_OS
  if (HOME_ON_BOOT) {
    CurrentAppState = HOME;
  } else {
    CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));

    // Check boot keypress
    KB().setKeyboardState(NORMAL);
    char inchar = KB().updateKeypress();
    switch (inchar) {
      case 'h':
        CurrentAppState = HOME;
        break;
      case 'u':
        CurrentAppState = USB_APP;
        break;
      case 'f':
        CurrentAppState = FILEWIZ;
        break;
      case 't':
        CurrentAppState = TASKS;
        break;
      case 'n':
        CurrentAppState = TXT;
        break;
      case 's':
        CurrentAppState = SETTINGS;
        break;
      case 'c':
        CurrentAppState = CALENDAR;
        break;
      case 'j':
        CurrentAppState = JOURNAL;
        break;
      case 'd':
        CurrentAppState = LEXICON;
        break;
      case 'x':
        CurrentAppState = TERMINAL;
        break;
      case 'l':
        CurrentAppState = APPLOADER;
        break;
      default:
        break;
    }

    keypad.flush();

    // Initialize boot app if needed
    switch (CurrentAppState) {
      case HOME:
        HOME_INIT();
        break;
      case TXT:
        TXT_INIT();  // TODO: Does not work? Crash on startup
        // HOME_INIT();
        break;
      case SETTINGS:
        SETTINGS_INIT();
        break;
      case TASKS:
        TASKS_INIT();
        break;
      case USB_APP:
        HOME_INIT();
        break;
      case CALENDAR:
        CALENDAR_INIT();
        break;
      case LEXICON:
        LEXICON_INIT();
        break;
      case JOURNAL:
        JOURNAL_INIT();
        break;
      case TERMINAL:
        TERMINAL_INIT();
        break;
      default:
        HOME_INIT();
        break;
    }
  }
#endif  // POCKETMAGE_OS
  prefs.end();
}

void updateBattState() {
  // Read and scale voltage (add calibration offset if needed)
  float rawVoltage = (analogRead(BAT_SENS) * (3.3 / 4095.0) * 2) + 0.2;

  // Moving average smoothing (adjust alpha for responsiveness)
  static float filteredVoltage = rawVoltage;
  const float alpha = 0.1;  // Low-pass filter constant (lower = smoother, slower response)
  filteredVoltage = alpha * rawVoltage + (1.0 - alpha) * filteredVoltage;

  static float prevVoltage = 0.0;
  static int prevBattState = -1;  // Ensure valid initial state
  const float threshold = 0.05;   // Hysteresis threshold

  int newState = battState;

  // Charging state overrides everything
  MP2722::MP2722_ChargeStatus chg;
  if (/*digitalRead(CHRG_SENS) == 1*/ PowerSystem.getChargeStatus(chg) &&
      (chg.code == 0b001 || chg.code == 0b010 || chg.code == 0b011 || chg.code == 0b100 ||
       chg.code == 0b101)) {
    newState = 5;
  } else {
    // Check for low battery
    bool low;
    if (!PowerSystem.isBatteryLow(low)) {
      if (low) {
        OLED().oledWord("Battery Critial!");
        delay(1000);

// OTA_APP: Remove saveEditingFile
#if !OTA_APP
        // Save current work
        saveEditingFile();
#endif
        // Put device to sleep
        pocketmage::deepSleep(false);
      }
    }

    // Normal battery voltage thresholds with hysteresis
    if (filteredVoltage > 4.1 || (prevBattState == 4 && filteredVoltage > 4.1 - threshold)) {
      newState = 4;
    } else if (filteredVoltage > 3.9 || (prevBattState == 3 && filteredVoltage > 3.9 - threshold)) {
      newState = 3;
    } else if (filteredVoltage > 3.8 || (prevBattState == 2 && filteredVoltage > 3.8 - threshold)) {
      newState = 2;
    } else if (filteredVoltage > 3.7 || (prevBattState == 1 && filteredVoltage > 3.7 - threshold)) {
      newState = 1;
    } else if (filteredVoltage <= 3.7) {
      newState = 0;
    }
  }

  if (newState != battState) {
    battState = newState;
    prevBattState = newState;
    // newState = true;
  }

  prevVoltage = filteredVoltage;
}

// Prompt the user for text input, return the text
String textPrompt(String promptText, String prefix) {
  String currentLine = "";
  int cursor_pos = 0;
  long lastInput = millis();

  for (;;) {
    // Run background tasks
    #if !OTA_APP // POCKETMAGE_OS
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
    #endif
    updateBattState();

    int currentMillis = millis();
    String left = "";
    String right = "";

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      char inchar = KB().updateKeypress();

      if (inchar != 0) lastInput = millis();

      // HANDLE INPUTS
      // No char recieved
      if (inchar == 0) ;
      // CR Recieved
      else if (inchar == 13) {
        cursor_pos = 0;
        break;
      }
      // SHIFT Recieved
      else if (inchar == 17) {
        if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) {
          KB().setKeyboardState(NORMAL);
        } else if (KB().getKeyboardState() == FUNC) {
          KB().setKeyboardState(FN_SHIFT);
        } else {
          KB().setKeyboardState(SHIFT);
        }
      }
      // FN Recieved
      else if (inchar == 18) {
        if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) {
          KB().setKeyboardState(NORMAL);
        } else if (KB().getKeyboardState() == SHIFT) {
          KB().setKeyboardState(FN_SHIFT);
        } else {
          KB().setKeyboardState(FUNC);
        }
      }
      // BKSP Recieved
      else if (inchar == 8) {
        if (currentLine.length() > 0 && cursor_pos != 0) {
          if (cursor_pos == currentLine.length()) {
            currentLine.remove(currentLine.length() - 1, 1);
          } else {
            currentLine.remove(cursor_pos - 1, 1);
          }
          cursor_pos--;
        }
      }
      // LEFT
      else if (inchar == 19) {
        if (cursor_pos > 0) {
          cursor_pos--;
        }
      }
      // RIGHT
      else if (inchar == 21) {
        if (cursor_pos < currentLine.length()) {
          cursor_pos++;
        }
      }
      // CENTER
      else if (inchar == 20) {
      }
      // SHIFT+LEFT
      else if (inchar == 28) {
        cursor_pos = 0;
        KB().setKeyboardState(NORMAL);
      }
      // SHIFT+RIGHT
      else if (inchar == 30) {
        cursor_pos = currentLine.length();
        KB().setKeyboardState(NORMAL);
      }
      // SHIFT+CENTER
      else if (inchar == 29) {
        KB().setKeyboardState(NORMAL);
      }
      // FN+LEFT
      else if (inchar == 12) {
        currentLine = "_EXIT_";
        break;
      }
      // FN+RIGHT
      else if (inchar == 6) {
        KB().setKeyboardState(NORMAL);
      }
      // FN+CENTER
      else if (inchar == 7) {
        currentLine = "";
        cursor_pos = 0;
        KB().setKeyboardState(NORMAL);
      }
      // FN+SHIFT+LEFT
      else if (inchar == 24) {
        KB().setKeyboardState(NORMAL);
      }
      // FN+SHIFT+RIGHT
      else if (inchar == 26) {
        KB().setKeyboardState(NORMAL);
      }
      // FN+SHIFT+CENTER
      else if (inchar == 25) {
        KB().setKeyboardState(NORMAL);
      }
      // TAB, SHIFT+TAB / FN+TAB, FN+SHIFT+TAB
      else if (inchar == 9 || inchar == 14) {
        KB().setKeyboardState(NORMAL);
      } else {
        // split line at cursor_pos
        if (cursor_pos == 0) {
          currentLine = inchar + currentLine;
        } else if (cursor_pos == currentLine.length()) {
          currentLine += inchar;
        } else {
          left = currentLine.substring(0, cursor_pos);
          right = currentLine.substring(cursor_pos);
          currentLine = left + inchar + right;
        }
        cursor_pos++;
        if (inchar >= 48 && inchar <= 57) {
        }  // Only leave FN on if typing numbers
        else if (KB().getKeyboardState() != NORMAL) {
          KB().setKeyboardState(NORMAL);
        }
      }

      currentMillis = millis();
      // Make sure oled only updates at OLED_MAX_FPS
      if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;

        if (millis() - lastInput > IDLE_TIME) {
          mageIdle(true);
        } else {
          resetIdle();
          if (prefix != "") OLED().oledLine(prefix + currentLine, cursor_pos+prefix.length(), false, promptText);
          else OLED().oledLine(currentLine, cursor_pos, false, promptText);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }

  return currentLine;
}

// OTA_APP: Remove definition of saveEditingFile
#if !OTA_APP
void saveEditingFile() {
  if (!OTA_APP) {
    OLED().oledWord("Saving Work");
    // pocketmage::file::saveFile();
    String savePath = PM_SDAUTO().getEditingFile();
    if (savePath != "" && savePath != "-" && savePath != "/temp.txt" && fileLoaded) {
      if (!savePath.startsWith("/"))
        savePath = "/" + savePath;
      ESP_LOGE(TAG, "Saving MarkdownFile");
      saveMarkdownFile(PM_SDAUTO().getEditingFile());
      ESP_LOGE(TAG, "Done saving MarkdownFile");
    }
  }
}
#endif