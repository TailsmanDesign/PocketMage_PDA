//  ooooo   ooooo   .oooooo.   ooo        ooooo oooooooooooo  //
//  `888'   `888'  d8P'  `Y8b  `88.       .888' `888'     `8  //
//   888     888  888      888  888b     d'888   888          //
//   888ooooo888  888      888  8 Y88. .P  888   888oooo8     //
//   888     888  888      888  8  `888'   888   888    "     //
//   888     888  `88b    d88'  8    Y     888   888       o  //
//  o888o   o888o  `Y8bood8P'  o8o        o888o o888ooooood8  //

#include <globals.h>
#include "esp_log.h"

#if !OTA_APP // POCKETMAGE_OS
static String currentLine = "";
static bool resetIdleAnim = false; 
static int prevTime = 0;
long lastInput = 0;
static int cursor_pos = 0;

void HOME_INIT() {
  CurrentAppState = HOME;
  currentLine     = "";
  KB().setKeyboardState(NORMAL);
  CurrentHOMEState = HOME_HOME;
  lastInput = millis();
  newState = true;
  //frames.push_back(&testTextScreen);
}

String commandSelect(String command) {
  String returnText = "";
  command.toLowerCase();

  // OPEN IN FILE WIZARD
  if (command.startsWith("-")) {
    command = removeChar(command, ' ');
    command = removeChar(command, '-');
    keypad.disableInterrupts();
    PM_SDAUTO().listDir(*global_fs, "/");
    keypad.enableInterrupts();

    for (uint8_t i = 0; i < MAX_FILES; i++) {
      String lowerFileName = PM_SDAUTO().getFilesListIndex(i);
      lowerFileName.toLowerCase();
      if (command == lowerFileName || (command+".txt") == lowerFileName || ("/"+command+".txt") == lowerFileName) {
        PM_SDAUTO().setWorkingFile(PM_SDAUTO().getFilesListIndex(i));
        FILEWIZ_INIT();
        return "";
      }
    }
  }

  // OPEN IN TXT EDITOR
  if (command.startsWith("/")) {
    command = removeChar(command, ' ');
    command = removeChar(command, '/');
    keypad.disableInterrupts();
    PM_SDAUTO().listDir(*global_fs, "/");
    keypad.enableInterrupts();

    for (uint8_t i = 0; i < MAX_FILES; i++) {
      String lowerFileName = PM_SDAUTO().getFilesListIndex(i);
      lowerFileName.toLowerCase();
      if (command == lowerFileName || (command+".txt") == lowerFileName || ("/"+command+".txt") == lowerFileName) {
        PM_SDAUTO().setEditingFile(PM_SDAUTO().getFilesListIndex(i));
        TXT_INIT();
        return "";
      }
    }
  }

  // Dice Roll
  if (command.startsWith("roll d")) {
    String numStr = command.substring(6);
    int sides = numStr.toInt();
    if (sides < 1) {
      OLED().oledWord("Please enter a valid number");
      delay(2000);
    } 
    else if (sides == 1) {
      OLED().oledWord("D1: you rolled a 1, duh!");
      delay(2000);
    }
    else {
      int roll = (esp_random() % sides) + 1;
      if (roll == sides)  OLED().oledWord("D" + String(sides) + ": " + String(roll) + "!!!");
      else if (roll == 1) OLED().oledWord("D" + String(sides) + ": " + String(roll) + " :(");
      else                OLED().oledWord("D" + String(sides) + ": " + String(roll));
      delay(3000);
      KB().setKeyboardState(NORMAL);
    }
  }

  // Boot to other apps
  else if (command == "a") rebootToAppSlot(1);
  else if (command == "b") rebootToAppSlot(2);
  else if (command == "c") rebootToAppSlot(3);
  else if (command == "d") rebootToAppSlot(4);
  
  /////////////////////////////
  else if (command == "reset") {
    esp_restart();
  } 
  else if (command == "sdreset") {
    prefs.begin("PocketMage", false);
    prefs.putBool("SD_SPI_CMPT", false);
    prefs.end();
    returnText = "SD compatibility mode disabled";
  }
  /////////////////////////////
  else if (command == "sleep") {
    PWR_BTN_event = true;
    //pocketmage::power::deepSleep();
  }
  /////////////////////////////
  else if (command == "home") {
    if (CurrentAppState == HOME) {
      returnText = "You're home, silly!";
    }
    else {
      HOME_INIT();
    }
  } 
  /////////////////////////////
  else if (command == "note" || command == "text" || command == "write" || command == "notebook" || command == "notepad" || command == "txt" || command == "1") {
    TXT_INIT();
  }
  /////////////////////////////
  else if (command == "file wizard" || command == "wiz" || command == "file wiz" || command == "filewiz" || command == "file" || command == "2") {
    FILEWIZ_INIT();
  }
  /////////////////////////////
  else if (command == "back up" || command == "export" || command == "transfer" || command == "usb transfer" || command == "usb" || command == "3") {
    USB_INIT();
  }
  else if (command == "app loader" || command == "app" || command == "loader" || command == "load") {
    APPLOADER_INIT();
  }
  /////////////////////////////
  else if (command == "tasks" || command == "task" || command == "6") {
    TASKS_INIT();
  }
  else if (command == "term" || command == "terminal" || command == "cmd" || command == "command" || command == "script" || command == "0") {
    TERMINAL_INIT();
  }
  /////////////////////////////
  else if (command == "bluetooth" || command == "bt" || command == "4") {
    // OPEN BLUETOOTH
  }
  /////////////////////////////
  else if (command == "preferences" || command == "setting" || command == "settings" || command == "set" || command == "5") {
    SETTINGS_INIT();
  }
  else if (command == "cal" || command == "calendar" || command == "7") {
    CALENDAR_INIT();
  }
  else if (command == "lex" || command == "lexicon" || command == "dict" || command == "dictionary" || command == "9") {
    LEXICON_INIT();
  }
  else if (command == "journ" || command == "journal" || command == "daily" || command == "8") {
    JOURNAL_INIT();
  }
  /////////////////////////////
  else if (command == "i farted") {
    returnText = "That smells";
  } 
  else if (command == "poop") {
    returnText = "Yuck";
  } 
  else if (command == "hello") {
    returnText = "Hey, you!";
  } 
  else if (command == "hi") {
    returnText = "What's up?";
  } 
  else if (command == "i love you") {
    returnText = "luv u 2 <3";
  } 
  else if (command == "what can you do") {
    returnText = "idk man";
  } 
  else if (command == "alexa") {
    returnText = "...";
  } 
  else {
    returnText = settingCommandSelect(command);
  }

  if (returnText != "") {
    OLED().oledWord(returnText);
    delay(2000);
  }

  return returnText;
}

