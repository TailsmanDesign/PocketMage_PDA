
#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS
enum JournalState {J_MENU, J_TXT};
JournalState CurrentJournalState = J_MENU;

String currentJournal = "";
String bufferEditingFile = PM_SDAUTO().getEditingFile();
static String currentLine = "";
static volatile bool doFull = false;
static int cursor_pos = 0;

void JOURNAL_INIT() {
  CurrentAppState = JOURNAL;
  CurrentJournalState = J_MENU;
  EINK().forceSlowFullUpdate(true);
  newState = true;
  KB().setKeyboardState(NORMAL);
  bufferEditingFile = PM_SDAUTO().getEditingFile();
}

// File Operations
void loadJournal() {
  PM_SDAUTO().setEditingFile(currentJournal);
  PM_SDAUTO().loadFile();
}

void saveJournal() {
  PM_SDAUTO().setEditingFile(currentJournal);
  PM_SDAUTO().saveFile();
}

String getCurrentJournal() {return currentJournal;}

// Functions
bool isLeapYear(int year) {
  return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

void drawJMENU() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  // Display background
  EINK().drawStatusBar("Type:YYYYMMDD or (T)oday");
  display.drawBitmap(0, 0, _journal, 320, 218, GxEPD_BLACK);

  // Update current progress graph
  DateTime now = CLOCK().nowDT();
  String year = String(now.year());

  // Files are in the format "/journal/YYYYMMDD.txt"
  // JANUARY
  for (int i = 1; i < 31; i++) {
    String dayCode = "";
    if (i<10) dayCode = "0"+String(i);
    else dayCode = String(i);
    
    String fileCode = "/journal/" + year + "01" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7*(i-1)), 50, 4, 4, GxEPD_BLACK);
  }
  // FEBRUARY
  int febDays = isLeapYear(year.toInt()) ? 29 : 28;
  for (int i = 1; i <= febDays; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "02" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 59, 4, 4, GxEPD_BLACK);
  }

  // MARCH
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "03" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 68, 4, 4, GxEPD_BLACK);
  }

  // APRIL
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "04" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 77, 4, 4, GxEPD_BLACK);
  }

  // MAY
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "05" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 86, 4, 4, GxEPD_BLACK);
  }

  // JUNE
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "06" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 95, 4, 4, GxEPD_BLACK);
  }

  // JULY
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "07" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 104, 4, 4, GxEPD_BLACK);
  }

  // AUGUST
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "08" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 113, 4, 4, GxEPD_BLACK);
  }

  // SEPTEMBER
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "09" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 122, 4, 4, GxEPD_BLACK);
  }

  // OCTOBER
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "10" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 131, 4, 4, GxEPD_BLACK);
  }

  // NOVEMBER
  for (int i = 1; i <= 30; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "11" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 140, 4, 4, GxEPD_BLACK);
  }

  // DECEMBER
  for (int i = 1; i <= 31; i++) {
    String dayCode = (i < 10) ? "0" + String(i) : String(i);
    String fileCode = "/journal/" + year + "12" + dayCode + ".txt";
    if (global_fs->exists(fileCode)) display.fillRect(91 + (7 * (i - 1)), 149, 4, 4, GxEPD_BLACK);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

void JMENUCommand(String command) {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  command.toLowerCase();

  if (command == "t") {
    DateTime now = CLOCK().nowDT();

    String dayStr = "";
    if (now.day() < 10) dayStr = "0" + String(now.day());
    else dayStr = String(now.day());

    String monthStr = "";
    if (now.month() < 10) monthStr = "0" + String(now.month());
    else monthStr = String(now.month());

    String fileName = "/journal/" + String(now.year()) + monthStr + dayStr + ".txt";
    
    // If file doesn't exist, create it
    if (!global_fs->exists(fileName)) {
      File f = global_fs->open(fileName, FILE_WRITE);
      if (f) f.close();
    }

    currentJournal = fileName;

    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
    // Load file
    TXT_INIT_JournalMode();
    
    return;
  }

  // command in the form "YYYYMMDD"
  else if (command.length() == 8 && command.toInt() > 0) {
    String fileName = "/journal/" + command + ".txt";

    if (!global_fs->exists(fileName)) {
      File f = global_fs->open(fileName, FILE_WRITE);
      if (f) f.close();
    }

    currentJournal = fileName;

    if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
    SDActive = false;
    // Load file
    TXT_INIT_JournalMode();
    
    return;
  }

  // command in the form "jan 1"
  else {
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex != -1 && spaceIndex < command.length() - 1) {
      String monthStr = command.substring(0, spaceIndex);
      String dayStr = command.substring(spaceIndex + 1);
      int day = dayStr.toInt();

      if (day < 1 || day > 31) {
        if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
        SDActive = false;
        return;  // invalid day
      }
      String monthMap = "janfebmaraprmayjunjulaugsepoctnovdec";
      int monthIndex = monthMap.indexOf(monthStr);
      if (monthIndex == -1) {
        if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
        SDActive = false;
        return;  // invalid month
      }
      int month = (monthIndex / 3) + 1;

      String year = String(CLOCK().nowDT().year());
      String m = (month < 10) ? "0" + String(month) : String(month);
      String d = (day < 10) ? "0" + String(day) : String(day);
      String fileName = "/journal/" + year + m + d + ".txt";

      if (!global_fs->exists(fileName)) {
        File f = global_fs->open(fileName, FILE_WRITE);
        if (f) f.close();
      }

      currentJournal = fileName;

      if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
      SDActive = false;
      // Load file
      TXT_INIT_JournalMode();
      
      return;
    }
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

// Loops
void processKB_JOURNAL() {
  int currentMillis = millis();
  char inchar;
  String left = "";
  String right = "";

  switch (CurrentJournalState) {
    case J_MENU:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = KB().updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);   
        //CR Recieved
        else if (inchar == 13) {                          
          JMENUCommand(currentLine);
          currentLine = "";
          cursor_pos = 0;
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
        //BKSP Recieved
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
        else if (inchar == 12 ) {
          HOME_INIT();
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
        }
        else {
          //split line at cursor_pos
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
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (KB().getKeyboardState() != NORMAL) {
            KB().setKeyboardState(NORMAL);
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at OLED_MAX_FPS
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          OLED().oledLine(currentLine, cursor_pos, false);
        }
      }
      break;
    default:
      CurrentJournalState = J_MENU;
      break;
  }
}

void einkHandler_JOURNAL() {
  switch (CurrentJournalState) {
    case J_MENU:
      if (newState) {
        newState = false;

        drawJMENU();

        EINK().multiPassRefresh(2);
      }
      break;
    default:
      CurrentJournalState = J_MENU;
      break;
  } 
}
#endif
