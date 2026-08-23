#include <globals.h>
static constexpr const char* TAG = "UTILS";

static uint8_t prevSec = 0;

void printDebug() {
  DateTime now = CLOCK().nowDT();
  if (now.second() != prevSec) {
    prevSec = now.second();
    float batteryVoltage = getBatteryVoltage();

    // Display GPIO states and system info
    ESP_LOGD(
        TAG, "PWR_BTN: %d, KB_INT: %d, CHRG: %d, RTC_INT: %d, BAT: %.2f, CPU_FRQ: %.1f, FFU: %d",
        digitalRead(PWR_BTN), digitalRead(KB_IRQ), digitalRead(CHRG_SENS), digitalRead(RTC_INT),
        batteryVoltage, (float)getCpuFrequencyMhz(), (int)GxEPD2_310_GDEQ031T10::useFastFullUpdate);

    // Display system time
    ESP_LOGD(TAG, "SYSTEM_CLOCK: %d/%d/%d (%s) %d:%d:%d", now.month(), now.day(), now.year(),
             I18n::dayName(now.dayOfTheWeek()), now.hour(), now.minute(), now.second());
  }
}

#if OTA_APP
void checkTimeout() {
  int randomScreenSaver = 0;
  CLOCK().setTimeoutMillis(millis());
  ESP_LOGV(TAG, "checking timeout");

  // Trigger timeout deep sleep
  if (!disableTimeout) {
    if (CLOCK().getTimeDiff() >= TIMEOUT * 1000) {
      ESP_LOGD(TAG, "Device idle... Deep sleeping");

      // Give a chance to keep device awake
      OLED().oledWord(TR(STR_UTILS_SLEEP));
      unsigned long i = millis();
      unsigned long j = millis();
      while ((j - i) <= 4000) {  // 4 sec
        j = millis();
        if (KB().updateKeypress() != 0) {
          OLED().oledWord(TR(STR_GOOD_SAVE));
          delay(500);
          CLOCK().setPrevTimeMillis(millis()); // Reset system idle timer
          keypad.flush();
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent watchdog starvation during this while loop
      }

      // user skipped reboot flag if true, return to OS normally
      if (!pocketmage::setRebootFlagOTA()) {
        return;
      }
      display.setFullWindow();
      pocketmage::deepSleep();
    }
  } else {
    CLOCK().setPrevTimeMillis(millis());
  }

  // Power Button Event sleep
  if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
    PWR_BTN_event = false;
    ESP_LOGE(TAG, "Power Button Event: Sleeping now");
    pocketmage::deepSleep();

  } else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    ESP_LOGE(TAG, "Power Button Event: powering off from NOWLATER");
    PWR_BTN_event = false;
    pocketmage::deepSleep();
  }
}

#else // OS APPLICATION
void checkTimeout() {
  int randomScreenSaver = 0;
  CLOCK().setTimeoutMillis(millis());
  ESP_LOGV(TAG, "checking timeout");

  // Trigger timeout deep sleep
  if (!disableTimeout) {
    if (CLOCK().getTimeDiff() >= TIMEOUT * 1000) {
      ESP_LOGD(TAG, "Device idle... Deep sleeping");

      // Give a chance to keep device awake
      OLED().oledWord(TR(STR_UTILS_SLEEP));
      unsigned long i = millis();
      unsigned long j = millis();
      while ((j - i) <= 4000) {  // 4 sec
        j = millis();
        if (KB().updateKeypress() != 0) {
          OLED().oledWord(TR(STR_GOOD_SAVE));
          delay(500);
          CLOCK().setPrevTimeMillis(millis()); // Reset system idle timer
          keypad.flush();
          return;
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent watchdog starvation during this while loop
      }

      saveEditingFile();

      switch (CurrentAppState) {
        case TXT:
          if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "") {
            pocketmage::deepSleep(true);
          } else {
            pocketmage::deepSleep();
          }
          break;
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

    saveEditingFile();

    if (digitalRead(CHRG_SENS) == HIGH) {
      // Save last state
      prefs.begin("PocketMage", false);
      prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
      prefs.putString("editingFile", PM_SDAUTO().getEditingFile());
      prefs.end();

      CurrentAppState = HOME;
      CurrentHOMEState = NOWLATER;

      updateTaskArray();
      sortTasksByDueDate(tasks);
      
      u8g2.clearBuffer();
      OLED().oledWord(" ");
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
        case TXT:
          if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "") {
            ESP_LOGE(TAG, "text sleep mode");
            EINK().setFullRefreshAfter(FULL_REFRESH_AFTER + 1);
            display.setFullWindow();

            display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
            display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
            
            EINK().statusBar(PM_SDAUTO().getEditingFile(), true);

            display.fillRect(320 - 86, 240 - 52, 87, 52, GxEPD_WHITE);
            display.drawBitmap(320 - 86, 240 - 52, sleep1, 87, 52, GxEPD_BLACK);

            pocketmage::deepSleep(true);
          }
          // Sleep device normally
          else {
            pocketmage::deepSleep();
          }
          break;
        default:
          pocketmage::deepSleep();
          break;
      }
    }

  } else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
    ESP_LOGE(TAG, "Power Button Event: powering off from NOWLATER");
    PWR_BTN_event = false;
    pocketmage::deepSleep();
  }
}