void drawHome() {
  EINK().resetDisplay();

  int16_t x1, y1;
  uint16_t charWidth, charHeight;
  uint8_t appsPerRow = 5; // Number of apps per row
  uint8_t spacingX = 60;  // Horizontal spacing
  uint8_t spacingY = 60;  // Vertical spacing
  uint8_t iconSize = 40;  // Icon width and height
  uint8_t startX = 20;    // Initial X position
  uint8_t startY = 20;    // Initial Y position

  display.setFont(&FreeSerif9pt7b);
  for (int i = 0; i < sizeof(appIcons) / sizeof(appIcons[0]); i++) {
    int row = i / appsPerRow;
    int col = i % appsPerRow;
    
    int xPos = startX + (spacingX * col);
    int yPos = startY + (spacingY * row);
    if (row == 2) yPos += 10;

    display.drawBitmap(xPos, yPos, appIcons[i], iconSize, iconSize, GxEPD_BLACK);
    display.getTextBounds(appStateNames[i], 0, 0, &x1, &y1, &charWidth, &charHeight);
    display.setCursor(xPos + (iconSize / 2) - (charWidth / 2), yPos + iconSize + 13);
    display.print(appStateNames[i]);
  }
  display.setFont(&FreeMonoBold9pt7b);

  // Draw sideload app rounded rect
  //display.drawRoundRect(startX-15, (3*spacingY) - iconSize, (5*spacingX)+10, spacingY + 10, 15, GxEPD_BLACK);
  //display.drawRoundRect(startX-15, (3*spacingY) - iconSize, (1*spacingX)+10, spacingY + 10, 15, GxEPD_BLACK);

  // Draw sideload apps
  loadAndDrawAppIcon(80 , 150, 1, true, 7);  // OTA1
  loadAndDrawAppIcon(140, 150, 2, true, 7);  // OTA2
  loadAndDrawAppIcon(200, 150, 3, true, 7);  // OTA3
  loadAndDrawAppIcon(260, 150, 4, true, 7);  // OTA4

  // Draw status bar
  EINK().drawStatusBar("Type a Command:");
}

