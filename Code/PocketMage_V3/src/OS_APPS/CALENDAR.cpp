// ooooooooooooo       .o.        .oooooo..o oooo    oooo  .oooooo..o   //
// 8'   888   `8      .888.      d8P'    `Y8 `888   .8P'  d8P'    `Y8   //
//      888          .8"888.     Y88bo.       888  d8'    Y88bo.        //
//      888         .8' `888.     `"Y8888o.   88888[       `"Y8888o.    //
//      888        .88ooo8888.         `"Y88b  888`88b.         `"Y88b  //
//      888       .8'     `888.  oo     .d8P  888  `88b.  oo     .d8P   //
//     o888o     o88o      o8888o 8""88888P'  o888o  o888o 8""88888P'   //  
// AUDIT 1

#include <globals.h>
#if !OTA_APP // POCKETMAGE_OS
static constexpr const char* TAG = "CALENDAR"; // Tag for all calls to ESP_LOG

enum CalendarState { WEEK, MONTH, NEW_EVENT, VIEW_EVENT, SUN, MON, TUE, WED, THU, FRI, SAT };
CalendarState CurrentCalendarState = MONTH;

static String currentLine = "";

int monthOffsetCount = 0;
int weekOffsetCount = 0;

int currentDate = 0;
int currentMonth = 0;
int currentYear = 0;

// New Event
int newEventState = 0;
int editingEventIndex = 0;
String newEventName = "";
String newEventStartDate = "";
String newEventStartTime = "";
String newEventDuration = "";
String newEventRepeat = "";
String newEventNote = "";

std::vector<std::vector<String>> dayEvents;
std::vector<std::vector<String>> calendarEvents;

// Helper to format timePrompt integer output into "HH:MM" string
inline String formatTimeInt(int t) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", t / 100, t % 100);
  return String(buf);
}

// Helper to format YYYYMMDD string to DD/MM/YYYY for the display
inline String formatDateDisplay(String yyyymmdd) {
  if (yyyymmdd.length() != 8) return yyyymmdd;
  return yyyymmdd.substring(6, 8) + "/" + yyyymmdd.substring(4, 6) + "/" + yyyymmdd.substring(0, 4);
}

void updateEventArray();
void sortEventsByDate(std::vector<std::vector<String>> &calendarEvents);

void CALENDAR_INIT() {
  currentLine = "";
  CurrentAppState = CALENDAR;
  CurrentCalendarState = MONTH;
  KB().setKeyboardState(NORMAL);
  monthOffsetCount = 0;
  weekOffsetCount = 0;

  updateEventArray();
  sortEventsByDate(calendarEvents);

  newState = true;
}

#pragma region Event Data Management
void updateEventArray() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* eventsFile = "/sys/events.txt";

  // If the file doesn't exist, create it safely
  if (!global_fs->exists(eventsFile)) {
    File f = global_fs->open(eventsFile, FILE_WRITE);
    if (f) f.close();
  }

  File file = global_fs->open(eventsFile, "r"); 
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading: %s", file.path());
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    SDActive = false;
    return;
  }

  calendarEvents.clear(); // Clear the existing vector before loading the new data

  // Loop through the file, line by line
  while (file.available()) {
    String line = file.readStringUntil('\n');  
    line.trim();  
    
    // Skip empty lines
    if (line.length() == 0) {
      continue;
    }

    uint8_t delimiterPos1 = line.indexOf('|');
    uint8_t delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);
    uint8_t delimiterPos3 = line.indexOf('|', delimiterPos2 + 1);
    uint8_t delimiterPos4 = line.indexOf('|', delimiterPos3 + 1);
    uint8_t delimiterPos5 = line.indexOf('|', delimiterPos4 + 1);

    if (delimiterPos1 == -1 || delimiterPos5 == -1) continue; // Basic validation

    String eventName  = line.substring(0, delimiterPos1);
    String startDate   = line.substring(delimiterPos1 + 1, delimiterPos2);
    String startTime  = line.substring(delimiterPos2 + 1, delimiterPos3);
    String duration = line.substring(delimiterPos3 + 1, delimiterPos4);
    String repeat = line.substring(delimiterPos4 + 1, delimiterPos5);
    String note = line.substring(delimiterPos5 + 1);

    // Add the event to the vector
    calendarEvents.push_back({eventName, startDate, startTime, duration, repeat, note});
  }

  file.close();  

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

void sortEventsByDate(std::vector<std::vector<String>> &calendarEvents) {
  std::sort(calendarEvents.begin(), calendarEvents.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; 
  });
}

void updateEventsFile() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);
  
  const char* tempFile = "/sys/events.tmp";
  const char* eventsFile = "/sys/events.txt";

  File file = global_fs->open(tempFile, FILE_WRITE);
  if (file) {
    for (size_t i = 0; i < calendarEvents.size(); i++) {
      file.print(calendarEvents[i][0]); file.print("|");
      file.print(calendarEvents[i][1]); file.print("|");
      file.print(calendarEvents[i][2]); file.print("|");
      file.print(calendarEvents[i][3]); file.print("|");
      file.print(calendarEvents[i][4]); file.print("|");
      file.println(calendarEvents[i][5]); 
    }
    file.close();

    if (global_fs->exists(tempFile)) {
      PM_SDAUTO().deleteFile(*global_fs, eventsFile);
      PM_SDAUTO().renameFile(*global_fs, tempFile, eventsFile);
    }
  } else {
    OLED().sysMessage("SAVE FAILED!",1000);
  }

  if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;
}

void addEvent(String eventName, String startDate, String startTime , String duration, String repeat, String note) {
  calendarEvents.push_back({eventName, startDate, startTime , duration, repeat, note});
  sortEventsByDate(calendarEvents);
  updateEventsFile();
}

void deleteEvent(int index) {
  if (index >= 0 && index < calendarEvents.size()) {
    calendarEvents.erase(calendarEvents.begin() + index);
  }
}