// Exit the NOWLATER (charging shutdown) screen back to home, triggered by any
// keypad key.  bootKey carries the shortcut letter pressed to wake (reused by
// loadState so "press a letter while off" launches the app from NOWLATER too).
// Re-locks via loadState() when a PIN is configured.
void wakeFromNowlater(char bootKey) {
  loadState(true, bootKey);
  keypad.flush();
  disableTimeout = false;

  CurrentHOMEState = HOME_HOME;
  OLED().setPowerSave(false);

  // Play startup jingle
  BZ().playJingle(Jingles::Startup);
  newState = true;

  // While locked, keep the NOWLATER frame on the panel so the PIN prompt does
  // not appear over a blank flash; the home screen draws after unlocking.
  if (deviceLocked) return;

  display.fillScreen(GxEPD_WHITE);
  EINK().refresh();
  delay(200);
}

// Boot shortcut letters (pressed while off, or as the NOWLATER wake key) map
// to an app; any other key keeps the currently loaded app.
// TODO: This should be i18n-ed?
AppState bootShortcutApp(char inchar) {
  switch (inchar) {
    case 'h': return HOME;
    case 'u': return USB_APP;
    case 'f': return FILEWIZ;
    case 't': return TASKS;
    case 'n': return TXT;
    case 's': return SETTINGS;
    case 'c': return CALENDAR;
    case 'j': return JOURNAL;
    case 'd': return LEXICON;
    case 'x': return TERMINAL;
    case 'l': return APPLOADER;
    default:  return CurrentAppState;
  }
}

#endif

void loadState(bool changeState, char bootKey) {
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
  FAST_REFRESH = prefs.getBool("FAST_REFRESH", POCKETMAGE_HW_VERSION == 1);
  OLED_BRIGHTNESS = prefs.getInt("OLED_BRIGHTNESS", 255);
  OLED_MAX_FPS = prefs.getInt("OLED_MAX_FPS", 60);
  MUTE_BUZZER = prefs.getBool("MUTE_BUZZER", false);
  I18n::setLanguage(static_cast<Lang>(prefs.getInt("Language", static_cast<int>(Lang::English))));

  OTA1_APP = prefs.getString("OTA1", "-");
  OTA2_APP = prefs.getString("OTA2", "-");
  OTA3_APP = prefs.getString("OTA3", "-");
  OTA4_APP = prefs.getString("OTA4", "-");

  // Every boot/wake starts locked when a PIN is configured
  #if !OTA_APP  // POCKETMAGE_OS
  deviceLocked = prefs.getBool("LOCK_ENABLED", false);
  #endif  // POCKETMAGE_OS

  if (!changeState) {
    prefs.end();
    return;
  }

  u8g2.setContrast(OLED_BRIGHTNESS);

#if !OTA_APP  // POCKETMAGE_OS
  if (!prefs.getBool("Onboarded", false)) {
    // First boot (or NVS wipe): the setup wizard takes priority over any
    // saved app, boot shortcut, or HOME_ON_BOOT preference.
    CurrentAppState = ONBOARDING;
    ONBOARDING_INIT();
  } else if (HOME_ON_BOOT) {
    CurrentAppState = HOME;
    HOME_INIT();
  } else {
    CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));

    // Check boot keypress; a key captured by the NOWLATER wake path is passed
    // in so the shortcut letter opens its app without re-reading the keypad.
    char inchar = bootKey;
    if (inchar == 0) {
      KB().setKeyboardState(NORMAL);
      inchar = KB().updateKeypress();
    }
    CurrentAppState = bootShortcutApp(inchar);

    keypad.flush();

    // Initialize boot app if needed
    switch (CurrentAppState) {
      case HOME:      HOME_INIT(); break;
      case TXT:       TXT_INIT(); break; 
      case FILEWIZ:   FILEWIZ_INIT(); break;
      case SETTINGS:  SETTINGS_INIT(); break;
      case TASKS:     TASKS_INIT(); break;
      case USB_APP:   HOME_INIT(); break;
      case COMM:      COMM_INIT(); break;
      case CALENDAR:  CALENDAR_INIT(); break;
      case LEXICON:   LEXICON_INIT(); break;
      case JOURNAL:   JOURNAL_INIT(); break;
      case TERMINAL:  TERMINAL_INIT(); break;
      case APPLOADER: APPLOADER_INIT(); break;
      default:        HOME_INIT(); break;
    }
  }
#endif  // POCKETMAGE_OS
  prefs.end();
}