void drawThickLine(int x0, int y0, int x1, int y1, int thickness) {
  float dx = x1 - x0;
  float dy = y1 - y0;
  float length = sqrt(dx * dx + dy * dy);
  float stepX = dx / length;
  float stepY = dy / length;

  for (float i = 0; i <= length; i += thickness / 2.0) {
    int cx = round(x0 + i * stepX);
    int cy = round(y0 + i * stepY);
    display.fillCircle(cx, cy, thickness / 2, GxEPD_BLACK);
  }
}

void resetIdle() {
  resetIdleAnim = true;
}

void mageIdle(bool internalRefresh) {
  enum MageState { IDLE, RUN_LEFT, RUN_RIGHT};
  static MageState CurrentMageState = RUN_RIGHT;

  static int MagePosition = -30; //px
  static bool MageDirection = true; // T:right, F:left
  static int goalPosition = 30;
  static int progress = 0;
  static long internalMillis = 0;
  static int runSpeed = 3;

  uint32_t chance = 1;

  if (resetIdleAnim) {
    MagePosition = -30;
    MageDirection = true;
    goalPosition = 30;
    progress = 0;
    chance = 1;
    internalMillis = 0;
    runSpeed = 3;
    CurrentMageState = RUN_RIGHT;

    resetIdleAnim = false;
  }
  
  // Frame rate control
  const uint32_t FRAME_INTERVAL = 100; // milliseconds per frame (e.g., 10 FPS = 100 ms)
  static uint32_t lastUpdate = 0;
  internalMillis++;

  if (millis() - lastUpdate < FRAME_INTERVAL) return; // skip until next frame
  lastUpdate = millis();

  if (internalRefresh) u8g2.clearBuffer();
  u8g2.setBitmapMode(1);

  switch (CurrentMageState) {
    case IDLE:
      // Idle animation (half frames)
      if (MageDirection)  u8g2.drawXBMP(MagePosition,-1,29,29,idle_right_allArray[(internalMillis/4) % 7]);
      else                u8g2.drawXBMP(MagePosition,-1,29,29,idle_left_allArray[(internalMillis/4) % 7]);

      // 1 in 50 chance to stop idling (0-5 sec)
      chance = (esp_random() % 50);
      
      if (chance == 0) {
        // Generate random position for Mage to walk to
        goalPosition = (esp_random() % (u8g2.getDisplayWidth()-29)); // 0 - screen width)

        // Generate random run speed 2-4
        runSpeed = (esp_random() % 3) + 2;
        
        if      (goalPosition < MagePosition)  CurrentMageState = RUN_LEFT;
        else if (goalPosition > MagePosition)  CurrentMageState = RUN_RIGHT;
      }
      break;
    case RUN_LEFT:
      MageDirection = false;
      
      // Display animation frame
      if (progress < 5) {
        u8g2.drawXBMP(MagePosition,-1,29,29,trans_left_allArray[progress]);      // Transition for first 5 frames
        progress++;
        MagePosition--;
      }
      else {
        u8g2.drawXBMP(MagePosition,-1,29,29,run_left_allArray[(progress-5)%6]);  // Rest of frames are running
        progress++;
        MagePosition-=runSpeed;
      }

      // Goal reached
      if (MagePosition <= goalPosition) {
        progress = 0;
        CurrentMageState = IDLE;
      }

      break;
    case RUN_RIGHT:
      MageDirection = true;
      
      // Display animation frame
      if (progress < 5) {
        u8g2.drawXBMP(MagePosition,-1,29,29,trans_right_allArray[progress]);      // Transition for first 5 frames
        progress++;
        MagePosition++;
      }
      else {
        u8g2.drawXBMP(MagePosition,-1,29,29,run_right_allArray[(progress-5)%6]);  // Rest of frames are running
        progress++;
        MagePosition+=runSpeed;
      }             

      // Goal reached
      if (MagePosition >= goalPosition) {
        progress = 0;
        CurrentMageState = IDLE;
      }
      break;
  }

  if (internalRefresh) {
    OLED().infoBar();
    u8g2.sendBuffer();
  }
}

