// AUDIT 1

#include <globals.h>
#include <vector>

#if !OTA_APP // POCKETMAGE_OS

enum class SetType { BOOLEAN, INTEGER, ACTION };

// Data-driven struct holding configuration for each setting
struct SettingItem {
  String key;              // Preference key name
  StringID nameID;         // Display name in UI
  SetType type;            // Boolean, Integer, or Action
  int* intVal;             // Pointer to global int variable
  bool* boolVal;           // Pointer to global bool variable
  int minVal;              // Min boundary for ints
  int maxVal;              // Max boundary for ints
  void (*onUpdate)();      // Callback executed after variable changes or for Action
};

// Callbacks for setting changes
void updateLumina() { u8g2.setContrast(OLED_BRIGHTNESS); }
void updateFastRef() { if (FAST_REFRESH) EINK().markPanelNeedsFullRefresh(); }
void updateBlank() {}

// Action Callbacks for specialized prompts
void actionSetTime() {
  if (!applyTimeFromPrompt()) return;
  DateTime now = CLOCK().nowDT();
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", now.hour(), now.minute());
  OLED().sysMessage(TR(STR_SETTINGS_TIME_UPDATED_TO) + String(timeBuf), 1000);
}

void actionSetDate() {
  if (!applyDateFromPrompt()) return;
  OLED().sysMessage(TR(STR_SETTINGS_DATE_UPDATED), 1000);
}

void actionSetPin() {
  String pin1 = textPrompt(TR(STR_LOCK_ENTER_PIN), "", true);
  if (pin1 == "_EXIT_" || pin1 == "_RETURN_" || pin1 == "_CENTER_") return;
  String pin2 = textPrompt(TR(STR_LOCK_CONFIRM_PIN), "", true);
  if (pin2 == "_EXIT_" || pin2 == "_RETURN_" || pin2 == "_CENTER_") return;
  if (!lockPinValid(pin1)) {
    OLED().sysMessage(TR(STR_LOCK_PIN_INVALID), 1000);
    return;
  }
  if (pin1 != pin2) {
    OLED().sysMessage(TR(STR_LOCK_MISMATCH), 1000);
    return;
  }
  lockSetPin(pin1);
  OLED().sysMessage(TR(STR_LOCK_PIN_SET), 1000);
}

void actionSetLang() {
  String langPart = textPrompt(TR(STR_SETTINGS_LANG_PROMPT));
  if (langPart != "_EXIT_" && langPart != "_RETURN_" && langPart != "_CENTER_") {
    langPart.trim();
    langPart.toLowerCase();
    if (I18n::setLanguageByCode(langPart.c_str())) {
      prefs.begin("PocketMage", false);
      prefs.putInt("Language", static_cast<int>(I18n::language()));
      prefs.end();
      OLED().sysMessage(String(TR(STR_TERM_LANG_SET)) + I18n::nativeName(), 1000);
    } else {
      OLED().sysMessage(TR(STR_TERM_HELP_LANG), 1000);
    }
  }
}

void actionOnboarding() {
  ONBOARDING_INIT();
}