void updateBattState() {
  float rawVoltage = getBatteryVoltage();

  // Moving average smoothing
  static float filteredVoltage = rawVoltage;
  const float alpha = 0.1;  
  filteredVoltage = alpha * rawVoltage + (1.0 - alpha) * filteredVoltage;

  static float prevVoltage = 0.0;
  static int prevBattState = -1;  
  const float threshold = 0.05;   

  int newState = battState;

  // Charging state overrides everything
  MP2722::MP2722_ChargeStatus chg;
  if (PowerSystem.getChargeStatus(chg) &&
      (chg.code == 0b001 || chg.code == 0b010 || chg.code == 0b011 || chg.code == 0b100 ||
       chg.code == 0b101)) {
    newState = 5;
  } else {
    // Check for low battery
    bool low;
    if (PowerSystem.isBatteryLow(low)) {
      if (low) {
        OLED().sysMessage(TR(STR_UTILS_BATT_CRITICAL),1000);

#if !OTA_APP
        saveEditingFile();
#endif
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
  }

  prevVoltage = filteredVoltage;
}

#pragma region Basic Inputs
// Prompt the user for text input, return the text
#if !OTA_APP // PocketMage OS Only
String textPrompt(String promptText, String prefix, bool mask, bool lockGlyph) {
  String currentLine = "";
  int cursor_pos = 0;
  long lastInput = CLOCK().getPrevTimeMillis(); // Sync to system idle
  bool redraw = true; 

  for (;;) {
    #if !OTA_APP 
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
      if (CurrentHOMEState == NOWLATER) return "_RETURN_";
    #endif
    
    updateBattState();
    KB().checkUSBKB();

    static int lastBattState = -1;
    if (battState != lastBattState) {
      redraw = true;
      lastBattState = battState;
    }

    int currentMillis = millis();
    String left = "";
    String right = "";

    unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
    if (currentSystemTime > lastInput) {
        lastInput = currentSystemTime;
        redraw = true;
    }

    char inchar = KB().updateKeypress();

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      if (inchar != 0) {
        lastInput = millis();
        KBBounceMillis = currentMillis; 
        redraw = true;

        // HANDLE INPUTS
        if (inchar == 23) {
          currentLine = "_RETURN_";
          break;
        }
        else if (inchar == 13) {
          cursor_pos = 0;
          break;
        }
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) {
            KB().setKeyboardState(NORMAL);
          } else if (KB().getKeyboardState() == FUNC) {
            KB().setKeyboardState(FN_SHIFT);
          } else {
            KB().setKeyboardState(SHIFT);
          }
        }
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) {
            KB().setKeyboardState(NORMAL);
          } else if (KB().getKeyboardState() == SHIFT) {
            KB().setKeyboardState(FN_SHIFT);
          } else {
            KB().setKeyboardState(FUNC);
          }
        }
        else if (inchar == 8) {
          if (currentLine.length() > 0 && cursor_pos != 0) {
            int old_cursor = cursor_pos;
            do { cursor_pos--; } while (cursor_pos > 0 && (currentLine[cursor_pos] & 0xC0) == 0x80);
            int bytesToDelete = old_cursor - cursor_pos;
            currentLine.remove(cursor_pos, bytesToDelete);
          }
        }
        else if (inchar == 19) {
          if (cursor_pos > 0) {
            do { cursor_pos--; } while (cursor_pos > 0 && (currentLine[cursor_pos] & 0xC0) == 0x80);
          }
        }
        else if (inchar == 21) {
          if (cursor_pos < currentLine.length()) {
            do { cursor_pos++; } while (cursor_pos < currentLine.length() && (currentLine[cursor_pos] & 0xC0) == 0x80);
          }
        }
        else if (inchar == 20) {
          currentLine = "_CENTER_";
          break;
        }
        else if (inchar == 28) {
          cursor_pos = 0;
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 30) {
          cursor_pos = currentLine.length();
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 29) {
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 12) {
          currentLine = "_EXIT_";
          break;
        }
        else if (inchar == 6) {
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 7) {
          currentLine = "";
          cursor_pos = 0;
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 24) {
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 26) {
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 25) {
          KB().setKeyboardState(NORMAL);
        }
        else if (inchar == 9 || inchar == 14) {
          KB().setKeyboardState(NORMAL);
        } else {
          String chStr = String(inchar);
          if (cursor_pos == 0) {
            currentLine = chStr + currentLine;
          } else if (cursor_pos == currentLine.length()) {
            currentLine += chStr;
          } else {
            left = currentLine.substring(0, cursor_pos);
            right = currentLine.substring(cursor_pos);
            currentLine = left + chStr + right;
          }
          cursor_pos++;
          if (inchar >= 48 && inchar <= 57) {
          } 
          else if (KB().getKeyboardState() != NORMAL) {
            KB().setKeyboardState(NORMAL);
          }
        }
      }
    }

    // Handle idle state transitions
    bool isIdle = (millis() - lastInput > IDLE_TIME);
    static bool wasIdle = false;
    
    // If we just woke up from being idle, reset the mage and force a text redraw
    if (isIdle != wasIdle) {
       wasIdle = isIdle;
       if (!isIdle) {
           resetIdle();
           redraw = true; 
       }
    }

    // Display Update Loop (Runs at OLED_MAX_FPS)
    if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
      if (isIdle) {
        // Continuously update the idle animation frames while idle
        OLEDFPSMillis = currentMillis;
        // The lock prompt must stay on screen: never yield to the mage animation
        if (!lockGlyph) mageIdle(true);
      } 
      else if (redraw) {
        // Only redraw the text prompt if the user typed or moved the cursor
        OLEDFPSMillis = currentMillis;
        redraw = false;
        
        // Mask password input without losing cursor position
        String displayLine = currentLine;
        if (mask) {
          for (int i = 0; i < displayLine.length(); i++) displayLine[i] = '*';
        }
        // Lock screen: draw text + glyph into one frame and send a single buffer,
        // so the glyph never strobes against a separate text-only send
        if (lockGlyph) {
          OLED().oledLine(prefix + displayLine, cursor_pos + prefix.length(), false, promptText, true);
          u8g2.drawXBMP(u8g2.getDisplayWidth() - kOledLockGlyphX, kOledLockGlyphY,
                        kOledLockGlyphW, kOledLockGlyphH, _lockIcon);
          u8g2.sendBuffer();
        }
        else if (prefix != "") OLED().oledLine(prefix + displayLine, cursor_pos + prefix.length(), false, promptText);
        else OLED().oledLine(displayLine, cursor_pos, false, promptText);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }

  return currentLine;
}