void processKB_HOME() {
  int currentMillis = millis();
  String left = "";
  String right = "";
  String command = "";

  switch (CurrentHOMEState) {
    case HOME_HOME:
      KB().setKeyboardState(NORMAL);
      command = textPrompt();
      if (command != "_EXIT_") commandSelect(command);
      else newState = true;
      break;

    case NOWLATER:
      DateTime now = CLOCK().nowDT();
      if (prevTime != now.minute()) {
        prevTime = now.minute();
        newState = true;
      }
      else newState = false;
      break;
  }
}

void einkHandler_HOME() {
  switch (CurrentHOMEState) {
    case HOME_HOME:
      if (newState) {
        newState = false;
        drawHome();
        //EINK().refresh();
        //einkFramesDynamic(frames,false);
        EINK().multiPassRefresh(1);
      }
      break;

    case NOWLATER:
      if (newState) {
        newState = false;

        // BACKGROUND
        display.drawBitmap(0, 0, nowLaterallArray[0], 320, 240, GxEPD_BLACK);

        // CLOCK HANDS
        float pi = 3.14159;

        float hourLength    = 25;
        float minuteLength  = 40;
        uint8_t hourWidth   = 5;
        uint8_t minuteWidth = 2;

        uint8_t centerX     = 76;
        uint8_t centerY     = 94;

        DateTime now = CLOCK().nowDT();

        // Convert time to proper angles in radians
        float minuteAngle = (now.minute() / 60.0) * 2 * pi;  
        float hourAngle   = ((now.hour() % 12) / 12.0 + (now.minute() / 60.0) / 12.0) * 2 * pi;

        // Convert angles to coordinates
        uint8_t minuteX = (minuteLength * cos(minuteAngle - pi/2)) + centerX;
        uint8_t minuteY = (minuteLength * sin(minuteAngle - pi/2)) + centerY;
        uint8_t hourX   = (hourLength   * cos(hourAngle   - pi/2)) + centerX;
        uint8_t hourY   = (hourLength   * sin(hourAngle   - pi/2)) + centerY;

        drawThickLine(centerX, centerY, minuteX, minuteY, minuteWidth);
        drawThickLine(centerX, centerY, hourX  , hourY  , hourWidth);

        // WEATHER

        // TASKS/CALENDAR
        //151,68
        if (!tasks.empty()) {
          ESP_LOGV("CALENDAR", "Printing Tasks\n");

          int loopCount = std::min((int)tasks.size(), 7);
          for (int i = 0; i < loopCount; i++) {
            display.setFont(&FreeSerif9pt7b);
            // PRINT TASK NAME
            display.setCursor(151, 68 + (25 * i));
            display.print(tasks[i][0].c_str());
          }
        }

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
  }
}
#endif