void deleteEventByIndex(int indexToDelete) {
  if (indexToDelete >= 0 && indexToDelete < dayEvents.size()) {
    std::vector<String> targetEvent = dayEvents[indexToDelete];

    // Remove from dayEvents
    dayEvents.erase(dayEvents.begin() + indexToDelete);

    // Remove matching event from calendarEvents
    for (int i = 0; i < calendarEvents.size(); i++) {
      if (calendarEvents[i] == targetEvent) {
        calendarEvents.erase(calendarEvents.begin() + i);
        break;  // Only remove the first match
      }
    }
  }
}

void updateEventByIndex(int indexToUpdate) {
  if (indexToUpdate >= 0 && indexToUpdate < dayEvents.size()) {
    std::vector<String> oldEvent = dayEvents[indexToUpdate];

    // New event data
    std::vector<String> updatedEvent = {
      newEventName,
      newEventStartDate,
      newEventStartTime,
      newEventDuration,
      newEventRepeat,
      newEventNote
    };

    // Update dayEvents
    dayEvents[indexToUpdate] = updatedEvent;

    // Find and update matching event in calendarEvents
    for (int i = 0; i < calendarEvents.size(); i++) {
      if (calendarEvents[i] == oldEvent) {
        calendarEvents[i] = updatedEvent;
        break;  // Stop after first match
      }
    }
  }
}

#pragma region General Functions
String intToYYYYMMDD(int year_, int month_, int date_) {
  String y = String(year_);
  String m = (month_ < 10 ? "0" : "") + String(month_);
  String d = (date_ < 10 ? "0" : "") + String(date_);
  return y + m + d;
}

String getMonthName(int month) {
  switch(month) {
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default: return "ERR";
  }
}

int getDayOfWeek(int year, int month, int day) {
  if (month < 3) {
    month += 12;
    year -= 1;
  }

  int K = year % 100;
  int J = year / 100;

  int h = (day + 13*(month + 1)/5 + K + K/4 + J/4 + 5*J) % 7;

  // Convert Zeller’s output to: 0 = Sunday, ..., 6 = Saturday
  int d = (h + 6) % 7;
  return d;
}

int stringToPositiveInt(String input) {
  input.trim();
  if (input.length() == 0) return -1;

  for (int i = 0; i < input.length(); i++) {
    if (!isDigit(input[i])) return -1;
  }

  return input.toInt();
}

int daysInMonth(int year, int month) {
  if (month == 2) {
    // Leap year
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;
  } else {
    return 31;
  }
}

void invertOledRect(int x, int y, int w, int h) {
  u8g2.setDrawColor(2);         // 2 sets the drawing mode to XOR
  u8g2.drawBox(x, y, w, h);     // Draw the box (inverts the pixels)
  u8g2.setDrawColor(1);         // Return to standard solid drawing mode
}

