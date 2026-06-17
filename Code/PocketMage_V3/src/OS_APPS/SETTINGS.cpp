// AUDIT 1

#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS

// Simplified state machine
enum SettingsState { SETTINGS_MAIN };
SettingsState CurrentSettingsState = SETTINGS_MAIN;

void SETTINGS_INIT() {
  // OPEN SETTINGS
  CurrentAppState = SETTINGS;
  CurrentSettingsState = SETTINGS_MAIN;
  KB().setKeyboardState(NORMAL);
  newState = true;
}

String settingCommandSelect(String command) {
  String returnText = "";
  command.toLowerCase();

  if (command.startsWith("timeset") || command.startsWith("settime")) {
    String timePart = "";
    
    // Extract the argument if there is a space
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      timePart = command.substring(spaceIdx + 1);
      timePart.trim();
    }

    // If no argument was provided, launch the interactive UI
    if (timePart.length() == 0) {
      int newTime = timePrompt(); // Returns integer like 1430 or 5
      
      // Format the integer back into a safe, padded string (e.g., "00:05" or "14:30")
      char timeBuf[6];
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", newTime / 100, newTime % 100);
      
      CLOCK().setTimeFromString(String(timeBuf));
      returnText = "Time Updated to " + String(timeBuf);
    } 
    // If a manual argument was provided, validate and parse it directly
    else if (timePart.length() >= 4) { 
      CLOCK().setTimeFromString(timePart);
      returnText = "Time Updated";
    } 
    else {
      returnText = "Invalid Format (HH:MM)";
    }
    
    return returnText;
  }
  else if (command.startsWith("dateset") || command.startsWith("setdate")) {
    String datePart = "";
    
    // Extract the argument if there is a space
    int spaceIdx = command.indexOf(' ');
    if (spaceIdx != -1) {
      datePart = command.substring(spaceIdx + 1);
      datePart.trim();
    }

    // If no argument was provided, launch the interactive UI
    if (datePart.length() == 0) {
      String newDate = datePrompt(); // Returns formatted "DD/MM/YYYY"

      // Parse the returned string into integers
      int day   = newDate.substring(0, 2).toInt();
      int month = newDate.substring(3, 5).toInt();
      int year  = newDate.substring(6, 10).toInt();

      DateTime now = CLOCK().nowDT();  // Preserve current time
      CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
      
      returnText = "Date Updated to " + newDate;
    }
    // If a manual argument was provided, validate and parse it directly
    else if (datePart.length() == 8 && datePart.toInt() > 0) {
      int year  = datePart.substring(0, 4).toInt();
      int month = datePart.substring(4, 6).toInt();
      int day   = datePart.substring(6, 8).toInt();

      DateTime now = CLOCK().nowDT();  // Preserve current time
      CLOCK().getRTC().adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
      returnText = "Date Updated";
    } else {
      returnText = "Invalid format (YYYYMMDD)";
    }
    
    return returnText;
  }
  else if (command.startsWith("lumina ")) {
    String luminaPart = command.substring(7);
    int lumina = stringToInt(luminaPart);
    if (lumina == -1) return "Invalid";
    
    if (lumina > 255) lumina = 255;
    else if (lumina < 0) lumina = 0;
    
    OLED_BRIGHTNESS = lumina;
    u8g2.setContrast(OLED_BRIGHTNESS);
    
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_BRIGHTNESS", OLED_BRIGHTNESS);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("timeout ")) {
    String timeoutPart = command.substring(8);
    int timeout = stringToInt(timeoutPart);
    if (timeout == -1) return "Invalid!";
    
    if (timeout > 3600) timeout = 3600;
    else if (timeout < 15) timeout = 15;
    
    TIMEOUT = timeout;
    
    prefs.begin("PocketMage", false);
    prefs.putInt("TIMEOUT", TIMEOUT);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("oledfps ")) {
    String oledfpsPart = command.substring(8);
    int oledfps = stringToInt(oledfpsPart);
    if (oledfps == -1) return "Invalid";
    
    if (oledfps > 144) oledfps = 144;
    else if (oledfps < 5) oledfps = 5;
    
    OLED_MAX_FPS = oledfps;
    
    prefs.begin("PocketMage", false);
    prefs.putInt("OLED_MAX_FPS", OLED_MAX_FPS);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("clock ")) {
    String clockPart = command.substring(6);
    clockPart.trim();

    if (clockPart != "t" && clockPart != "f") return "Invalid";

    SYSTEM_CLOCK = (clockPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SYSTEM_CLOCK", SYSTEM_CLOCK);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("showyear ")) {
    String yearPart = command.substring(9);
    yearPart.trim();

    if (yearPart != "t" && yearPart != "f") return "Invalid";

    SHOW_YEAR = (yearPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SHOW_YEAR", SHOW_YEAR);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("savepower ")) {
    String savePowerPart = command.substring(10);
    savePowerPart.trim();

    if (savePowerPart != "t" && savePowerPart != "f") return "Invalid";

    SAVE_POWER = (savePowerPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("SAVE_POWER", SAVE_POWER);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("debug ")) {
    String debugPart = command.substring(6);
    debugPart.trim();

    if (debugPart != "t" && debugPart != "f") return "Invalid";

    DEBUG_VERBOSE = (debugPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("DEBUG_VERBOSE", DEBUG_VERBOSE);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("boottohome ")) {
    String bootHomePart = command.substring(11);
    bootHomePart.trim();

    if (bootHomePart != "t" && bootHomePart != "f") return "Invalid";

    HOME_ON_BOOT = (bootHomePart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("HOME_ON_BOOT", HOME_ON_BOOT);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else if (command.startsWith("allownosd ")) {
    String noSDPart = command.substring(10);
    noSDPart.trim();

    if (noSDPart != "t" && noSDPart != "f") return "Invalid";

    ALLOW_NO_MICROSD = (noSDPart == "t");
    
    prefs.begin("PocketMage", false);
    prefs.putBool("ALLOW_NO_SD", ALLOW_NO_MICROSD);
    prefs.end();
    
    newState = true;
    return "Settings Updated";
  }
  else {
    return "Huh?";
  }
}

void processKB_settings() {
  String command = "";
  String returnText = "";

  switch (CurrentSettingsState) {
    case SETTINGS_MAIN:
      command = textPrompt();
      if (command == "_RETURN_") return;
      else if (command != "_EXIT_") {
        returnText = settingCommandSelect(command);
        if (returnText != "") {
          OLED().sysMessage(returnText,1000);
        }
      }
      else HOME_INIT();
      break;
  }
}

void einkHandler_settings() {
  if (newState) {
    newState = false;

    EINK().resetDisplay();
    display.drawBitmap(0, 0, _settings, 320, 218, GxEPD_BLACK);

    u8g2f.setFont(u8g2_font_ncenR10_tf);
    u8g2f.setFontMode(1);
    
    // First column of settings
    // OLED_BRIGHTNESS
    u8g2f.setCursor(8, 42);
    u8g2f.print(String(OLED_BRIGHTNESS).c_str());
    // TIMEOUT
    u8g2f.setCursor(8, 65);
    u8g2f.print(String(TIMEOUT).c_str());
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
    u8g2f.setCursor(163, 42);
    u8g2f.print(String(OLED_MAX_FPS).c_str());

    EINK().drawStatusBar("Type a Command:");

    EINK().multiPassRefresh(2);
  }
}
#endif