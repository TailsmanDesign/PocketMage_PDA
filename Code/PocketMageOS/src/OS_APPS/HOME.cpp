//  ooooo   ooooo   .oooooo.   ooo        ooooo oooooooooooo  //
//  `888'   `888'  d8P'  `Y8b  `88.       .888' `888'     `8  //
//   888     888  888      888  888b     d'888   888          //
//   888ooooo888  888      888  8 Y88. .P  888   888oooo8     //
//   888     888  888      888  8  `888'   888   888    "     //
//   888     888  `88b    d88'  8    Y     888   888       o  //
//  o888o   o888o  `Y8bood8P'  o8o        o888o o888ooooood8  //
// AUDIT 1

#include <globals.h>
#include "esp_log.h"

#if !OTA_APP // POCKETMAGE_OS
static String currentLine = "";
static bool resetIdleAnim = false; 
static int prevTime = 0;
long lastInput = 0;
static int cursor_pos = 0;

// Layout constants
constexpr int HOME_TASKS_X      = 151;   // task list x
constexpr int HOME_TASKS_Y0     = 68;    // first task baseline
constexpr int HOME_TASKS_PITCH  = 25;    // task row pitch
constexpr int HOME_TASKS_MAX    = 7;     // max task rows
constexpr int HOME_TASKS_W      = kEinkWidth - HOME_TASKS_X - 10;  // 159: task text width