String repeatPrompt(String startDateStr) {
  int year  = startDateStr.substring(0, 4).toInt();
  int month = startDateStr.substring(4, 6).toInt();
  int day   = startDateStr.substring(6, 8).toInt();
  int dow   = getDayOfWeek(year, month, day);
  int ordinal = ((day - 1) / 7) + 1;

  const char* dowNames[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
  const char* monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

  // Helper for 1st, 2nd, 3rd, 4th
  auto getOrdinalSuffix = [](int n) {
      if(n == 1) return "st";
      if(n == 2) return "nd";
      if(n == 3) return "rd";
      return "th";
  };

  // --- STEP 1: Select Repeat Mode ---
  int mode = 0; // 0:NO, 1:DAILY, 2:WEEKLY, 3:MONTHLY, 4:YEARLY
  const char* modes[] = {"No Repeat", "Daily", "Weekly", "Monthly", "Yearly"};

  KB().setKeyboardState(NORMAL);
  keypad.flush();

  while(true) {
    u8g2.clearBuffer();
    u8g2.drawXBMP(0,0,256,32,_repeatGUI0);
    u8g2.drawTriangle(20+(54*mode),26,17+(54*mode),31,23+(54*mode),31);

    u8g2.sendBuffer();

    char c = KB().updateKeypress();
    if (c == 19 || c == 12) { // Left (Normal or Func)
      mode--; if(mode < 0) mode = 4;
    } else if (c == 21 || c == 6) { // Right (Normal or Func)
      mode++; if(mode > 4) mode = 0;
    } else if (c == 13 || c == 20 || c == 7) { // Enter / Center
      break;
    } else if (c == 8 || c == 127) { // Backspace to exit
      return "_EXIT_";
    }
    delay(10);
  }

  if (mode == 0) return "NO";
  if (mode == 1) return "DAILY";

  // --- STEP 2A: WEEKLY ---
  if (mode == 2) {
    bool days[7] = {false};
    days[dow] = true; // Auto select current day
    int cursor = dow;
    
    while(true) {
      u8g2.clearBuffer();
      u8g2.drawXBMP(0,0,256,32,_repeatGUI1);

      // Weekday Grid
      for(int i = 0; i < 7; i++) {
        int bx = 62 + (i * 28);
        int by = 3;
        if(days[i]) {
          // Selected: Invert colors
          invertOledRect(bx, by, 18, 18);
        } else {
          // Unselected: Do nothing
        }
        if(i == cursor) {
          // Cursor indicator (Triangle)
          u8g2.setDrawColor(1);
          u8g2.drawTriangle(bx+9, by+22, bx+12, by+27, bx+6, by+27);
        }
      }
      u8g2.sendBuffer();

      char c = KB().updateKeypress();
      if (c == 19 || c == 12) {
        cursor--; if(cursor < 0) cursor = 6;
      } else if (c == 21 || c == 6) {
        cursor++; if(cursor > 6) cursor = 0;
      } else if (c == 20 || c == 7 || c == 32) { // Center/Space toggles day
        days[cursor] = !days[cursor];
      } else if (c == 13) { 
        String rep = "WEEKLY ";
        for(int i=0; i<7; i++) {
          if(days[i]) rep += String(dowNames[i]);
        }
        return rep;
      } else if (c == 8 || c == 127) {
        return "_EXIT_";
      }
      delay(10);
    }
  }

  // --- STEP 2B: MONTHLY or YEARLY ---
  if (mode == 3 || mode == 4) {
    int sel = 0; // 0 = Date, 1 = Ordinal
    
    String dateStr, ordStr;
    if (mode == 3) { // Monthly text formatting
      dateStr = String(day) + getOrdinalSuffix(day);
      ordStr = String(ordinal) + getOrdinalSuffix(ordinal) + " " + dowNames[dow];
    } else { // Yearly text formatting
      String mNameStr = monthNames[month-1];
      mNameStr = mNameStr.substring(0,1) + mNameStr.substring(1);
      mNameStr.toLowerCase();
      mNameStr[0] = toupper(mNameStr[0]);
      
      dateStr = mNameStr + " " + String(day);
      ordStr = String(ordinal) + getOrdinalSuffix(ordinal) + " " + dowNames[dow] + " in " + mNameStr;
    }

    while(true) {
      u8g2.clearBuffer();
      //u8g2.setFont(u8g2_font_5x8_tf);
      //u8g2.drawStr(0, 8, mode == 3 ? "Monthly Repeat:" : "Yearly Repeat:");
      u8g2.drawXBMP(0,0,256,32,_repeatGUI2);
      u8g2.setFont(u8g2_font_ncenB10_tr);

      // Calculate X coordinates for perfectly centered text
      int dateWidth = u8g2.getStrWidth(dateStr.c_str());
      int dateX = 0 + (107 - dateWidth) / 2; // Box starts at 0, width is 107

      int ordWidth = u8g2.getStrWidth(ordStr.c_str());
      int ordX = 151 + (107 - ordWidth) / 2; // Box starts at 151, width is 107

      // Date vs Ordinal selection
      if (sel == 0) { // Date pill selected
        u8g2.drawRBox(0, 10, 107, 22, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(dateX, 26, dateStr.c_str());
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawRFrame(0, 10, 107, 22, 8);
        u8g2.drawStr(dateX, 26, dateStr.c_str());
      }

      if (sel == 1) { // Ordinal pill selected
        u8g2.drawRBox(151, 10, 106, 22, 8);
        u8g2.setDrawColor(0);
        u8g2.drawStr(ordX, 26, ordStr.c_str());
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawRFrame(151, 10, 106, 22, 8);
        u8g2.drawStr(ordX, 26, ordStr.c_str());
      }
      u8g2.sendBuffer();

      char c = KB().updateKeypress();
      if (c == 19 || c == 12) { sel = 0; }
      else if (c == 21 || c == 6) { sel = 1; }
      else if (c == 13 || c == 20 || c == 7) {
        if (mode == 3) {
          if (sel == 0) return "MONTHLY " + String(day);
          else return "MONTHLY " + String(ordinal) + String(dowNames[dow]);
        } else {
          if (sel == 0) {
            String dayStr = String(day);
            if (day < 10) dayStr = "0" + dayStr;
            return "YEARLY " + String(monthNames[month-1]) + dayStr;
          } else {
            return "YEARLY " + String(ordinal) + String(dowNames[dow]) + " " + String(monthNames[month-1]);
          }
        }
      } else if (c == 8 || c == 127) {
        return "_EXIT_";
      }
      delay(10);
    }
  }
  return "NO";
}

void commandSelectMonth(String command) {
  command.toLowerCase();

  const char* monthNames[] = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec"
  };

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize Stuff
    newEventState = 0;
    newEventName = "";
    newEventStartDate = intToYYYYMMDD(currentYear, currentMonth, currentDate); // Set to current viewing date
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine     = "";

    newState        = true;
    KB().setKeyboardState(NORMAL);
    return;
  }

  // Check if command starts with a 3-letter month
  else if (command.length() >= 4) {
    String prefix = command.substring(0, 3);
    String yearPart = command.substring(4);
    yearPart.trim();

    for (int i = 0; i < 12; i++) {
      if (prefix == monthNames[i]) {
        int yearInt = stringToInt(yearPart);
        if (yearInt == -1 || yearInt < 1970 || yearInt > 2200) {
          OLED().sysMessage("Invalid",500);
          return;
        }

        currentMonth = i + 1;        // 1-indexed month
        currentYear = yearInt;
        newState = true;

        // Update monthOffsetCount relative to now
        DateTime now = CLOCK().nowDT();
        int currentAbsMonth = now.year() * 12 + now.month();
        int targetAbsMonth = currentYear * 12 + currentMonth;
        monthOffsetCount = targetAbsMonth - currentAbsMonth;

        return;
      }
    }
  }

  // Check if command is in YYYYMMDD format
  else if (command.length() == 8 && stringToPositiveInt(command) != -1) {
    int year = command.substring(0, 4).toInt();
    int month = command.substring(4, 6).toInt();
    int date = command.substring(6, 8).toInt();

    if (year < 1970 || year > 2200 || month < 1 || month > 12 || date < 1 || date > daysInMonth(month, year)) {
      OLED().sysMessage("Invalid",500);
      return;
    }

    currentYear = year;
    currentMonth = month;
    currentDate = date;

    DateTime now = CLOCK().nowDT();
    int currentAbsMonth = now.year() * 12 + now.month();
    int targetAbsMonth = currentYear * 12 + currentMonth;
    monthOffsetCount = targetAbsMonth - currentAbsMonth;

    int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);

    switch (dayOfWeek) {
      case 0: CurrentCalendarState = SUN; break;
      case 1: CurrentCalendarState = MON; break;
      case 2: CurrentCalendarState = TUE; break;
      case 3: CurrentCalendarState = WED; break;
      case 4: CurrentCalendarState = THU; break;
      case 5: CurrentCalendarState = FRI; break;
      case 6: CurrentCalendarState = SAT; break;
    }

    newState        = true;
    KB().setKeyboardState(NORMAL);
    return;
  }

  // Check if user entered a numeric day (for current month)
  else {
    int intDay = stringToPositiveInt(command);
    DateTime now = CLOCK().nowDT();
    if (intDay == -1 || intDay > daysInMonth(currentMonth, currentYear)) {
      OLED().sysMessage("Invalid",500);
      return;
    }
    else {
      currentDate = intDay;

      int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);

      switch (dayOfWeek) {
        case 0: CurrentCalendarState = SUN; break;
        case 1: CurrentCalendarState = MON; break;
        case 2: CurrentCalendarState = TUE; break;
        case 3: CurrentCalendarState = WED; break;
        case 4: CurrentCalendarState = THU; break;
        case 5: CurrentCalendarState = FRI; break;
        case 6: CurrentCalendarState = SAT; break;
      }

      newState        = true;
      KB().setKeyboardState(NORMAL);
      return;
    }
  }
}