// Core array defining the system settings
static std::vector<SettingItem> settingsList = {
  SettingItem{"OLED_BRIGHTNESS", STR_SETTINGS_NAME_BRIGHTNESS, SetType::INTEGER, &OLED_BRIGHTNESS, nullptr, 0, 255, updateLumina},
  SettingItem{"TIMEOUT", STR_SETTINGS_NAME_TIMEOUT, SetType::INTEGER, &TIMEOUT, nullptr, 15, 3600, updateBlank},
  SettingItem{"OLED_MAX_FPS", STR_SETTINGS_NAME_MAX_FPS, SetType::INTEGER, &OLED_MAX_FPS, nullptr, 5, 144, updateBlank},
  SettingItem{"MUTE_BUZZER", STR_SETTINGS_NAME_MUTE, SetType::BOOLEAN, nullptr, &MUTE_BUZZER, 0, 0, updateBlank},
  SettingItem{"SYSTEM_CLOCK", STR_SETTINGS_NAME_CLOCK, SetType::BOOLEAN, nullptr, &SYSTEM_CLOCK, 0, 0, updateBlank},
  SettingItem{"SHOW_YEAR", STR_SETTINGS_NAME_YEAR, SetType::BOOLEAN, nullptr, &SHOW_YEAR, 0, 0, updateBlank},
  SettingItem{"SAVE_POWER", STR_SETTINGS_NAME_POWER, SetType::BOOLEAN, nullptr, &SAVE_POWER, 0, 0, updateBlank},
  SettingItem{"FAST_REFRESH", STR_SETTINGS_NAME_FAST_REFRESH, SetType::BOOLEAN, nullptr, &FAST_REFRESH, 0, 0, updateFastRef},
  SettingItem{"DEBUG_VERBOSE", STR_SETTINGS_NAME_DEBUG, SetType::BOOLEAN, nullptr, &DEBUG_VERBOSE, 0, 0, updateBlank},
  SettingItem{"HOME_ON_BOOT", STR_SETTINGS_NAME_HOME_BOOT, SetType::BOOLEAN, nullptr, &HOME_ON_BOOT, 0, 0, updateBlank},
  SettingItem{"ALLOW_NO_SD", STR_SETTINGS_NAME_ALLOW_SD, SetType::BOOLEAN, nullptr, &ALLOW_NO_MICROSD, 0, 0, updateBlank},
  SettingItem{"LOCK_ENABLED", STR_SETTINGS_NAME_LOCK, SetType::BOOLEAN, nullptr, (bool*)&deviceLocked, 0, 0, updateBlank},
  SettingItem{"", STR_SETTINGS_NAME_SET_PIN, SetType::ACTION, nullptr, nullptr, 0, 0, actionSetPin},
  SettingItem{"", STR_SETTINGS_NAME_SET_TIME, SetType::ACTION, nullptr, nullptr, 0, 0, actionSetTime},
  SettingItem{"", STR_SETTINGS_NAME_SET_DATE, SetType::ACTION, nullptr, nullptr, 0, 0, actionSetDate},
  SettingItem{"", STR_SETTINGS_NAME_SET_LANG, SetType::ACTION, nullptr, nullptr, 0, 0, actionSetLang},
  SettingItem{"", STR_SETTINGS_NAME_ONBOARD, SetType::ACTION, nullptr, nullptr, 0, 0, actionOnboarding}
};

// Simplified state machine
enum SettingsState { SETTINGS_MAIN };
SettingsState CurrentSettingsState = SETTINGS_MAIN;

// State variables for UI Interaction
static ulong settingsScrollIndex = 0;

// Layout constants for the Settings UI Box
constexpr int SETTINGS_BOX_X      = 6;
constexpr int SETTINGS_BOX_Y      = 26;
constexpr int SETTINGS_BOX_W      = 206;
constexpr int SETTINGS_BOX_H      = 186;
constexpr int SETTINGS_TOG_W      = 26;
constexpr int SETTINGS_TOG_H      = 11;
constexpr int SETTINGS_ITEMS_PAGE = 9;
constexpr int SETTINGS_PITCH      = SETTINGS_BOX_H / SETTINGS_ITEMS_PAGE;

// Localized labels can overflow the fixed rows (German especially): clamp them
// to the box interior on the e-ink and clear of the OLED hint column.
constexpr int SETTINGS_NAME_MAX_W = (SETTINGS_BOX_X + SETTINGS_BOX_W) - (SETTINGS_BOX_X + 10) - 38 - 8; // valX + value gutter + right pad
constexpr int SETTINGS_HIGHLIGHT_MAX_W = (SETTINGS_BOX_X + SETTINGS_BOX_W) - ((SETTINGS_BOX_X + 10) - 2); // right edge - highlight start
constexpr int SETTINGS_OLED_NAME_MAX_W = 140 - kOledPrevX - 2; // clear of the hint column at x=140