void HOME_INIT() {
  u8g2f.setForegroundColor(GxEPD_BLACK);
  display.setRotation(3);
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
  command = I18n::normalizeCommand(command);
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
      OLED().sysMessage(TR(STR_HOME_INVALID_NUMBER),1000);
    } 
    else if (sides == 1) {
      OLED().sysMessage(TR(STR_HOME_D1_SILLY),2000);
    }
    else {
      int roll = (esp_random() % sides) + 1;
      if (roll == sides)  OLED().sysMessage("D" + String(sides) + ": " + String(roll) + "!!!",3000);
      else if (roll == 1) OLED().sysMessage("D" + String(sides) + ": " + String(roll) + " :(",3000);
      else                OLED().sysMessage("D" + String(sides) + ": " + String(roll),3000);
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
    returnText = TR(STR_HOME_SD_COMPAT_OFF);
  }
  /////////////////////////////
  else if (command == "sleep") {
    PWR_BTN_event = true;
    //pocketmage::power::deepSleep();
  }
  /////////////////////////////
  else if (command == "home") {
    if (CurrentAppState == HOME) {
      returnText = TR(STR_HOME_WELCOME);
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
  else if (command == "bluetooth" || command == "bt") {
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
  else if (command == "chat" || command == "msg" || command == "4") {
    COMM_INIT();
  }
  else if (command == "guide" || command == "help") {
    GUIDE_INIT();
  }
  else if (command == "version" || command == "ver") {
    OLED().sysMessage(TR(STR_HOME_PMOS_PREFIX) + String(OS_VERSION_STR),2000);   
  }
  /////////////////////////////
  else if (command == "i farted") {
    returnText = TR(STR_HOME_SMELLS);
  } 
  else if (command == "poop") {
    returnText = TR(STR_HOME_YUCK);
  } 
  else if (command == "hello") {
    returnText = TR(STR_HOME_HEY);
  } 
  else if (command == "hi") {
    returnText = TR(STR_HOME_WHATS_UP);
  } 
  else if (command == "i love you") {
    returnText = TR(STR_HOME_LOVE);
  } 
  else if (command == "what can you do") {
    returnText = TR(STR_HOME_IDK);
  } 
  else if (command == "alexa") {
    returnText = "...";
  } 
  else if (command == "crash") {
    OLED().oledWord(TR(STR_HOME_CRASHING));
    // Force an ALU math exception
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;
  }
  else {
    returnText = settingCommandSelect(command);
  }

  if (returnText != "") {
    OLED().sysMessage(returnText,2000);
  }

  return returnText;
}

void drawHome() {
  EINK().resetDisplay();

  constexpr uint8_t appsPerRow = 5;   // Number of apps per row
  constexpr uint8_t gridPitch  = 60;  // Horizontal/vertical spacing
  constexpr uint8_t startX     = 20;  // Initial X position
  constexpr uint8_t startY     = 20;  // Initial Y position
  constexpr uint8_t row2Shift  = 10;  // extra Y shift for the third row

  for (int i = 0; i < sizeof(appIcons) / sizeof(appIcons[0]); i++) {
    int row = i / appsPerRow;
    int col = i % appsPerRow;
    
    int xPos = startX + (gridPitch * col);
    int yPos = startY + (gridPitch * row);
    if (row == 2) yPos += row2Shift;

    display.drawBitmap(xPos, yPos, appIcons[i], kIconCellSize, kIconCellSize, GxEPD_BLACK);
    const char* name = I18n::appName(i);
    FontStyle nameStyle = FontEngine::fitStyle(DisplayTarget::EINK, name,
                                               kGridLabelMaxW, kLabelCascade,
                                               kLabelCascadeCount);
    String label = truncateWithEllipsis(name, kGridLabelMaxW, nameStyle);
    int w = FontEngine::textWidth(DisplayTarget::EINK, label, nameStyle);
    FontEngine::drawText(DisplayTarget::EINK, xPos + (kIconCellSize / 2) - (w / 2),
                         yPos + kIconCellSize + kIconNameGap, label, nameStyle);
  }

  // Draw sideload app rounded rect
  //display.drawRoundRect(startX-15, (3*spacingY) - iconSize, (5*spacingX)+10, spacingY + 10, 15, GxEPD_BLACK);
  //display.drawRoundRect(startX-15, (3*spacingY) - iconSize, (1*spacingX)+10, spacingY + 10, 15, GxEPD_BLACK);

  // Draw sideload apps
  loadAndDrawAppIcon(80 , 150, 1, true, kGridLabelMaxW);  // OTA1
  loadAndDrawAppIcon(140, 150, 2, true, kGridLabelMaxW);  // OTA2
  loadAndDrawAppIcon(200, 150, 3, true, kGridLabelMaxW);  // OTA3
  loadAndDrawAppIcon(260, 150, 4, true, kGridLabelMaxW);  // OTA4

  // Draw status bar
  EINK().drawStatusBar(TR(STR_HOME_TYPE_CMD));
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
      if (command == "_RETURN_") return;
      else if (command != "_EXIT_") commandSelect(command);
      else newState = true;
      break;

    case NOWLATER:
      // Any keypad key turns the device back on; the power button powers it off
      {
        char wakeKey = KB().updateKeypress();
        if (wakeKey != 0) {
          // Feed the shortcut letter through so boot shortcuts work from
          // NOWLATER (loadState's own keypad read would come up empty here).
          wakeFromNowlater(wakeKey);
          break;
        }
      }
      DateTime now = CLOCK().nowDT();
      if (prevTime != now.minute()) {
        prevTime = now.minute();
        newState = true;
      }
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
        EINK().refresh();
      }
      break;

    case NOWLATER:
      if (newState) {
        newState = false;

        // NOWLATER frame is authored for rotation 3 (see #280); force it so the
        // clock face never renders inverted after another app left a rotation.
        display.setRotation(3);

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

          int loopCount = std::min((int)tasks.size(), HOME_TASKS_MAX);
          for (int i = 0; i < loopCount; i++) {
            FontEngine::drawText(DisplayTarget::EINK, HOME_TASKS_X, HOME_TASKS_Y0 + (HOME_TASKS_PITCH * i), truncateWithEllipsis(tasks[i][0], HOME_TASKS_W, FontStyle::Body), FontStyle::Body);
          }
        }

        EINK().refresh();
      }
      break;
  }
}
#endif