void commandSelectWeek(String command) {
  command.toLowerCase();

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize Stuff
    newEventState = 0;
    newEventName = "";
    newEventStartDate = intToYYYYMMDD(currentYear, currentMonth, currentDate); // Set to current viewing date
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine     = "";

    newState        = true;
    KB().setKeyboardState(NORMAL);
    return;
  }
  // Commands for each day
  else if (command == "sun" || command == "su") {
    CurrentCalendarState = SUN;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day()); 
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedSunday = currentSunday + TimeSpan(weekOffsetCount * 7, 0, 0, 0);

    currentDate  = viewedSunday.day();
    currentMonth = viewedSunday.month();
    currentYear  = viewedSunday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "mon" || command == "mo") {
    CurrentCalendarState = MON;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedMonday = currentSunday + TimeSpan(weekOffsetCount * 7 + 1, 0, 0, 0);

    currentDate  = viewedMonday.day();
    currentMonth = viewedMonday.month();
    currentYear  = viewedMonday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "tue" || command == "tu") {
    CurrentCalendarState = TUE;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedTuesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 2, 0, 0, 0);

    currentDate  = viewedTuesday.day();
    currentMonth = viewedTuesday.month();
    currentYear  = viewedTuesday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "wed" || command == "we") {
    CurrentCalendarState = WED;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedWednesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 3, 0, 0, 0);

    currentDate  = viewedWednesday.day();
    currentMonth = viewedWednesday.month();
    currentYear  = viewedWednesday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "thu" || command == "th") {
    CurrentCalendarState = THU;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedThursday = currentSunday + TimeSpan(weekOffsetCount * 7 + 4, 0, 0, 0);

    currentDate  = viewedThursday.day();
    currentMonth = viewedThursday.month();
    currentYear  = viewedThursday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "fri" || command == "fr") {
    CurrentCalendarState = FRI;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedFriday = currentSunday + TimeSpan(weekOffsetCount * 7 + 5, 0, 0, 0);

    currentDate  = viewedFriday.day();
    currentMonth = viewedFriday.month();
    currentYear  = viewedFriday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "sat" || command == "sa") {
    CurrentCalendarState = SAT;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedSaturday = currentSunday + TimeSpan(weekOffsetCount * 7 + 6, 0, 0, 0);

    currentDate  = viewedSaturday.day();
    currentMonth = viewedSaturday.month();
    currentYear  = viewedSaturday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }
}

void commandSelectDay(String command) {
  command.toLowerCase();

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize new blank event
    newEventState     = 0;
    newEventName      = "";
    newEventStartDate = intToYYYYMMDD(currentYear, currentMonth, currentDate);
    newEventStartTime = "";
    newEventDuration  = "";
    newEventRepeat    = "";
    newEventNote      = "";
    currentLine       = "";

    newState          = true;
    KB().setKeyboardState(NORMAL);
    return;
  }

  // Check if the command is a single digit referring to a specific event
  if (command.length() == 1 && isDigit(command.charAt(0))) {
    int index = command.toInt() - 1;

    if (index >= 0 && index < dayEvents.size()) {
      std::vector<String>& evt = dayEvents[index];

      editingEventIndex = index;
      newEventState     = -1;
      newEventName      = evt[0];
      newEventStartDate = evt[1];
      newEventStartTime = evt[2];
      newEventDuration  = evt[3];
      newEventRepeat    = evt[4];
      newEventNote      = evt[5];
      currentLine       = "";

      CurrentCalendarState = VIEW_EVENT;
      KB().setKeyboardState(NORMAL);
      newState             = true;
    }
  }
}