void SETTINGS_INIT() {
  // OPEN SETTINGS
  CurrentAppState = SETTINGS;
  CurrentSettingsState = SETTINGS_MAIN;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

String settingCommandSelect(String command) {
  String returnText = "";
  command = I18n::normalizeCommand(command);
  command.toLowerCase();

  if (command.startsWith("timeset") || command.startsWith("settime")) {
    String timePart = "";
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      timePart = command.substring(spaceIdx + 1);
      timePart.trim();
    }
    if (timePart.length() == 0) {
      actionSetTime();
    } else if (timePart.length() >= 4) { 
      CLOCK().setTimeFromString(timePart);
      returnText = TR(STR_SETTINGS_TIME_UPDATED);
    } else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_HHMM);
    }
    return returnText;
  }
  else if (command.startsWith("mute ")) {
    String muteArg = command.substring(5);
    muteArg.trim();
    
    if (muteArg == "on" || muteArg == "t") {
      MUTE_BUZZER = true;
    } else if (muteArg == "off" || muteArg == "f") {
      MUTE_BUZZER = false;
    } else {
      return TR(STR_INVALID);
    }
    
    prefs.begin("PocketMage", false);
    prefs.putBool("MUTE_BUZZER", MUTE_BUZZER);
    prefs.end();
    
    newState = true;
    return TR(STR_SETTINGS_UPDATED);
  }
  else if (command == "lock" || command.startsWith("lock ")) {
    String lockArg = "";
    int lockSpaceIdx = command.indexOf(' ');
    if (lockSpaceIdx != -1) {
      lockArg = command.substring(lockSpaceIdx + 1);
      lockArg.trim();
      lockArg.toLowerCase();
    }
    if (lockArg.length() == 0) {
      return lockIsEnabled() ? TR(STR_LOCK_ENABLED) : TR(STR_LOCK_DISABLED);
    }
    else if (lockArg == "on") {
      if (!lockHasPin()) {
        actionSetPin();
      } 
      if (lockHasPin()) {
        prefs.begin("PocketMage", false);
        prefs.putBool("LOCK_ENABLED", true);
        prefs.end();
        deviceLocked = true;
        newState = true;
        return TR(STR_LOCK_ENABLED);
      }
      return "";
    }
    else if (lockArg == "off") {
      lockDisable();
      newState = true;
      return TR(STR_LOCK_DISABLED);
    } else {
      return TR(STR_LOCK_HELP);
    }
  }
  else if (command.startsWith("lockpin ")) {
    String pin = command.substring(8);
    pin.trim();
    if (!lockPinValid(pin)) return TR(STR_LOCK_PIN_INVALID);
    lockSetPin(pin);
    newState = true;
    return TR(STR_LOCK_PIN_SET);
  }
  else if (command.startsWith("dateset") || command.startsWith("setdate")) {
    String datePart = "";
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      datePart = command.substring(spaceIdx + 1);
      datePart.trim();
    }
    if (datePart.length() == 0) {
      actionSetDate();
    } else if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();
      static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
      int maxDay = dim[month - 1];
      if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) maxDay = 29;
      if (year < 1970 || year > 2200 || month < 1 || month > 12 || day < 1 || day > maxDay) {
        returnText = TR(STR_SETTINGS_INVALID_DATE);
      } else {
        DateTime now = CLOCK().nowDT();
        CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
        returnText = TR(STR_SETTINGS_DATE_UPDATED);
      }
    } else {
      returnText = TR(STR_SETTINGS_INVALID_FMT_YYYYMMDD);
    }
    return returnText;
  }
  else if (command.startsWith("lang ")) {
    String langPart = command.substring(5);
    langPart.trim();
    langPart.toLowerCase();
    if (!I18n::setLanguageByCode(langPart.c_str())) return TR(STR_TERM_HELP_LANG);
    prefs.begin("PocketMage", false);
    prefs.putInt("Language", static_cast<int>(I18n::language()));
    prefs.end();
    newState = true;
    return String(TR(STR_TERM_LANG_SET)) + I18n::nativeName();
  }
  else {
    return TR(STR_SETTINGS_HUH);
  }
}