int boolPrompt(String promptText) {
  KB().setKeyboardState(NORMAL);
  pocketmage::setCpuSpeed(240); // Boost clock for smooth animation

  String msg = promptText + TR(STR_UTILS_YN_SUFFIX);
  u8g2.clearBuffer();
  const uint16_t dw = u8g2.getDisplayWidth();
  const uint16_t dh = u8g2.getDisplayHeight();
  
  int y_offset = 0;
  int x_offset = 0;
  FontStyle activeStyle;

  int w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::OledWord);
  if (w < dw-8) {
    y_offset = 16 + 3 + 5;
    x_offset = (dw - w) / 2;
    activeStyle = FontStyle::OledWord;
  } 
  else {
    w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::Heading3);
    if (w < dw-8) {
      y_offset = 16 + 2 + 5;
      x_offset = (dw - w) / 2;
      activeStyle = FontStyle::Heading3;
    } 
    else {
      w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::BodyBold);
      if (w < dw-8) {
        y_offset = 16 + 1 + 5;
        x_offset = (dw - w) / 2;
        activeStyle = FontStyle::BodyBold;
      } 
      else {
        w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::Caption);
        if (w < dw-8) {
          y_offset = 16 + 5;
          x_offset = (dw - w) / 2;
        } 
        else {
          y_offset = 16 + 5;
          x_offset = dw - w;
        }
        activeStyle = FontStyle::Caption;
      }
    }
  }

  for (int y = dh; y > 0; y-=2) {
    u8g2.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, activeStyle);
    u8g2.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  unsigned long lastSystemTime = CLOCK().getPrevTimeMillis();
  int retVal = -1;

  for (;;) {
    #if !OTA_APP 
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
    #endif

    CLOCK().setPrevTimeMillis(millis());
    updateBattState();

    unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
    if (currentSystemTime > lastSystemTime) {
        lastSystemTime = currentSystemTime;
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, x_offset, y_offset, msg, activeStyle);
        u8g2.drawRFrame(0, 0, dw, dh + 16, 10);
        u8g2.sendBuffer();
    }

    int currentMillis = millis();
    char inchar = KB().updateKeypress();

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      if (inchar != 0) {
        KBBounceMillis = currentMillis; 
        
        if (inchar == 'y' || inchar == 'Y') {
          retVal = 1;
          break;
        }
        else if (inchar == 'n' || inchar == 'N') {
          retVal = 0;
          break;
        }
        else if (inchar == 23) {
          retVal = 0;
          break;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }

  pocketmage::setCpuSpeed(240);

  for (int y = 0; y <= dh; y+=2) {
    u8g2.clearBuffer();
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, activeStyle);
    u8g2.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  return retVal;
}