int checkEvents(String YYYYMMDD, bool countOnly = false) {
  int eventCount = 0;

  // Return -1 if input format is invalid
  if (YYYYMMDD.length() != 8) return -1;

  // Convert input to DateTime
  int year  = YYYYMMDD.substring(0, 4).toInt();
  int month = YYYYMMDD.substring(4, 6).toInt();
  int day   = YYYYMMDD.substring(6, 8).toInt();
  DateTime dt(year, month, day);

  // Define helper strings
  const char* daysOfWeek[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
  const char* monthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  String weekday = String(daysOfWeek[dt.dayOfTheWeek()]);
  String weekdayUpper = weekday;
  weekdayUpper.toUpperCase();

  String dayStr = String(day);
  String monthName = String(monthNames[month - 1]);
  String dateCode = monthName + (day < 10 ? "0" + dayStr : dayStr);
  dateCode.toUpperCase();

  int weekdayIndex = dt.dayOfTheWeek();  // 0 = Sunday
  int nthWeekday = ((day - 1) / 7) + 1;

  dayEvents.clear();  // Clear previous day's events

  // Check whether any repeat events happen on this day
  for (size_t i = 0; i < calendarEvents.size(); i++) {
    String eventDate = calendarEvents[i][1];
    String eventTime = calendarEvents[i][2];
    String repeatCode = calendarEvents[i][4];

    // Direct match
    if (eventDate == YYYYMMDD) {
      if (!countOnly) dayEvents.push_back(calendarEvents[i]);
      eventCount++;
      continue;
    }

    // Handle repeating events
    if (repeatCode != "NO") {
      repeatCode.toUpperCase();

      // Skip repeat if date is before original event date
      if (eventDate.length() == 8 && YYYYMMDD < eventDate) continue;

      // DAILY
      if (repeatCode == "DAILY") {
        if (!countOnly) dayEvents.push_back(calendarEvents[i]);
        eventCount++;
        continue;
      }

      // WEEKLY SU, MOWEFR, etc.
      if (repeatCode.startsWith("WEEKLY ")) {
        String days = repeatCode.substring(7);
        days.trim();

        for (int j = 0; j + 1 < days.length(); j += 2) {
          String codeDay = days.substring(j, j + 2);
          if (codeDay == weekdayUpper) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            break;
          }
        }
        continue;
      }

      // MONTHLY 10 or 2Tu
      if (repeatCode.startsWith("MONTHLY ")) {
        String monthlyCode = repeatCode.substring(8);

        // Monthly on specific date (e.g. 10)
        if (monthlyCode == dayStr) {
          if (!countOnly) dayEvents.push_back(calendarEvents[i]);
          eventCount++;
          continue;
        }

        // Monthly on ordinal weekday (e.g. 2Tu)
        if (monthlyCode.length() == 3) {
          int nth = monthlyCode.charAt(0) - '0';
          String codeWeekday = monthlyCode.substring(1);
          codeWeekday.toUpperCase();

          if (nth == nthWeekday && codeWeekday == weekdayUpper) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            continue;
          }
        }
      }

      // YEARLY APR22 or YEARLY 2SU APR
      if (repeatCode.startsWith("YEARLY ")) {
        String yearlyCode = repeatCode.substring(7);
        yearlyCode.toUpperCase();
        
        // Check for static date (e.g. APR22)
        if (yearlyCode == dateCode) {
          if (!countOnly) dayEvents.push_back(calendarEvents[i]);
          eventCount++;
          continue;
        }
        
        // Check for ordinal date (e.g. 2SU APR)
        if (yearlyCode.length() >= 6 && isDigit(yearlyCode[0])) {
          int nth = yearlyCode.charAt(0) - '0';
          String codeWeekday = yearlyCode.substring(1, 3);
          String codeMonth = yearlyCode.substring(4, 7);
          
          String currentMonthUpper = monthName;
          currentMonthUpper.toUpperCase();
          
          if (nth == nthWeekday && codeWeekday == weekdayUpper && codeMonth == currentMonthUpper) {
            if (!countOnly) dayEvents.push_back(calendarEvents[i]);
            eventCount++;
            continue;
          }
        }
      }
    }
  }

  // Sort by start time (HH:MM to minutes)
  if (!countOnly) {
    std::sort(dayEvents.begin(), dayEvents.end(), [](const std::vector<String>& a, const std::vector<String>& b) {
      String aTime = a[2];
      String bTime = b[2];

      int aMin = aTime.substring(0, 2).toInt() * 60 + aTime.substring(3, 5).toInt();
      int bMin = bTime.substring(0, 2).toInt() * 60 + bTime.substring(3, 5).toInt();

      return aMin < bMin;
    });
  }

  return eventCount;
}

void drawCalendarMonth(int monthOffset) {
  int GRID_X =  7;     // X offset of first cell
  int GRID_Y = 49;     // Y offset of first row
  int CELL_W = 44;     // Width of each cell
  int CELL_H = 27;     // Height of each cell

  DateTime now = CLOCK().nowDT();

  // Step 1: Calculate target month/year
  int month = now.month() + monthOffset;
  int year = now.year();
  while (month > 12) { month -= 12; year++; }
  while (month < 1)  { month += 12; year--; }

  currentMonth = month;
  currentYear = year;

  // Draw Background
  EINK().drawStatusBar(getMonthName(currentMonth) + " " + String(currentYear)+ " | Type a Date:");
  display.drawBitmap(0, 0, calendar_allArray[1], 320, 218, GxEPD_BLACK);

  // Step 2: Day of the week for the 1st of the month (0 = Sun, 6 = Sat)
  DateTime firstDay(year, month, 1);
  int startDay = firstDay.dayOfTheWeek();  // 0–6, Sun to Sat

  // Step 3: Number of days in the month
  int nextYear  = (month == 12) ? (year + 1) : year;
  int nextMonth = (month == 12) ? 1 : (month + 1);

  int daysInMonth = (DateTime(nextYear, nextMonth, 1) - DateTime(year, month, 1)).days();

  // Step 4: Blank out leading days
  for (int i = 0; i < startDay; ++i) {
    int x = GRID_X + i * CELL_W;
    int y = GRID_Y;
    display.fillRect(x, y, CELL_W, CELL_H, GxEPD_WHITE);
  }

  // Step 5: Blank out trailing days
  int totalBoxes = 42;  // 7x6 grid
  int trailingStart = startDay + daysInMonth;
  for (int i = trailingStart; i < totalBoxes; ++i) {
    int row = i / 7;
    int col = i % 7;
    int x = GRID_X + col * CELL_W;
    int y = GRID_Y + row * CELL_H;
    display.fillRect(x, y, CELL_W, CELL_H, GxEPD_WHITE);
  }
  // Step 6: Draw day numbers and events
  for (int i = 0; i < daysInMonth; ++i) {
    int dayIndex = i + startDay;     // total box index in the 7x6 grid
    int row = dayIndex / 7;
    int col = dayIndex % 7;

    int x = GRID_X + col * CELL_W;
    int y = GRID_Y + row * CELL_H;

    int dayNum = i + 1;  // 1-based day number

    // Current day
    if (dayNum == now.day() && monthOffset == 0) {
      display.setFont(&FreeSerifBold9pt7b);
    }
    else display.setFont(&FreeSerif9pt7b);
    
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x + 6, y + 15); 
    display.print(dayNum);

    // Draw icon if there are events on day
    String YYYYMMDD = intToYYYYMMDD(year, month, dayNum);

    int numEvents = checkEvents(YYYYMMDD, true);

    // Events found
    if (numEvents > 2) {
      display.setFont(&Font5x7Fixed);
      display.setCursor(x + 32, y + 16);
      display.print(String(numEvents));
    }
    else if (numEvents > 1) {
      display.drawBitmap(x + 29, y + 8, _eventMarker1, 10, 10, GxEPD_BLACK);
    }
    else if (numEvents > 0) {
      display.drawBitmap(x + 29, y + 8, _eventMarker0, 10, 10, GxEPD_BLACK);
    }
  }
}