void processKB_SETTINGS() {
  pocketmage::setCpuSpeed(240);
  char inchar = KB().updateKeypress();
  if (inchar == 0) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }

  // Update hardware touchpad scroll mapped natively to Settings Array
  int scrollStep = (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) ? 5 : 1;
  if (TOUCH().updateScroll(settingsList.size() - 1, settingsScrollIndex, scrollStep)) {
    newState = true;
  }

  int currentMillis = millis();

  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    if (inchar != 0) {
      KBBounceMillis = currentMillis;

      // Forceful App Exit (FN + <)
      if (inchar == 12) { 
        HOME_INIT();
        return;
      }
      // Enter or Center Click to change setting
      else if (inchar == 13 || inchar == 20) { 
        SettingItem& item = settingsList[settingsScrollIndex];
        
        if (item.type == SetType::BOOLEAN) {
          // Direct Toggle logic instead of boolPrompt
          bool nextState = !(*(item.boolVal));
          
          if (nextState) { // Turning ON
            if (item.key == "LOCK_ENABLED") {
              if (!lockHasPin()) {
                actionSetPin();
                if (!lockHasPin()) {
                  // User aborted PIN creation
                  KB().setKeyboardState(NORMAL);
                  newState = true;
                  return;
                }
              }
              *(item.boolVal) = true;
              deviceLocked = true;
            } else {
              *(item.boolVal) = true;
            }
          } 
          else { // Turning OFF
            if (item.key == "LOCK_ENABLED") {
              lockDisable();
              *(item.boolVal) = false;
              deviceLocked = false;
            } else {
              *(item.boolVal) = false;
            }
          }

          // Save the toggled value
          prefs.begin("PocketMage", false);
          prefs.putBool(item.key.c_str(), *(item.boolVal));
          prefs.end();
          if (item.onUpdate) item.onUpdate();
        } 
        else if (item.type == SetType::INTEGER) {
          String promptTxt = String(TR(item.nameID)) + " (" + String(item.minVal) + "-" + String(item.maxVal) + "):";
          String resStr = textPrompt(promptTxt);
          
          if (resStr != "_RETURN_" && resStr != "_EXIT_" && resStr != "_CENTER_") {
            int val = resStr.toInt();
            if (val >= item.minVal && val <= item.maxVal) {
              *(item.intVal) = val;
              
              prefs.begin("PocketMage", false);
              prefs.putInt(item.key.c_str(), *(item.intVal));
              prefs.end();
              if (item.onUpdate) item.onUpdate();
            } else {
              OLED().sysMessage(TR(STR_INVALID), 1000);
            }
          }
        }
        else if (item.type == SetType::ACTION) {
          if (item.onUpdate) item.onUpdate();
          // The action may have switched CurrentAppState (e.g. launching
          // onboarding); bail before the OLED preview below repaints over
          // the new app's splash.
          if (CurrentAppState != SETTINGS) return;
        }

        // Return gracefully and redraw menus
        KB().setKeyboardState(NORMAL);
        newState = true;
      }
      // Space to manually type a setting command
      else if (inchar == 32) { 
        String cmd = textPrompt(TR(STR_SETTINGS_CMD_PROMPT), "> ");
        if (cmd != "_RETURN_" && cmd != "_EXIT_" && cmd != "_CENTER_" && cmd != "") {
          String returnText = settingCommandSelect(cmd);
          if (returnText != "") OLED().sysMessage(returnText, 1000);
        }
        KB().setKeyboardState(NORMAL);
        newState = true;
      }
    }
  }

  // OLED Real-time Update
  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;

    // OLED Setting Scroll Preview always displayed as the default view
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    
    int startLine = 0;
    if (settingsScrollIndex >= 1) startLine = settingsScrollIndex - 1;
    
    int y = kOledPrevY0;
    for (int i = startLine; i < startLine + kOledPrevRows; i++) {
      if (i >= settingsList.size()) break;
      
      // Draw Triangle Selector for the current Item
      if (i == settingsScrollIndex) {
        u8g2.drawTriangle(0, y - 2 * kOledPrevTriH, 0, y, kOledPrevTriW, y - kOledPrevTriH);
      }
      
      FontEngine::drawText(DisplayTarget::OLED, kOledPrevX, y,
                           truncateWithEllipsis(TR(settingsList[i].nameID), SETTINGS_OLED_NAME_MAX_W, FontStyle::Tiny, DisplayTarget::OLED), FontStyle::Tiny);
      y += kOledPrevPitch;
    }

    // Draw instructions on the right side of the OLED
    FontEngine::drawText(DisplayTarget::OLED, 140, 12, TR(STR_SETTINGS_ENTER_CHANGE), FontStyle::Tiny);
    FontEngine::drawText(DisplayTarget::OLED, 140, 26, TR(STR_SETTINGS_SPACE_TYPE), FontStyle::Tiny);

    u8g2.sendBuffer();
  }
}