int timePrompt(int defaultTime) {
  uint8_t digits[4] = {0,0,0,0};
  ulong currentIndex = 0;

  // If a valid time is passed (e.g., 1430), extract it into the digits array
  if (defaultTime >= 0 && defaultTime <= 2359) {
    digits[0] = (defaultTime / 1000) % 10;
    digits[1] = (defaultTime / 100) % 10;
    digits[2] = (defaultTime / 10) % 10;
    digits[3] = defaultTime % 10;
  }

  // X coordinates for the 4 digits (HHMM)
  const int tX[4] = {93, 110, 131, 148};

  for (;;) {
    #if !OTA_APP 
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
    #endif

    // Update scroll
    int scrollVec = TOUCH().getScrollVector();
    if (scrollVec != 0) {
      
      if (currentIndex == 0) {
        // Isolate the Tens of Hours digit to prevent base-24 modulo bleeding
        int d0 = digits[0] + scrollVec;
        if (d0 > 2) d0 = 0;
        if (d0 < 0) d0 = 2;
        digits[0] = d0;
        
        // Reverse Clamp: If we wrapped to 2, ensure the hours digit isn't sitting at 4-9
        if (digits[0] == 2 && digits[1] > 3) {
           digits[1] = 3; 
        }
      } else {
        // Convert current digits to total minutes for smooth carry-over
        int total_mins = (digits[0] * 10 + digits[1]) * 60 + (digits[2] * 10 + digits[3]);

        // Apply the scroll vector based on cursor position
        switch (currentIndex) {
          case 1: total_mins += scrollVec * 60;  break; // +/- 1 hour
          case 2: total_mins += scrollVec * 10;  break; // +/- 10 minutes
          case 3: total_mins += scrollVec * 1;   break; // +/- 1 minute
        }

        // Wrap-around logic for 24-hour format (1440 minutes in a day)
        total_mins = total_mins % 1440;
        if (total_mins < 0) {
            total_mins += 1440; // Wrap backwards (e.g. 00:00 - 1 min = 23:59)
        }

        // Convert total minutes back to individual digits
        int h = total_mins / 60;
        int m = total_mins % 60;
        
        digits[0] = h / 10;
        digits[1] = h % 10;
        digits[2] = m / 10;
        digits[3] = m % 10;
      }
    }
    
    // Update system state
    updateBattState();
    KB().checkUSBKB();

    // Handle keyboard inputs
    KB().setKeyboardState(FUNC);
    
    int currentMillis = millis();
    char inchar = KB().updateKeypress();

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      if (inchar != 0) {
        KBBounceMillis = currentMillis;

        // Left arrow or bksp
        if (inchar == 12 || inchar == 8) {
          if (currentIndex > 0) {
            currentIndex--;
          } 
        }
        // Right arrow
        else if (inchar == 6) {
          if (currentIndex < 3) {
            currentIndex++;
          }
        }
        // Direct Numeric Entry (0-9)
        else if (inchar >= '0' && inchar <= '9') {
          int val = inchar - '0'; // Convert ASCII char to integer
          
          switch (currentIndex) {
            case 0: // Tens of Hours (Max 2)
              digits[0] = (val > 2) ? 2 : val;
              // Reverse Clamp: If we just set it to 2, ensure the hours digit isn't sitting at 4-9
              if (digits[0] == 2 && digits[1] > 3) {
                 digits[1] = 3; 
              }
              break;
              
            case 1: // Hours (Max 3 if Tens is 2, otherwise Max 9)
              if (digits[0] == 2) {
                digits[1] = (val > 3) ? 3 : val;
              } else {
                digits[1] = val;
              }
              break;
              
            case 2: // Tens of Minutes (Max 5)
              digits[2] = (val > 5) ? 5 : val;
              break;
              
            case 3: // Minutes (Max 9)
              digits[3] = val;
              break;
          }
          
          // Auto-advance cursor for natural typing flow
          if (currentIndex < 3) {
            currentIndex++;
          }
        }
        // Enter 
        else if (inchar == 13) {
          int returnInt = 0;
          returnInt += digits[3]*1;
          returnInt += digits[2]*10;
          returnInt += digits[1]*100;
          returnInt += digits[0]*1000;

          // Because of our modulo math and strict clamping, the digits can 
          // physically never exceed 23:59. But we clamp just in case.
          if (returnInt >= 2400) {
              returnInt = 0;
          }

          return returnInt;
        }
      }
    }

    // Draw interface
    u8g2.clearBuffer(); // Required so text doesn't smear endlessly

    // Draw background
    u8g2.drawXBMP(0,0,256,32,timeInput);

    // Draw indicator
    switch (currentIndex) {
      case 0:
        u8g2.drawXBMP(89,21,24,11,leftRightIndicator0);
        break;
      case 1:
        u8g2.drawXBMP(106,21,24,11,leftRightIndicator1);
        break;
      case 2:
        u8g2.drawXBMP(127,21,24,11,leftRightIndicator1);
        break;
      case 3:
        u8g2.drawXBMP(144,21,24,11,leftRightIndicator2);
        break;
    }

    // Draw digits dynamically with inverted active block
    for (int i = 0; i < 4; i++) {
      if (i == currentIndex) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(tX[i], 0, 15, 20); // Draw white background block
        u8g2.setDrawColor(0);               // Set text color to black
      } else {
        u8g2.setDrawColor(1);               // Standard white text
      }
      FontEngine::drawGlyph(DisplayTarget::OLED, tX[i], 16, '0' + digits[i], FontStyle::ClockDigit);
    }
    u8g2.setDrawColor(1); // Reset for next draw cycle

    u8g2.sendBuffer(); // Required to push the frame to the OLED

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }
}
#endif

