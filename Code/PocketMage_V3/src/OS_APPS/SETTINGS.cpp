#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS
enum SettingsState { settings0, settings1 };
SettingsState CurrentSettingsState = settings0;

static String currentLine = "";
static int cursor_pos = 0;

void SETTINGS_INIT() {
  // OPEN SETTINGS
  currentLine = "";
  CurrentAppState = SETTINGS;
  CurrentSettingsState = settings0;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

String settingCommandSelect(String command) {
  String returnText = "";
  command.toLowerCase();

  if (command.startsWith("timeset ") || command.startsWith("settime ")) {
    String timePart = command.substring(8);
    CLOCK().setTimeFromString(timePart);
    return returnText;
  }
  else if (command.startsWith("dateset ") || command.startsWith("setdate ")) {
    String datePart = command.substring(8);
    if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();

      DateTime now = CLOCK().nowDT();  // Preserve current time
      CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    } else {
      returnText = "Invalid format (use YYYYMMDD)";
    }
    return returnText;
  }
  else if (command.startsWith("lumina ")) {
    String luminaPart = command.substring(7);
    int lumina = stringToInt(luminaPart);
    if (lumina == -1) {
      returnText = "Invalid";
      return returnText;
    }
    else if (lumina > 255) lumina = 255;
    else if (lumina < 0) lumina = 0;
    OLED_BRIGHTNESS = lumina;
    u8g2.setContrast(OLED_BRIGHTNESS);
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_BRIGHTNESS", OLED_BRIGHTNESS);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }
  else if (command.startsWith("timeout ")) {
    String timeoutPart = command.substring(8);
    int timeout = stringToInt(timeoutPart);
    if (timeout == -1) return "Invalid!";
    else if (timeout > 3600) timeout = 3600;
    else if (timeout < 15) timeout = 15;
    TIMEOUT = timeout;
    prefs.begin("PocketMage", false);
    prefs.putInt("TIMEOUT", TIMEOUT);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }
  else if (command.startsWith("oledfps ")) {
    String oledfpsPart = command.substring(8);
    int oledfps = stringToInt(oledfpsPart);
    if (oledfps == -1) {
      returnText = "Invalid";
      return returnText;
    }
    else if (oledfps > 144) oledfps = 144;
    else if (oledfps < 5) oledfps = 5;
    OLED_MAX_FPS = oledfps;
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_MAX_FPS", OLED_MAX_FPS);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }
  else if (command.startsWith("clock ")) {
    String clockPart = command.substring(6);
    clockPart.trim();

    if (clockPart != "t" && clockPart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    SYSTEM_CLOCK = (clockPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SYSTEM_CLOCK", SYSTEM_CLOCK);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }

  else if (command.startsWith("showyear ")) {
    String yearPart = command.substring(9);
    yearPart.trim();

    if (yearPart != "t" && yearPart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    SHOW_YEAR = (yearPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SHOW_YEAR", SHOW_YEAR);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }

  else if (command.startsWith("savepower ")) {
    String savePowerPart = command.substring(10);
    savePowerPart.trim();

    if (savePowerPart != "t" && savePowerPart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    SAVE_POWER = (savePowerPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("SAVE_POWER", SAVE_POWER);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }

  else if (command.startsWith("debug ")) {
    String debugPart = command.substring(6);
    debugPart.trim();

    if (debugPart != "t" && debugPart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    DEBUG_VERBOSE = (debugPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("DEBUG_VERBOSE", DEBUG_VERBOSE);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }

  else if (command.startsWith("boottohome ")) {
    String bootHomePart = command.substring(11);
    bootHomePart.trim();

    if (bootHomePart != "t" && bootHomePart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    HOME_ON_BOOT = (bootHomePart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("HOME_ON_BOOT", HOME_ON_BOOT);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }

  else if (command.startsWith("allownosd ")) {
    String noSDPart = command.substring(10);
    noSDPart.trim();

    if (noSDPart != "t" && noSDPart != "f") {
      returnText = "Invalid";
      return returnText;
    }

    ALLOW_NO_MICROSD = (noSDPart == "t");
    prefs.begin("PocketMage", false);
    prefs.putBool("ALLOW_NO_SD", ALLOW_NO_MICROSD);
    prefs.end();
    newState = true;
    returnText = "Settings Updated";
    return returnText;
  }
  else {
    returnText = "Huh?";
    return returnText;
  }
  return "";
}

void processKB_settings() {
  int currentMillis = millis();
  String left = "";
  String right = "";
  String command = "";
  String returnText = "";

  switch (CurrentSettingsState) {
    case settings0:
      command = textPrompt();
      if (command != "_EXIT_") {
        returnText = settingCommandSelect(command);
        if (returnText != "") {
          OLED().oledWord(returnText);
          delay(1000);
        }
      }
      else HOME_INIT();
      break;

    case settings1:
      break;
  }
}

void einkHandler_settings() {
  if (newState) {
    newState = false;

    // Load settings
    loadState(false);
    
    // Display Background
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    display.setFont(&FreeSerif9pt7b);
    // First column of settings
    // OLED_BRIGHTNESS
    display.setCursor(8, 42);
    display.print(String(OLED_BRIGHTNESS).c_str());
    // TIMEOUT
    display.setCursor(8, 65);
    display.print(String(TIMEOUT).c_str());
    // SYSTEM_CLOCK
    if (SYSTEM_CLOCK) display.drawBitmap(8, 75, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 75, _toggleOFF, 26, 11, GxEPD_BLACK);
    // SHOW_YEAR
    if (SHOW_YEAR) display.drawBitmap(8, 98, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 98, _toggleOFF, 26, 11, GxEPD_BLACK);
    // SAVE_POWER
    if (SAVE_POWER) display.drawBitmap(8, 121, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 121, _toggleOFF, 26, 11, GxEPD_BLACK);
    // DEBUG_VERBOSE
    if (DEBUG_VERBOSE) display.drawBitmap(8, 144, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 144, _toggleOFF, 26, 11, GxEPD_BLACK);
    // HOME_ON_BOOT
    if (HOME_ON_BOOT) display.drawBitmap(8, 167, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 167, _toggleOFF, 26, 11, GxEPD_BLACK);
    // ALLOW_NO_MICROSD
    if (ALLOW_NO_MICROSD) display.drawBitmap(8, 190, _toggleON, 26, 11, GxEPD_BLACK);
    else display.drawBitmap(8, 190, _toggleOFF, 26, 11, GxEPD_BLACK);
    // OLED_MAX_FPS
    display.setCursor(163, 42);
    display.print(String(OLED_MAX_FPS).c_str());

    EINK().drawStatusBar("Type a Command:");

    EINK().multiPassRefresh(2);
  }
}
#endif