void drawCalendarWeek(int weekOffset) {
  EINK().drawStatusBar("Type Sun, etc. or (N)ew");
  display.drawBitmap(0, 0, calendar_allArray[0], 320, 218, GxEPD_BLACK);

  // Get current date
  DateTime now = CLOCK().nowDT();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int dow = now.dayOfTheWeek();  // 0 = Sunday

  // Calculate how many days to go back to get to Sunday, adjusted by weekOffset
  int totalOffset = -dow + (weekOffset * 7);

  for (int i = 0; i < 7; i++) {
    // Compute day offset from today
    int offset = totalOffset + i;

    // Convert (year, month, day + offset) into a new date
    int y = year;
    int m = month;
    int d = day + offset;

    // Normalize date forward/backward
    while (d <= 0) {
      m--;
      if (m < 1) {
        m = 12;
        y--;
      }
      d += daysInMonth(m, y);
    }
    while (d > daysInMonth(m, y)) {
      d -= daysInMonth(m, y);
      m++;
      if (m > 12) {
        m = 1;
        y++;
      }
    }

    // Format YYYYMMDD
    String YYYYMMDD = intToYYYYMMDD(y, m, d);

    // Draw date
    display.setFont(&FreeSerif9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(9 + (i * 44), 62);
    String dateStr = String(m) + "/" + String(d);
    display.print(dateStr);

    // Load and draw events
    int eventCount = checkEvents(YYYYMMDD, false);
    if (eventCount > 6) eventCount = 6;

    // Blank out extra space
    display.fillRect(9 + (i * 44), 71 + (eventCount * 23), 39, ((6 - eventCount) * 23), GxEPD_WHITE);

    for (int j = 0; j < eventCount; j++) {
      String startTime = dayEvents[j][2];
      // Indicator for repeat events
      if (dayEvents[j][4] != "NO") startTime = ":: " + startTime;
      String eventName = dayEvents[j][0].substring(0, 6);

      // Print Start Time
      display.setFont(&Font3x7FixedNum);
      display.setTextColor(GxEPD_BLACK);
      display.setCursor(12 + (i * 44), 80 + (j * 23));
      display.print(startTime);

      // Print Event Name
      display.setFont(&Font5x7Fixed);
      display.setCursor(12 + (i * 44), 89 + (j * 23));
      display.print(eventName);
    }
  }
}

#pragma region Loops
void processKB_CALENDAR() {
  int currentMillis = millis();
  DateTime now = CLOCK().nowDT();
  char inchar = 0;

  switch (CurrentCalendarState) {
    case MONTH:
      // 1. Drain the hardware buffer continuously at loop speed
      inchar = KB().updateKeypress();

      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          if (inchar == 12) { HOME_INIT(); }  
          else if (inchar == 13) {                          
            commandSelectMonth(currentLine);
            currentLine = "";
          }                                       
          else if (inchar == 17) {
            if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(SHIFT);
          }
          else if (inchar == 18) {
            if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT) KB().setKeyboardState(NORMAL);
            else if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(FN_SHIFT);
            else KB().setKeyboardState(FUNC);
          }
          else if (inchar == 32) { currentLine += " "; }
          else if (inchar == 8) {                  
            if (currentLine.length() > 0) currentLine.remove(currentLine.length() - 1);
          }
          else if (inchar == 19) {
            monthOffsetCount--;
            newState = true;
          }
          else if (inchar == 21) {
            monthOffsetCount++;
            newState = true;
          }
          else if (inchar == 20 || inchar == 7) {
            CurrentCalendarState = WEEK;
            KB().setKeyboardState(NORMAL);
            newState = true;
            delay(200);
            break;
          }
          else {
            currentLine += inchar;
            if (inchar >= 48 && inchar <= 57) {}  
            else if (KB().getKeyboardState() != NORMAL) KB().setKeyboardState(NORMAL);
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        OLED().oledLine(currentLine, currentLine.length(), false);
      }
      break;

    case WEEK:
      // 1. Drain the hardware buffer continuously at loop speed
      inchar = KB().updateKeypress();

      // 2. Only process the actual input if the cooldown has expired
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          if (inchar == 12) { HOME_INIT(); }  
          else if (inchar == 13) {                          
            commandSelectWeek(currentLine);
            currentLine = "";
          }                                       
          else if (inchar == 17) {                                  
            if (KB().getKeyboardState() == SHIFT) KB().setKeyboardState(NORMAL);
            else KB().setKeyboardState(SHIFT);
          }
          else if (inchar == 18) {                                  
            if (KB().getKeyboardState() == FUNC) KB().setKeyboardState(NORMAL);
            else KB().setKeyboardState(FUNC);
          }
          else if (inchar == 32) { currentLine += " "; }
          else if (inchar == 8) {                  
            if (currentLine.length() > 0) currentLine.remove(currentLine.length() - 1);
          }
          else if (inchar == 19) {
            weekOffsetCount--;
            newState = true;
          }
          else if (inchar == 21) {
            weekOffsetCount++;
            newState = true;
          }
          else if (inchar == 20 || inchar == 7) {
            CurrentCalendarState = MONTH;
            KB().setKeyboardState(NORMAL);
            newState = true;
            delay(200);
            break;
          }
          else {
            currentLine += inchar;
            if (inchar >= 48 && inchar <= 57) {}  
            else if (KB().getKeyboardState() != NORMAL) KB().setKeyboardState(NORMAL);
          }
        }
      }

      // 3. Update OLED at true OLED_MAX_FPS, completely independent of keyboard bounce
      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        OLED().oledLine(currentLine, currentLine.length(), false);
      }
      break;

    case NEW_EVENT:
      if (newEventState == 0) {
        KB().setKeyboardState(NORMAL);
        String input = textPrompt("Enter Event Name:");
        if (input == "_RETURN_") return;
        else if (input != "_EXIT_") { 
          newEventName = input; 
          newEventState++; 
          newState = true; delay(50); 
        } else { 
          CurrentCalendarState = MONTH; newState = true; 
        }
      }
      else if (newEventState == 1) {
        String uiDate = datePrompt(newEventStartDate); // pass default
        newEventStartDate = uiDate.substring(6, 10) + uiDate.substring(3, 5) + uiDate.substring(0, 2);
        newEventState++; newState = true; delay(50);
      }
      else if (newEventState == 2) {
        int defaultT = -1;
        if (newEventStartTime.length() == 5) {
            defaultT = newEventStartTime.substring(0,2).toInt() * 100 + newEventStartTime.substring(3,5).toInt();
        }
        int t = timePrompt(defaultT); // pass default
        newEventStartTime = formatTimeInt(t);
        newEventState++; newState = true; delay(50);
      }
      else if (newEventState == 3) {
        int defaultDur = -1;
        if (newEventDuration.length() == 5) {
            defaultDur = newEventDuration.substring(0,2).toInt() * 100 + newEventDuration.substring(3,5).toInt();
        }
        int dur = timePrompt(defaultDur); // pass default
        newEventDuration = formatTimeInt(dur);
        newEventState++; newState = true; delay(50);
      }
      else if (newEventState == 4) {
        String code = repeatPrompt(newEventStartDate);
        if (code == "_EXIT_") { 
          CurrentCalendarState = MONTH; 
          newState = true; 
        } else {
          newEventRepeat = code;
          newEventState++; 
          newState = true; 
          delay(50);
        }
      }
      else if (newEventState == 5) {
        KB().setKeyboardState(NORMAL);
        String note = textPrompt("Attach Note:");
        if (note == "_RETURN_") return;
        else if (note != "_EXIT_") {
          newEventNote = note;
          addEvent(newEventName, newEventStartDate, newEventStartTime, newEventDuration, newEventRepeat, newEventNote);
          OLED().sysMessage("Event Created!",1000);
          CurrentCalendarState = MONTH;
          newState = true;
        } else {
          CurrentCalendarState = MONTH; newState = true;
        }
      }
      break;

    case VIEW_EVENT:
      // Force FUNC state before draining buffer
      KB().setKeyboardState(FUNC); 
      inchar = KB().updateKeypress();

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          if (inchar == 12 || inchar == 8 || inchar == 127) { // 12 is Left Arrow in FUNC, 8 is BKSP
            CurrentCalendarState = MONTH;
            newState = true;
          }  
          else if (inchar == '1') {
            String input = textPrompt("Edit Event Name:");
            if (input == "_RETURN_") return;
            else if (input != "_EXIT_") { newEventName = input; newState = true; }
          }
          else if (inchar == '2') {
            String uiDate = datePrompt(newEventStartDate); 
            if (uiDate != "_EXIT_" && uiDate.length() > 0) {
                newEventStartDate = uiDate.substring(6, 10) + uiDate.substring(3, 5) + uiDate.substring(0, 2);
                newState = true;
            }
          }
          else if (inchar == '3') {
            int defaultT = -1;
            if (newEventStartTime.length() == 5) {
                defaultT = newEventStartTime.substring(0,2).toInt() * 100 + newEventStartTime.substring(3,5).toInt();
            }
            int t = timePrompt(defaultT); 
            newEventStartTime = formatTimeInt(t);
            newState = true;
          }
          else if (inchar == '4') {
            int defaultDur = -1;
            if (newEventDuration.length() == 5) {
                defaultDur = newEventDuration.substring(0,2).toInt() * 100 + newEventDuration.substring(3,5).toInt();
            }
            int dur = timePrompt(defaultDur); 
            newEventDuration = formatTimeInt(dur);
            newState = true;
          }
          else if (inchar == '5') {
            String code = repeatPrompt(newEventStartDate);
            if (code != "_EXIT_") {
              newEventRepeat = code;
              newState = true;
            }
          }
          else if (inchar == '6') {
            String note = textPrompt("Edit Note:");
            if (note == "_RETURN_") return;
            else if (note != "_EXIT_") { newEventNote = note; newState = true; }
          }
          else if (inchar == '$') { // 'd' in FUNC layer
            if (boolPrompt("Delete Event?") == 1) {
              deleteEventByIndex(editingEventIndex);
              updateEventsFile();
              OLED().sysMessage("Event Deleted",1000);
              CurrentCalendarState = MONTH;
              newState = true;
            }
          }
          else if (inchar == '!') { // 's' in FUNC layer
            updateEventByIndex(editingEventIndex);
            updateEventsFile();
            OLED().sysMessage("Event Saved",1000);
            CurrentCalendarState = MONTH;
            newState = true;
          }
        }
      }

      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        // Make sure we only draw this if we didn't just exit the view state!
        if (CurrentCalendarState == VIEW_EVENT) {
            OLED().oledLine("", 0, false, "Type 1-6,(D)el, or (S)ave");
        }
      }
      break;

    case SUN:
    case MON:
    case TUE:
    case WED:
    case THU:
    case FRI:
    case SAT:
      // Force FUNC state before draining buffer
      KB().setKeyboardState(FUNC); 
      inchar = KB().updateKeypress();

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        if (inchar != 0) {
          KBBounceMillis = currentMillis;

          if (inchar == 8) { // BKSP
            CurrentCalendarState = MONTH;
            currentLine = "";
            newState = true;
          }  
          else if (inchar == '/' || (inchar >= '1' && inchar <= '9')) {
            if (inchar == '/') inchar = 'n'; // '/' is 'n' in FUNC layer
            commandSelectDay(String(inchar));
          }
          else if (inchar == 12) { // 12 is Left Arrow in FUNC layer
            // Go back one day
            currentDate--;
            if (currentDate < 1) {
              currentMonth--;
              if (currentMonth < 1) {
                currentMonth = 12;
                currentYear--;
              }
              currentDate = daysInMonth(currentMonth, currentYear);
            }

            int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);
            switch (dayOfWeek) {
              case 0: CurrentCalendarState = SUN; break;
              case 1: CurrentCalendarState = MON; break;
              case 2: CurrentCalendarState = TUE; break;
              case 3: CurrentCalendarState = WED; break;
              case 4: CurrentCalendarState = THU; break;
              case 5: CurrentCalendarState = FRI; break;
              case 6: CurrentCalendarState = SAT; break;
            }
            newState = true;
          }
          else if (inchar == 6) { // 6 is Right Arrow in FUNC layer
            // Go forward one day
            int daysThisMonth = daysInMonth(currentMonth, currentYear);
            currentDate++;
            if (currentDate > daysThisMonth) {
              currentDate = 1;
              currentMonth++;
              if (currentMonth > 12) {
                currentMonth = 1;
                currentYear++;
              }
            }

            int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);
            switch (dayOfWeek) {
              case 0: CurrentCalendarState = SUN; break;
              case 1: CurrentCalendarState = MON; break;
              case 2: CurrentCalendarState = TUE; break;
              case 3: CurrentCalendarState = WED; break;
              case 4: CurrentCalendarState = THU; break;
              case 5: CurrentCalendarState = FRI; break;
              case 6: CurrentCalendarState = SAT; break;
            }
            newState = true;
          }
          else if (inchar == 7) { // 7 is Center Key in FUNC layer
            CurrentCalendarState = WEEK;
            newState = true;
            delay(200);
            break;
          }
        }
      }

      currentMillis = millis();
      if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
        OLEDFPSMillis = currentMillis;
        // Make sure we only draw this if we didn't just jump to another menu!
        if (CurrentCalendarState == SUN || CurrentCalendarState == MON || 
            CurrentCalendarState == TUE || CurrentCalendarState == WED || 
            CurrentCalendarState == THU || CurrentCalendarState == FRI || 
            CurrentCalendarState == SAT) {
          OLED().oledLine("", 0, false);
        }
      }
      break;
  }
}