// Helper function to calculate max days in a month (handles Leap Years)
static int getDaysInMonth(int month, int year) {
  if (month == 2) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28;
  }
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

#if !OTA_APP // PocketMage OS Only
String datePrompt(String defaultYYYYMMDD) {
  uint8_t digits[8] = {0,0,0,0,0,0,0,0};
  ulong currentIndex = 0;
  
  int d, m, y;
  
  if (defaultYYYYMMDD.length() == 8) {
    y = defaultYYYYMMDD.substring(0, 4).toInt();
    m = defaultYYYYMMDD.substring(4, 6).toInt();
    d = defaultYYYYMMDD.substring(6, 8).toInt();
  } else {
    DateTime now = CLOCK().nowDT();
    y = now.year();
    m = now.month();
    d = now.day();
  }

  digits[0] = d / 10;
  digits[1] = d % 10;
  digits[2] = m / 10;
  digits[3] = m % 10;
  digits[4] = y / 1000;
  digits[5] = (y / 100) % 10;
  digits[6] = (y / 10) % 10;
  digits[7] = y % 10;

  const int dX[8] = {57, 74, 96, 113, 135, 152, 168, 185};

  for (;;) {
    #if !OTA_APP 
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
    #endif

    int scrollVec = TOUCH().getScrollVector();
    if (scrollVec != 0) {
      
      int d = digits[0] * 10 + digits[1];
      int m = digits[2] * 10 + digits[3];
      int y = digits[4] * 1000 + digits[5] * 100 + digits[6] * 10 + digits[7];

      if (d == 0) d = 1;
      if (m == 0) m = 1;

      if (currentIndex == 0) {
        int d_tens = digits[0] + scrollVec;
        if (d_tens > 3) d_tens = 0;
        if (d_tens < 0) d_tens = 3;
        
        d = d_tens * 10 + digits[1];
        int maxDays = getDaysInMonth(m, y);
        if (d > maxDays) d = maxDays;
        if (d == 0) d = 1;
      } 
      else if (currentIndex == 1) {
        d += scrollVec;
        while (d > getDaysInMonth(m, y)) {
          d -= getDaysInMonth(m, y);
          m++;
          if (m > 12) { m = 1; y++; }
        }
        while (d < 1) {
          m--;
          if (m < 1) { m = 12; y--; }
          d += getDaysInMonth(m, y);
        }
      }
      else if (currentIndex == 2) {
        int m_tens = digits[2] + scrollVec;
        if (m_tens > 1) m_tens = 0;
        if (m_tens < 0) m_tens = 1;
        
        m = m_tens * 10 + digits[3];
        if (m > 12) m = 12;
        if (m == 0) m = 1;
        
        int maxDays = getDaysInMonth(m, y);
        if (d > maxDays) d = maxDays;
      }
      else if (currentIndex == 3) {
        m += scrollVec;
        while (m > 12) { m -= 12; y++; }
        while (m < 1) { m += 12; y--; }
        
        int maxDays = getDaysInMonth(m, y);
        if (d > maxDays) d = maxDays;
      }
      else {
        if (currentIndex == 4) y += scrollVec * 1000;
        if (currentIndex == 5) y += scrollVec * 100;
        if (currentIndex == 6) y += scrollVec * 10;
        if (currentIndex == 7) y += scrollVec * 1;
        
        if (y < 2000) y = 2000;
        if (y > 2199) y = 2199;
        
        int maxDays = getDaysInMonth(m, y);
        if (d > maxDays) d = maxDays;
      }

      digits[0] = d / 10;
      digits[1] = d % 10;
      digits[2] = m / 10;
      digits[3] = m % 10;
      digits[4] = y / 1000;
      digits[5] = (y / 100) % 10;
      digits[6] = (y / 10) % 10;
      digits[7] = y % 10;
    }
    
    updateBattState();
    KB().checkUSBKB();

    KB().setKeyboardState(FUNC);
    
    int currentMillis = millis();
    char inchar = KB().updateKeypress();

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      if (inchar != 0) {
        KBBounceMillis = currentMillis;

        if (inchar == 9 || inchar == 14) {
          DateTime now = CLOCK().nowDT();
          char dateBuf[11];
          snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", 
                   now.day(), now.month(), now.year());
          return String(dateBuf);
        }
        else if (inchar == 12 || inchar == 8) {
          if (currentIndex > 0) currentIndex--;
        }
        else if (inchar == 6) {
          if (currentIndex < 7) currentIndex++;
        }
        else if (inchar >= '0' && inchar <= '9') {
          int val = inchar - '0';
          digits[currentIndex] = val;

          int d = digits[0] * 10 + digits[1];
          int m = digits[2] * 10 + digits[3];
          int y = digits[4] * 1000 + digits[5] * 100 + digits[6] * 10 + digits[7];

          if (m > 12) m = 12;
          if (currentIndex > 1 && m == 0) m = 1; 

          int maxDays = getDaysInMonth(m == 0 ? 1 : m, y); 
          if (d > maxDays) d = maxDays;
          if (currentIndex <= 1 && d == 0 && currentIndex == 1) d = 1; 

          digits[0] = d / 10;
          digits[1] = d % 10;
          digits[2] = m / 10;
          digits[3] = m % 10;
          digits[4] = y / 1000;
          digits[5] = (y / 100) % 10;
          digits[6] = (y / 10) % 10;
          digits[7] = y % 10;

          if (currentIndex < 7) currentIndex++;
        }
        else if (inchar == 13) {
          char dateBuf[11];
          snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", 
                   digits[0]*10 + digits[1], 
                   digits[2]*10 + digits[3], 
                   digits[4]*1000 + digits[5]*100 + digits[6]*10 + digits[7]);
          
          return String(dateBuf);
        }
      }
    }

    u8g2.clearBuffer();

    u8g2.drawXBMP(0, 0, 256, 32, dateInput);

    const uint8_t* ind = leftRightIndicator1; 
    if (currentIndex == 0) ind = leftRightIndicator0;       
    else if (currentIndex == 7) ind = leftRightIndicator2;  
    
    u8g2.drawXBMP(dX[currentIndex] - 4, 21, 24, 11, ind);

    // Draw digits dynamically with inverted active block
    for (int i = 0; i < 8; i++) {
      if (i == currentIndex) {
        u8g2.setDrawColor(1);
        u8g2.drawBox(dX[i], 0, 15, 20); // Draw white background block
        u8g2.setDrawColor(0);               // Set text color to black
      } else {
        u8g2.setDrawColor(1);               // Standard white text
      }
      FontEngine::drawGlyph(DisplayTarget::OLED, dX[i], 16, '0' + digits[i], FontStyle::ClockDigit);
    }
    u8g2.setDrawColor(1); // Reset for next draw cycle

    u8g2.sendBuffer(); 

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }
}
#endif