void einkHandler_SETTINGS() {
  if (newState) {
    newState = false;
    beginEinkScreen();
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    // Draw scrollbar vertically aligned to the left of the internal bounding box
    drawScrollbar(settingsList.size(), SETTINGS_ITEMS_PAGE, settingsScrollIndex, SETTINGS_BOX_X, SETTINGS_BOX_Y, SETTINGS_BOX_H, 4, false, GxEPD_BLACK, GxEPD_WHITE);

    int textX = SETTINGS_BOX_X + 10; // Offset text past scrollbar
    int valX = textX;
    int nameX = valX + 38; // Provide room on the left to draw a checkbox or the [value]

    // Calculate Sliding Window Pagination
    int startIdx = settingsScrollIndex - (SETTINGS_ITEMS_PAGE / 2);
    if (startIdx < 0) startIdx = 0;
    if (startIdx > max(0, (int)settingsList.size() - SETTINGS_ITEMS_PAGE)) {
      startIdx = max(0, (int)settingsList.size() - SETTINGS_ITEMS_PAGE);
    }

    int y = SETTINGS_BOX_Y + 16; // First Baseline (Adjusted for 20px Pitch)
    
    // Draw Settings Array Iterations
    for (int i = startIdx; i < min((int)settingsList.size(), startIdx + SETTINGS_ITEMS_PAGE); i++) {
      SettingItem& item = settingsList[i];

      // Localized label: shrink to Small when Body overflows, then ellipsis-truncate
      FontStyle nameStyle = FontEngine::fitStyle(DisplayTarget::EINK, TR(item.nameID), SETTINGS_NAME_MAX_W, kLabelCascade, kLabelCascadeCount);
      String name = truncateWithEllipsis(TR(item.nameID), SETTINGS_NAME_MAX_W, nameStyle);

      // Visually invert/highlight currently selected setting row
      if (i == settingsScrollIndex) {
        // Dynamically size the bounding box to fit the exact text, clamped so it
        // never crosses the box edge onto the background
        int nameWidth = FontEngine::textWidth(DisplayTarget::EINK, name, nameStyle);
        int boxW = min((nameX - valX) + nameWidth + 12, SETTINGS_HIGHLIGHT_MAX_W); // value width + text width + padding
        
        display.fillRoundRect(valX - 2, y - 13, boxW, 18, 4, GxEPD_BLACK);
        u8g2f.setForegroundColor(GxEPD_WHITE);
      } else {
        u8g2f.setForegroundColor(GxEPD_BLACK);
      }

      // Draw values on the left
      if (item.type == SetType::BOOLEAN) {
        const uint8_t* icon = *(item.boolVal) ? _toggleON : _toggleOFF;
        uint16_t color = (i == settingsScrollIndex) ? GxEPD_WHITE : GxEPD_BLACK;
        display.drawBitmap(valX, y - 10, icon, SETTINGS_TOG_W, SETTINGS_TOG_H, color);
      } 
      else if (item.type == SetType::INTEGER) {
        String valStr = "[" + String(*(item.intVal)) + "]";
        FontEngine::drawText(DisplayTarget::EINK, valX, y, valStr, FontStyle::Body);
      }
      else if (item.type == SetType::ACTION) {
        FontEngine::drawText(DisplayTarget::EINK, valX, y, "[>>]", FontStyle::Body);
      }

      // Draw text label
      FontEngine::drawText(DisplayTarget::EINK, nameX, y, name, nameStyle);
      y += SETTINGS_PITCH;
    }

    u8g2f.setForegroundColor(GxEPD_BLACK);
    
    // Output the static instruction prompt box below
    endEinkScreen(TR(STR_SETTINGS_EINK_HINT));
  }
}
#endif