void einkHandler_CALENDAR() {
  switch (CurrentCalendarState) {
    case WEEK:
      if (newState) {
        newState = false;
        EINK().resetDisplay();
        drawCalendarWeek(weekOffsetCount);
        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
      
    case MONTH:
      if (newState) {
        newState = false;
        EINK().resetDisplay();
        drawCalendarMonth(monthOffsetCount);
        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
      
    case NEW_EVENT:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        display.drawBitmap(0, 0, calendar_allArray[2], 320, 218, GxEPD_BLACK);
        display.setFont(&FreeSerif9pt7b);

        display.setCursor(106, 68);
        display.print(newEventName);

        display.setCursor(106, 90);
        display.print(formatDateDisplay(newEventStartDate));

        display.setCursor(106, 112);
        display.print(newEventStartTime);

        display.setCursor(106, 134);
        display.print(newEventDuration);
        
        display.setCursor(106, 156);
        display.print(newEventRepeat);

        display.setCursor(106, 178);
        display.print(newEventNote);

        switch (newEventState) {
          case 0: EINK().drawStatusBar("Enter Event Name on OLED"); break;
          case 1: EINK().drawStatusBar("Set Start Date on OLED"); break;
          case 2: EINK().drawStatusBar("Set Start Time on OLED"); break;
          case 3: EINK().drawStatusBar("Set Duration on OLED"); break;
          case 4: EINK().drawStatusBar("Set Repeat Code on OLED"); break;
          case 5: EINK().drawStatusBar("Attach Note on OLED"); break;
        }

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
      
    case VIEW_EVENT:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        EINK().drawStatusBar("Type 1-6,(D)el, or (S)ave");
        display.drawBitmap(0, 0, calendar_allArray[3], 320, 218, GxEPD_BLACK);
        display.setFont(&FreeSerif9pt7b);

        display.setCursor(106, 68);
        display.print(newEventName);

        display.setCursor(106, 90);
        display.print(formatDateDisplay(newEventStartDate));

        display.setCursor(106, 112);
        display.print(newEventStartTime);

        display.setCursor(106, 134);
        display.print(newEventDuration);
        
        display.setCursor(106, 156);
        display.print(newEventRepeat);

        display.setCursor(106, 178);
        display.print(newEventNote);

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
      
    case SUN:
    case MON:
    case TUE:
    case WED:
    case THU:
    case FRI:
    case SAT:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        EINK().drawStatusBar("Events 1-7 or (N)ew");
        display.drawBitmap(0, 0, calendar_allArray[CurrentCalendarState], 320, 218, GxEPD_BLACK);

        display.setFont(&FreeSerif9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(9 + (44*(CurrentCalendarState - 4)), 59);
        display.print(String(currentMonth) + "/" + String(currentDate));

        String YYYYMMDD = intToYYYYMMDD(currentYear, currentMonth, currentDate);
        int eventCount = checkEvents(YYYYMMDD, false);
        if (eventCount > 7) eventCount = 7;

        display.fillRect(12, 66 + (eventCount * 19), 297, ((7 - eventCount) * 19), GxEPD_WHITE);
        
        for (int j = 0; j < eventCount; j++) {
          String name       = dayEvents[j][0];
          String startTime  = dayEvents[j][2];
          String duration   = dayEvents[j][3];
          String repeatCode = dayEvents[j][4];
          String bottomInfo = "Starts: " + startTime + ", Dur: " + duration + ", Rep: " + repeatCode;

          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 74 + (j * 19));
          display.print(name);

          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 82 + (j * 19));
          display.print(bottomInfo);
        }

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
      }
      break;
  }
}
#endif