void waitForKeypress(String message) {
  KB().setKeyboardState(NORMAL); 
  pocketmage::setCpuSpeed(240); // Boost clock for smooth animation

  String msg = message;
  String bottomMsg = TR(STR_UTILS_PRESS_KEY);
  
  u8g2.clearBuffer();
  const uint16_t dw = u8g2.getDisplayWidth();
  const uint16_t dh = u8g2.getDisplayHeight();
  
  int y_offset = 0;
  int x_offset = 0;
  FontStyle activeStyle;

  int w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::OledWord);
  if (w < dw) {
    y_offset = 16 + 3;
    x_offset = (dw - w) / 2;
    activeStyle = FontStyle::OledWord;
  } 
  else {
    w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::Heading3);
    if (w < dw) {
      y_offset = 16 + 2;
      x_offset = (dw - w) / 2;
      activeStyle = FontStyle::Heading3;
    } 
    else {
      w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::BodyBold);
      if (w < dw) {
        y_offset = 16 + 1;
        x_offset = (dw - w) / 2;
        activeStyle = FontStyle::BodyBold;
      } 
      else {
        w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::Caption);
        if (w < dw) {
          y_offset = 16;
          x_offset = (dw - w) / 2;
        } 
        else {
          y_offset = 16;
          x_offset = dw - w;
        }
        activeStyle = FontStyle::Caption;
      }
    }
  }

  for (int y = dh; y > 0; y-=2) {
    u8g2.clearBuffer();
    
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, activeStyle);
    FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2, y + dh - 2, bottomMsg, FontStyle::Tiny);
    
    u8g2.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  unsigned long lastSystemTime = CLOCK().getPrevTimeMillis();

  for (;;) {
    #if !OTA_APP 
      if (!noTimeout)  checkTimeout();
      if (DEBUG_VERBOSE) printDebug();
    #endif
    
    updateBattState();

    unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
    if (currentSystemTime > lastSystemTime) {
        lastSystemTime = currentSystemTime;
        
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, x_offset, y_offset, msg, activeStyle);
        FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2, dh - 2, bottomMsg, FontStyle::Tiny);
        
        u8g2.drawRFrame(0, 0, dw, dh + 16, 10);
        u8g2.sendBuffer();
    }

    int currentMillis = millis();
    char inchar = KB().updateKeypress();

    if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
      if (inchar != 0) {
        KBBounceMillis = currentMillis; 
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    yield();
  }

  pocketmage::setCpuSpeed(240);

  for (int y = 0; y <= dh; y+=2) {
    u8g2.clearBuffer();
    
    FontEngine::drawText(DisplayTarget::OLED, x_offset, y + y_offset, msg, activeStyle);
    FontEngine::drawText(DisplayTarget::OLED, (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2, y + dh - 2, bottomMsg, FontStyle::Tiny);
    
    u8g2.drawRFrame(0, y, dw, dh + 16, 10);
    u8g2.sendBuffer();
    delay(5);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void checkCrashState() {
  esp_reset_reason_t reset_reason = esp_reset_reason();

  if (reset_reason == ESP_RST_PANIC || 
      reset_reason == ESP_RST_WDT || 
      reset_reason == ESP_RST_TASK_WDT || 
      reset_reason == ESP_RST_INT_WDT) {
    
    String crashMsg = TR(STR_UTILS_CRASH_PREFIX);

    switch (reset_reason) {
      case ESP_RST_PANIC:    crashMsg += TR(STR_UTILS_CRASH_PANIC); break;
      case ESP_RST_WDT:      crashMsg += TR(STR_UTILS_CRASH_WDT); break;
      case ESP_RST_TASK_WDT: crashMsg += TR(STR_UTILS_CRASH_TASK_WDT); break;
      case ESP_RST_INT_WDT:  crashMsg += TR(STR_UTILS_CRASH_INT_WDT); break;
      default:               crashMsg += TR(STR_UTILS_CRASH_UNKNOWN); break;
    }

    int romReason = (int)esp_rom_get_reset_reason(0);
    crashMsg += TR(STR_UTILS_CRASH_CODE_OPEN) + String(romReason) + ")";

    prefs.begin("PocketMage", false);
    prefs.putInt("CurrentAppState", HOME);
    prefs.end();

    PM_SDAUTO().setEditingFile("");
    waitForKeypress(crashMsg);
  }
}

// Apply a user-entered date (DD/MM/YYYY) to the RTC, keeping the current
// time-of-day. Prefills the prompt with the current date.
// Returns false when the prompt was aborted.
bool applyDateFromPrompt() {
  DateTime now = CLOCK().nowDT();
  char defaultDate[9];
  snprintf(defaultDate, sizeof(defaultDate), "%04d%02d%02d", now.year(), now.month(), now.day());

  String newDateStr = datePrompt(String(defaultDate));
  if (newDateStr == "_EXIT_" || newDateStr == "_RETURN_" || newDateStr == "_CENTER_")
    return false;

  int d = newDateStr.substring(0, 2).toInt();
  int m = newDateStr.substring(3, 5).toInt();
  int y = newDateStr.substring(6, 10).toInt();
  CLOCK().getRTC().adjust(DateTime(y, m, d, now.hour(), now.minute(), now.second()));
  return true;
}

// Apply a user-entered time (HHMM) to the RTC. Prefills the prompt with the
// current time. Returns false when the prompt was aborted or invalid.
bool applyTimeFromPrompt() {
  DateTime now = CLOCK().nowDT();
  int defaultTime = (now.hour() * 100) + now.minute();

  int newTime = timePrompt(defaultTime);
  if (newTime < 0)
    return false;

  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", newTime / 100, newTime % 100);
  CLOCK().setTimeFromString(String(timeBuf));
  return true;
}

// Shared clock-setup flow: optional "set the clock?" question followed by the
// date and time prompts with current values prefilled. Used by the boot RTC
// power-loss path and by the onboarding wizard's Date & Time step.
void runClockSetupFlow(bool ask) {
#if !OTA_APP
  // Temporarily disable the sleep timeout so the setup prompts don't force a sleep loop
  bool previousTimeoutState = noTimeout;
  noTimeout = true;

  if (ask && !boolPrompt(TR(STR_UTILS_POWER_LOST))) {
    noTimeout = previousTimeoutState;
    return;
  }

  bool changed = applyDateFromPrompt();
  changed |= applyTimeFromPrompt();
  if (changed)
    OLED().sysMessage(TR(STR_UTILS_TIME_SET), 500);

  noTimeout = previousTimeoutState;
#endif
}

void checkRTCPowerLoss() {
  // Check if RTC lost power (e.g., coin cell drained/removed)
  bool lostClock = SET_CLOCK_ON_UPLOAD || CLOCK().getRTC().lostPower();

  prefs.begin("PocketMage", true);
  bool onboarded = prefs.getBool("Onboarded", false);
  prefs.end();

  if (!lostClock)
    return;

  // Always give the clock a sane baseline; adjust() clears the hardware
  // lostPower() flag.
  CLOCK().setToCompileTimeUTC();

  // The first-boot wizard owns clock setup before Onboarded is set (its
  // Date & Time step calls runClockSetupFlow(false)); don't double-prompt.
  if (onboarded)
    runClockSetupFlow(true);
}

#if !OTA_APP
void saveEditingFile() {
    OLED().oledWord(TR(STR_UTILS_SAVING_WORK));
    String savePath = PM_SDAUTO().getEditingFile();
    if (savePath != "" && savePath != "-" && savePath != "/temp.txt" && fileLoaded) {
      if (!savePath.startsWith("/"))
        savePath = "/" + savePath;
      ESP_LOGI(TAG, "Saving MarkdownFile");
      saveMarkdownFile(savePath);
      ESP_LOGI(TAG, "Done saving MarkdownFile");
    }
}
#endif