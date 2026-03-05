
#include <globals.h>
#include <time.h>

#if !OTA_APP                                    // POCKETMAGE_OS
static constexpr const char* TAG = "CALENDAR";  // Tag for all calls to ESP_LOG

// function prototypes
int daysInMonth(int year, int month);
void updateEventArray();

enum CalendarState { WEEK, MONTH, NEW_EVENT, VIEW_EVENT, SUN, MON, TUE, WED, THU, FRI, SAT };
CalendarState CurrentCalendarState = MONTH;

struct CalendarEvent {
  String eventName;
  int year, month, day;
  int hour, minute;
  int durationMinutes;
  String repeat;
  String note;

  // stores original index in calendarEvents, for when it's copied into day events
  int calendarIndex = -1;

  // Compare all fields
  bool operator==(const CalendarEvent& other) const {
    return eventName == other.eventName && year == other.year && month == other.month &&
           day == other.day && hour == other.hour && minute == other.minute &&
           durationMinutes == other.durationMinutes && repeat == other.repeat && note == other.note;
  }
};

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

std::vector<CalendarEvent> dayEvents;
std::vector<CalendarEvent> calendarEvents;

void CALENDAR_INIT() {
  currentLine = "";
  CurrentAppState = CALENDAR;
  CurrentCalendarState = MONTH;
  KB().setKeyboardState(NORMAL);
  newState = true;
  monthOffsetCount = 0;
  weekOffsetCount = 0;
}

// General Functions
// Convert "SU", "MO", "TU", etc. to 0..6 (Sun=0)
int dayStringToInt(const String& day) {
  if (day == "SU")
    return 0;
  if (day == "MO")
    return 1;
  if (day == "TU")
    return 2;
  if (day == "WE")
    return 3;
  if (day == "TH")
    return 4;
  if (day == "FR")
    return 5;
  if (day == "SA")
    return 6;
  return -1;  // invalid
}
String repeatCodeToICS(const String& repeat) {
  String r = repeat;
  r.toUpperCase();
  if (r == "NO")
    return "";  // no recurrence
  if (r == "DAILY")
    return "FREQ=DAILY";
  if (r.startsWith("WEEKLY")) {
    // Expect "WEEKLY SU", "WEEKLY MO,WE" etc
    String days = r.substring(7);
    days.trim();
    return "FREQ=WEEKLY;BYDAY=" + days;
  }
  if (r.startsWith("MONTHLY")) {
    // Could be "MONTHLY 10" (day) or "MONTHLY 2TU" (nth weekday)
    return "FREQ=MONTHLY;BYMONTHDAY=" + r.substring(8);  // basic, extend later
  }
  if (r.startsWith("YEARLY")) {
    return "FREQ=YEARLY";  // extend later with BYMONTH/BYMONTHDAY
  }
  return "";  // fallback
}
String ICSRRULEtoApp(const String& rrule) {
  if (rrule.length() == 0)
    return "NO";

  String r = rrule;
  r.toUpperCase();

  if (r.startsWith("FREQ=DAILY"))
    return "DAILY";

  if (r.startsWith("FREQ=WEEKLY")) {
    int byDayIdx = r.indexOf("BYDAY=");
    if (byDayIdx >= 0) {
      String days = r.substring(byDayIdx + 6);
      days.trim();
      return "WEEKLY " + days;
    }
    return "WEEKLY";
  }

  if (r.startsWith("FREQ=MONTHLY")) {
    int byMonthDayIdx = r.indexOf("BYMONTHDAY=");
    if (byMonthDayIdx >= 0) {
      String day = r.substring(byMonthDayIdx + 11);
      day.trim();
      return "MONTHLY " + day;
    }
    return "MONTHLY";
  }

  if (r.startsWith("FREQ=YEARLY"))
    return "YEARLY";

  return "NO";  // fallback
}

// Parse YYYYMMDD string to DateTime
DateTime parseYYYYMMDD(const String& yyyymmdd) {
  int year = yyyymmdd.substring(0, 4).toInt();
  int month = yyyymmdd.substring(4, 6).toInt();
  int day = yyyymmdd.substring(6, 8).toInt();
  return DateTime(year, month, day);
}

// Compare two times in HHMM or HH:MM format
bool isAfter(const String& startTime, const String& currentTime) {
  String s = startTime;
  String c = currentTime;
  s.replace(":", "");
  c.replace(":", "");
  return s.toInt() >= c.toInt();
}

// Event Data Management

// #pragma message "TODO: Migrate to a better/global file management system"

// General Functions
String intToYYYYMMDD(int year_, int month_, int date_) {
  String y = String(year_);
  String m = (month_ < 10 ? "0" : "") + String(month_);
  String d = (date_ < 10 ? "0" : "") + String(date_);
  return y + m + d;
}

String getMonthName(int month) {
  switch (month) {
    case 1:
      return "Jan";
    case 2:
      return "Feb";
    case 3:
      return "Mar";
    case 4:
      return "Apr";
    case 5:
      return "May";
    case 6:
      return "Jun";
    case 7:
      return "Jul";
    case 8:
      return "Aug";
    case 9:
      return "Sep";
    case 10:
      return "Oct";
    case 11:
      return "Nov";
    case 12:
      return "Dec";
    default:
      return "ERR";
  }
}

int getDayOfWeek(int year, int month, int day) {
  if (month < 3) {
    month += 12;
    year -= 1;
  }

  int K = year % 100;
  int J = year / 100;

  int h = (day + 13 * (month + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;

  // Convert Zeller’s output to: 0 = Sunday, ..., 6 = Saturday
  int d = (h + 6) % 7;
  return d;
}

int stringToPositiveInt(String input) {
  input.trim();
  if (input.length() == 0)
    return -1;

  for (int i = 0; i < input.length(); i++) {
    if (!isDigit(input[i]))
      return -1;
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

void commandSelectMonth(String command) {
  command.toLowerCase();

  const char* monthNames[] = {"jan", "feb", "mar", "apr", "may", "jun",
                              "jul", "aug", "sep", "oct", "nov", "dec"};

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize Stuff
    newEventState = 0;
    newEventName = "";
    newEventStartDate = "";
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine = "";

    newState = true;
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
          OLED().oledWord("Invalid");
          delay(500);
          return;
        }

        currentMonth = i + 1;  // 1-indexed month
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

    if (year < 1970 || year > 2200 || month < 1 || month > 12 || date < 1 ||
        date > daysInMonth(month, year)) {
      OLED().oledWord("Invalid");
      delay(500);
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
      case 0:
        CurrentCalendarState = SUN;
        break;
      case 1:
        CurrentCalendarState = MON;
        break;
      case 2:
        CurrentCalendarState = TUE;
        break;
      case 3:
        CurrentCalendarState = WED;
        break;
      case 4:
        CurrentCalendarState = THU;
        break;
      case 5:
        CurrentCalendarState = FRI;
        break;
      case 6:
        CurrentCalendarState = SAT;
        break;
    }

    newState = true;
    KB().setKeyboardState(NORMAL);
    return;
  }

  // Check if user entered a numeric day (for current month)
  else {
    int intDay = stringToPositiveInt(command);
    DateTime now = CLOCK().nowDT();
    if (intDay == -1 || intDay > daysInMonth(currentMonth, currentYear)) {
      OLED().oledWord("Invalid");
      delay(500);
      return;
    } else {
      currentDate = intDay;

      int dayOfWeek = getDayOfWeek(currentYear, currentMonth, currentDate);

      switch (dayOfWeek) {
        case 0:
          CurrentCalendarState = SUN;
          break;
        case 1:
          CurrentCalendarState = MON;
          break;
        case 2:
          CurrentCalendarState = TUE;
          break;
        case 3:
          CurrentCalendarState = WED;
          break;
        case 4:
          CurrentCalendarState = THU;
          break;
        case 5:
          CurrentCalendarState = FRI;
          break;
        case 6:
          CurrentCalendarState = SAT;
          break;
      }

      newState = true;
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
    newEventStartDate = "";
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine = "";

    newState = true;
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

    currentDate = viewedSunday.day();
    currentMonth = viewedSunday.month();
    currentYear = viewedSunday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "mon" || command == "mo") {
    CurrentCalendarState = MON;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedMonday = currentSunday + TimeSpan(weekOffsetCount * 7 + 1, 0, 0, 0);

    currentDate = viewedMonday.day();
    currentMonth = viewedMonday.month();
    currentYear = viewedMonday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "tue" || command == "tu") {
    CurrentCalendarState = TUE;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedTuesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 2, 0, 0, 0);

    currentDate = viewedTuesday.day();
    currentMonth = viewedTuesday.month();
    currentYear = viewedTuesday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "wed" || command == "we") {
    CurrentCalendarState = WED;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedWednesday = currentSunday + TimeSpan(weekOffsetCount * 7 + 3, 0, 0, 0);

    currentDate = viewedWednesday.day();
    currentMonth = viewedWednesday.month();
    currentYear = viewedWednesday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "thu" || command == "th") {
    CurrentCalendarState = THU;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedThursday = currentSunday + TimeSpan(weekOffsetCount * 7 + 4, 0, 0, 0);

    currentDate = viewedThursday.day();
    currentMonth = viewedThursday.month();
    currentYear = viewedThursday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "fri" || command == "fr") {
    CurrentCalendarState = FRI;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedFriday = currentSunday + TimeSpan(weekOffsetCount * 7 + 5, 0, 0, 0);

    currentDate = viewedFriday.day();
    currentMonth = viewedFriday.month();
    currentYear = viewedFriday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }

  else if (command == "sat" || command == "sa") {
    CurrentCalendarState = SAT;

    DateTime now = CLOCK().nowDT();
    int todayDOW = getDayOfWeek(now.year(), now.month(), now.day());
    DateTime currentSunday = now - TimeSpan(todayDOW, 0, 0, 0);
    DateTime viewedSaturday = currentSunday + TimeSpan(weekOffsetCount * 7 + 6, 0, 0, 0);

    currentDate = viewedSaturday.day();
    currentMonth = viewedSaturday.month();
    currentYear = viewedSaturday.year();

    newState = true;
    KB().setKeyboardState(NORMAL);
  }
}

void commandSelectDay(String command) {
  command.toLowerCase();

  if (command == "n") {
    CurrentCalendarState = NEW_EVENT;

    // Initialize new blank event
    newEventState = 0;
    newEventName = "";
    newEventStartDate = intToYYYYMMDD(currentYear, currentMonth, currentDate);
    newEventStartTime = "";
    newEventDuration = "";
    newEventRepeat = "";
    newEventNote = "";
    currentLine = "";

    newState = true;
    KB().setKeyboardState(NORMAL);
    return;
  }

  // Check if the command is a single digit referring to a specific event
  if (command.length() == 1 && isDigit(command.charAt(0))) {
    int index = command.toInt() - 1;

    if (index >= 0 && index < dayEvents.size()) {
      CalendarEvent& evt = dayEvents[index];  // Use struct fields

      editingEventIndex = index;
      newEventState = -1;
      newEventName = evt.eventName;

      // Reconstruct YYYYMMDD string for newEventStartDate
      char dateBuf[9];
      snprintf(dateBuf, sizeof(dateBuf), "%04d%02d%02d", evt.year, evt.month, evt.day);
      newEventStartDate = String(dateBuf);

      // Reconstruct HHMM string for newEventStartTime
      char timeBuf[6];  // needs to be 6 bc last byte of string is null terminator \0
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", evt.hour, evt.minute);
      newEventStartTime = String(timeBuf);

      newEventDuration = String(evt.durationMinutes);
      newEventRepeat = evt.repeat;
      newEventNote = evt.note;
      currentLine = "";

      CurrentCalendarState = VIEW_EVENT;
      KB().setKeyboardState(NORMAL);
      newState = true;
    }
  }
}
// // Calculate duration in minutes between two ICS datetimes (YYYYMMDDTHHMMSS)

time_t parseICS(const String& dt) {
  struct tm t;

  t.tm_year = dt.substring(0, 4).toInt() - 1900;
  t.tm_mon = dt.substring(4, 6).toInt() - 1;
  t.tm_mday = dt.substring(6, 8).toInt();
  t.tm_hour = dt.substring(9, 11).toInt();
  t.tm_min = dt.substring(11, 13).toInt();
  t.tm_sec = dt.substring(13, 15).toInt();
  t.tm_isdst = -1;

  return mktime(&t);
}

int calculateDurationMinutes(const String& start, const String& end) {
  time_t t1 = parseICS(start);
  time_t t2 = parseICS(end);

  long diff = difftime(t2, t1);
  if (diff < 0)
    return 0;

  return diff / 60;
}

// int calculateDurationMinutes(const String& dtStart, const String& dtEnd) {
//     // Parse start
//     int sYear  = dtStart.substring(0,4).toInt();
//     int sMonth = dtStart.substring(4,6).toInt();
//     int sDay   = dtStart.substring(6,8).toInt();
//     int sHour  = dtStart.substring(9,11).toInt();
//     int sMin   = dtStart.substring(11,13).toInt();

//     // Parse end
//     int eYear  = dtEnd.substring(0,4).toInt();
//     int eMonth = dtEnd.substring(4,6).toInt();
//     int eDay   = dtEnd.substring(6,8).toInt();
//     int eHour  = dtEnd.substring(9,11).toInt();
//     int eMin   = dtEnd.substring(11,13).toInt();

//     // Convert to minutes since epoch (simple)
//     long startMinutes = (((sYear*12 + sMonth)*31 + sDay)*24 + sHour)*60 + sMin;
//     long endMinutes   = (((eYear*12 + eMonth)*31 + eDay)*24 + eHour)*60 + eMin;

//     long diff = endMinutes - startMinutes;
//     if (diff < 0) diff = 0;
//     return (int)diff;
// }

bool isOnOrAfter(const String& date, const String& startDate) {
  return date >= startDate;  // works because YYYYMMDD
}

int checkEvents(const String& YYYYMMDD, bool silent) {
  dayEvents.clear();
  int count = 0;

  // Only load ICS files once per app run
  static bool calendarEventsLoaded = false;
  if (!calendarEventsLoaded) {
    updateEventArray();  // populate calendarEvents
    calendarEventsLoaded = true;
  }

  // Convert YYYYMMDD string to ints
  int y = YYYYMMDD.substring(0, 4).toInt();
  int m = YYYYMMDD.substring(4, 6).toInt();
  int d = YYYYMMDD.substring(6, 8).toInt();

  for (const auto& e : calendarEvents) {
    // Build eventDate as YYYYMMDD string
    char eventDateBuf[9];
    snprintf(eventDateBuf, sizeof(eventDateBuf), "%04d%02d%02d", e.year, e.month, e.day);
    String eventDate = String(eventDateBuf);

    String repeat = e.repeat;

    bool includeEvent = false;

    if (YYYYMMDD < eventDate) {
      continue;
    }

    if (eventDate == YYYYMMDD) {
      includeEvent = true;
    } else if (repeat.startsWith("DAILY")) {
      includeEvent = true;
    } else if (repeat.startsWith("WEEKLY")) {
      // repeat format: "WEEKLY FR" or "WEEKLY MO,WE"
      String days = repeat.substring(7);  // after "WEEKLY"
      days.trim();

      int pos = 0;
      while (pos < days.length()) {
        int comma = days.indexOf(',', pos);
        if (comma == -1)
          comma = days.length();

        String token = days.substring(pos, comma);
        token.trim();

        int dow = getDayOfWeek(y, m, d);  // 0=Sun ... 6=Sat
        const char* dows[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
        String dowStr = dows[dow];
        if (token == dowStr) {
          includeEvent = true;
          break;
        }

        pos = comma + 1;
      }
    }

    if (includeEvent) {
      dayEvents.push_back(e);  // now dayEvents should be vector<CalendarEvent>
      if (++count >= 7)
        break;
    }
  }

  if (!silent)
    ESP_LOGI(TAG, "checkEvents(): %d events on %s", count, YYYYMMDD.c_str());

  return count;
}
void drawCalendarMonth(int monthOffset) {
  int GRID_X = 7;   // X offset of first cell
  int GRID_Y = 49;  // Y offset of first row
  int CELL_W = 44;  // Width of each cell
  int CELL_H = 27;  // Height of each cell

  DateTime now = CLOCK().nowDT();

  // Step 1: Calculate target month/year
  int month = now.month() + monthOffset;
  int year = now.year();
  while (month > 12) {
    month -= 12;
    year++;
  }
  while (month < 1) {
    month += 12;
    year--;
  }

  currentMonth = month;
  currentYear = year;

  // Draw Background
  EINK().drawStatusBar(getMonthName(currentMonth) + " " + String(currentYear) + " | Type a Date:");
  display.drawBitmap(0, 0, calendar_allArray[1], 320, 218, GxEPD_BLACK);

  // Step 2: Day of the week for the 1st of the month (0 = Sun, 6 = Sat)
  DateTime firstDay(year, month, 1);
  int startDay = firstDay.dayOfTheWeek();  // 0–6, Sun to Sat

  // Step 3: Number of days in the month
  int nextYear = (month == 12) ? (year + 1) : year;
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
    int dayIndex = i + startDay;  // total box index in the 7x6 grid
    int row = dayIndex / 7;
    int col = dayIndex % 7;

    int x = GRID_X + col * CELL_W;
    int y = GRID_Y + row * CELL_H;

    int dayNum = i + 1;  // 1-based day number

    // Current day
    if (dayNum == now.day() && monthOffset == 0) {
      display.setFont(&FreeSerifBold9pt7b);
    } else
      display.setFont(&FreeSerif9pt7b);

    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x + 6, y + 15);
    display.print(dayNum);

    // Draw icon if there are events on day

    String YYYYMMDD = intToYYYYMMDD(year, month, dayNum);
    // Pad month and dayNum with leading zeros
    /*String paddedMonth = (month < 10 ? "0" : "") + String(month);
    String paddedDay   = (dayNum < 10 ? "0" : "") + String(dayNum);

    // Format date as YYYYMMDD
    String YYYYMMDD = String(year) + paddedMonth + paddedDay;*/

    int numEvents = checkEvents(YYYYMMDD, true);

    // Events found
    if (numEvents > 2) {
      display.setFont(&Font5x7Fixed);
      display.setCursor(x + 32, y + 16);
      display.print(String(numEvents));
    } else if (numEvents > 1) {
      // More than 1 event
      display.drawBitmap(x + 29, y + 8, _eventMarker1, 10, 10, GxEPD_BLACK);
    } else if (numEvents > 0) {
      // One event exists
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
    if (eventCount > 6)
      eventCount = 6;

    // Blank out extra space
    display.fillRect(9 + (i * 44), 71 + (eventCount * 23), 39, ((6 - eventCount) * 23),
                     GxEPD_WHITE);

    for (int j = 0; j < eventCount; j++) {
      CalendarEvent& ev = dayEvents[j];

      // Format start time as HHMM
      char timeBuf[6];
      snprintf(timeBuf, sizeof(timeBuf), "%02d%02d", ev.hour, ev.minute);
      String startTime = String(timeBuf);

      // Indicator for repeat events
      if (ev.repeat != "NO")
        startTime = ":: " + startTime;

      // Short event name (first 6 chars)
      String eventName = ev.eventName;
      if (eventName.length() > 6)
        eventName = eventName.substring(0, 6);

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

// ics import/export
void expandRecurringEvents(int year, int month, int day) {
  dayEvents.clear();
  String todayStr = intToYYYYMMDD(year, month, day);

  for (size_t i = 0; i < calendarEvents.size(); i++) {
    CalendarEvent e = calendarEvents[i];
    e.calendarIndex = i;  // <- store original index

    // Non-repeating event
    if (e.year == year && e.month == month && e.day == day) {
      dayEvents.push_back(e);
      continue;
    }

    // Weekly recurrence
    if (e.repeat.startsWith("WEEKLY")) {
      String byDay = e.repeat.substring(7);
      int targetDOW = dayStringToInt(byDay);
      int todayDOW = getDayOfWeek(year, month, day);

      if (todayDOW == targetDOW) {
        if (e.year < year ||
            (e.year == year && (e.month < month || (e.month == month && e.day <= day)))) {
          CalendarEvent recEvent = e;
          recEvent.year = year;
          recEvent.month = month;
          recEvent.day = day;
          recEvent.calendarIndex = i;  // preserve original
          dayEvents.push_back(recEvent);
        }
      }
    }

    // TODO: handle DAILY, MONTHLY, YEARLY
    // DAILY recurrence
    if (e.repeat == "DAILY") {
      if (e.year < year ||
          (e.year == year && (e.month < month || (e.month == month && e.day <= day)))) {
        CalendarEvent recEvent = e;
        recEvent.year = year;
        recEvent.month = month;
        recEvent.day = day;
        recEvent.calendarIndex = i;
        dayEvents.push_back(recEvent);
      }
    }

    // MONTHLY recurrence
    if (e.repeat == "MONTHLY") {
      if (day == e.day) {
        if (e.year < year || (e.year == year && (e.month <= month))) {
          CalendarEvent recEvent = e;
          recEvent.year = year;
          recEvent.month = month;
          recEvent.day = day;
          recEvent.calendarIndex = i;
          dayEvents.push_back(recEvent);
        }
      }
    }

    // YEARLY recurrence
    if (e.repeat == "YEARLY") {
      if (month == e.month && day == e.day) {
        if (year >= e.year) {
          CalendarEvent recEvent = e;
          recEvent.year = year;
          recEvent.month = month;
          recEvent.day = day;
          recEvent.calendarIndex = i;
          dayEvents.push_back(recEvent);
        }
      }
    }
  }
}

bool parseICSDateTime(const String& line,
                      String& outDate,  // YYYYMMDD
                      String& outTime   // HH:MM
) {
  // Expected formats:
  // DTSTART:YYYYMMDD
  // DTSTART:YYYYMMDDTHHMMSS
  // DTSTART:YYYYMMDDTHHMM

  int colon = line.indexOf(':');
  if (colon < 0)
    return false;

  String value = line.substring(colon + 1);
  value.trim();

  if (value.length() < 8)
    return false;

  // Date
  outDate = value.substring(0, 8);

  // Time (optional)
  if (value.length() >= 13 && value.charAt(8) == 'T') {
    String hh = value.substring(9, 11);
    String mm = value.substring(11, 13);
    outTime = hh + ":" + mm;
  } else {
    outTime = "00:00";
  }

  return true;
}
bool computeICSDuration(const String& startLine,  // DTSTART:...
                        const String& endLine,    // DTEND:...
                        String& outDuration       // H:MM
) {
  String startDate, startTime;
  String endDate, endTime;

  if (!parseICSDateTime(startLine, startDate, startTime))
    return false;
  if (!parseICSDateTime(endLine, endDate, endTime))
    return false;

  // Only support same-day events for now
  if (startDate != endDate)
    return false;

  int sh = startTime.substring(0, 2).toInt();
  int sm = startTime.substring(3, 5).toInt();
  int eh = endTime.substring(0, 2).toInt();
  int em = endTime.substring(3, 5).toInt();

  int startMinutes = sh * 60 + sm;
  int endMinutes = eh * 60 + em;

  if (endMinutes <= startMinutes)
    return false;

  int diff = endMinutes - startMinutes;
  int hours = diff / 60;
  int minutes = diff % 60;

  outDuration = String(hours) + ":" + (minutes < 10 ? "0" : "") + String(minutes);
  return true;
}
bool parseICSRRule(const String& rruleLine, String& outRepeat) {
  outRepeat = "NO";

  if (!rruleLine.startsWith("RRULE:"))
    return false;

  String rule = rruleLine.substring(6);
  rule.toUpperCase();

  // DAILY
  if (rule.indexOf("FREQ=DAILY") >= 0) {
    outRepeat = "DAILY";
    return true;
  }

  // WEEKLY
  if (rule.indexOf("FREQ=WEEKLY") >= 0) {
    int bydayIdx = rule.indexOf("BYDAY=");
    if (bydayIdx >= 0) {
      String days = rule.substring(bydayIdx + 6);
      int semi = days.indexOf(';');
      if (semi >= 0)
        days = days.substring(0, semi);

      // days.replace(",", "");
      outRepeat = "WEEKLY " + days;
      return true;
    }
  }

  // MONTHLY
  if (rule.indexOf("FREQ=MONTHLY") >= 0) {
    int bymonthdayIdx = rule.indexOf("BYMONTHDAY=");
    if (bymonthdayIdx >= 0) {
      String day = rule.substring(bymonthdayIdx + 11);
      int semi = day.indexOf(';');
      if (semi >= 0)
        day = day.substring(0, semi);

      outRepeat = "MONTHLY " + day;
      return true;
    }

    int bydayIdx = rule.indexOf("BYDAY=");
    if (bydayIdx >= 0) {
      String code = rule.substring(bydayIdx + 6);
      int semi = code.indexOf(';');
      if (semi >= 0)
        code = code.substring(0, semi);

      outRepeat = "MONTHLY " + code;
      return true;
    }
  }

  // YEARLY
  if (rule.indexOf("FREQ=YEARLY") >= 0) {
    int bymonthIdx = rule.indexOf("BYMONTH=");
    int bymonthdayIdx = rule.indexOf("BYMONTHDAY=");

    if (bymonthIdx >= 0 && bymonthdayIdx >= 0) {
      int month = rule.substring(bymonthIdx + 8).toInt();
      int day = rule.substring(bymonthdayIdx + 11).toInt();

      const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

      if (month >= 1 && month <= 12) {
        outRepeat = "YEARLY " + String(months[month - 1]) + (day < 10 ? "0" : "") + String(day);
        return true;
      }
    }
  }

  return false;
}

void parseICSEvent(const std::vector<String>& icsLines) {
  std::vector<String> eventLines;
  bool inEvent = false;

  for (const auto& line : icsLines) {
    if (line == "BEGIN:VEVENT") {
      inEvent = true;
      eventLines.clear();
    }

    if (inEvent) {
      eventLines.push_back(line);
    }

    if (line == "END:VEVENT" && inEvent) {
      String eventName, repeat, note;
      int year = 0, month = 0, day = 0;
      int hour = 0, minute = 0;
      int durMins = 0;

      for (const auto& eline : eventLines) {
        if (eline.startsWith("SUMMARY:")) {
          eventName = eline.substring(8);
        } else if (eline.startsWith("DTSTART")) {
          int colonPos = eline.indexOf(':');
          if (colonPos != -1) {
            String dt = eline.substring(colonPos + 1);
            if (dt.length() >= 13) {
              year = dt.substring(0, 4).toInt();
              month = dt.substring(4, 6).toInt();
              day = dt.substring(6, 8).toInt();
              hour = dt.substring(9, 11).toInt();
              minute = dt.substring(11, 13).toInt();
            }
          }
        } else if (eline.startsWith("DTEND")) {
          int colonPos = eline.indexOf(':');
          if (colonPos != -1) {
            String dtEnd = eline.substring(colonPos + 1);

            // Build proper start datetime string from ints
            char startBuf[16];
            snprintf(startBuf, sizeof(startBuf), "%04d%02d%02dT%02d%02d00", year, month, day, hour,
                     minute);

            durMins = calculateDurationMinutes(String(startBuf), dtEnd);
          }
        } else if (eline.startsWith("RRULE:")) {
          parseICSRRule(eline, repeat);
        } else if (eline.startsWith("DESCRIPTION:")) {
          note = eline.substring(12);
        }
      }

      CalendarEvent ev;
      ev.eventName = eventName;
      ev.year = year;
      ev.month = month;
      ev.day = day;
      ev.hour = hour;
      ev.minute = minute;
      ev.durationMinutes = durMins;
      ev.repeat = repeat;
      ev.note = note;
      ev.calendarIndex = calendarEvents.size();  // <-- assign index

      calendarEvents.push_back(ev);

      inEvent = false;
    }
  }
}

void parseICSFile(const String& filename) {
  SDActive = true;
  File file = SD_MMC.open(filename, FILE_READ);
  if (!file) {
    ESP_LOGE(TAG, "Failed to open ICS file: %s", filename.c_str());
    return;
  }

  std::vector<String> veventLines;
  bool inEvent = false;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;

    if (line == "BEGIN:VEVENT") {
      inEvent = true;
      veventLines.clear();
    } else if (line == "END:VEVENT") {
      inEvent = false;
      parseICSEvent(veventLines);  // <-- parse one VEVENT at a time
    } else if (inEvent) {
      veventLines.push_back(line);
    }
  }

  file.close();
  SDActive = false;
}

String calculateDTEnd(const String& startDate, const String& startTime, const String& duration) {
  // Parse start date
  int year = startDate.substring(0, 4).toInt();
  int month = startDate.substring(4, 6).toInt();
  int day = startDate.substring(6, 8).toInt();

  // Parse start time
  int hour = startTime.substring(0, 2).toInt();
  int minute = startTime.substring(3, 5).toInt();

  // Parse duration
  int durHours = 0;
  int durMinutes = 0;
  if (duration.endsWith("m")) {
    durMinutes = duration.substring(0, duration.length() - 1).toInt();
  } else if (duration.indexOf(':') > 0) {
    int colon = duration.indexOf(':');
    durHours = duration.substring(0, colon).toInt();
    durMinutes = duration.substring(colon + 1).toInt();
  }

  // Add duration to start time
  minute += durMinutes;
  hour += durHours + minute / 60;
  minute = minute % 60;
  day += hour / 24;
  hour = hour % 24;

  // Adjust month/year for overflow
  int daysInThisMonth = daysInMonth(year, month);
  while (day > daysInThisMonth) {
    day -= daysInThisMonth;
    month++;
    if (month > 12) {
      month = 1;
      year++;
    }
    daysInThisMonth = daysInMonth(year, month);
  }

  // Format DTEND YYYYMMDDTHHMMSS
  char buf[17];
  snprintf(buf, sizeof(buf), "%04d%02d%02dT%02d%02d%02d", year, month, day, hour, minute, 0);
  return String(buf);
}

// end ics import/export

// Event Data Management
//
#pragma message "TODO: Migrate to a better/global file management system"
void updateEventArray() {
  ESP_LOGE(TAG, "updateEventsArray");
  ESP_LOGI(TAG, "IUPDATE events array");
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  calendarEvents.clear();

  SDActive = true;
  File eventsDir = SD_MMC.open("/sys/events");
  if (!eventsDir) {
    ESP_LOGE(TAG, "Failed to open /sys/events directory");
    SDActive = false;
    return;
  }

  File file = eventsDir.openNextFile();
  while (file) {
    if (!file.isDirectory() && file.name()[0] != '\0') {
      String filename = file.name();
      if (!filename.startsWith("/")) {
        filename = "/sys/events/" + filename;
      }

      if (filename.endsWith(".ics")) {
        ESP_LOGI(TAG, "Parsing ICS file: %s", filename.c_str());

        File f = SD_MMC.open(filename.c_str(), FILE_READ);
        if (!f) {
          ESP_LOGE(TAG, "Failed to open ICS file: %s", filename.c_str());
        } else {
          std::vector<String> icsLines;
          while (f.available()) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
              icsLines.push_back(line);
            }
          }
          f.close();

          parseICSEvent(icsLines);
        }

        delay(5);
      }
    }
    file = eventsDir.openNextFile();
  }

  eventsDir.close();
  SDActive = false;

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  SDActive = false;

  ESP_LOGI(TAG, "updateEventArray(): loaded %d events from ICS", (int)calendarEvents.size());
}

String calculateEndTimeFromInts(int year, int month, int day, int hour, int minute,
                                int durationMinutes) {
  // Fill a struct tm
  struct tm t = {0};
  t.tm_year = year - 1900;  // tm_year is years since 1900
  t.tm_mon = month - 1;     // tm_mon is 0-11
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = 0;

  // Convert to time_t (seconds since epoch)
  time_t startEpoch = mktime(&t);

  // Add duration (in seconds)
  time_t endEpoch = startEpoch + durationMinutes * 60;

  // Convert back to struct tm (UTC)
  struct tm endTime;
  gmtime_r(&endEpoch, &endTime);

  // Format YYYYMMDDTHHMMSS
  char buf[16];
  snprintf(buf, sizeof(buf), "%04d%02d%02dT%02d%02d%02d", endTime.tm_year + 1900,
           endTime.tm_mon + 1, endTime.tm_mday, endTime.tm_hour, endTime.tm_min, endTime.tm_sec);

  return String(buf);
}

void sortEventsByDate(std::vector<CalendarEvent>& calendarEvents) {
  std::sort(calendarEvents.begin(), calendarEvents.end(),
            [](const CalendarEvent& a, const CalendarEvent& b) {
              // Compare by year, month, day, hour, minute
              if (a.year != b.year)
                return a.year < b.year;
              if (a.month != b.month)
                return a.month < b.month;
              if (a.day != b.day)
                return a.day < b.day;
              if (a.hour != b.hour)
                return a.hour < b.hour;
              return a.minute < b.minute;
            });
}

void updateEventsFile() {
  SDActive = true;
  pocketmage::setCpuSpeed(240);
  delay(50);

  const char* filename = "/sys/events/calendar.ics";

  File f = SD_MMC.open(filename, FILE_WRITE);
  if (!f) {
    ESP_LOGE(TAG, "Failed to open %s for writing", filename);
    SDActive = false;
    return;
  }

  // Header
  f.println("BEGIN:VCALENDAR");
  f.println("PRODID:-//PocketMage//CalendarApp//EN");
  f.println("VERSION:2.0");

  for (size_t i = 0; i < calendarEvents.size(); i++) {
    const CalendarEvent& e = calendarEvents[i];

    // ---- Format DTSTART ----
    char dtStartBuf[20];
    snprintf(dtStartBuf, sizeof(dtStartBuf), "%04d%02d%02dT%02d%02d00", e.year, e.month, e.day,
             e.hour, e.minute);

    String dtStart = String(dtStartBuf);

    // ---- Calculate DTEND ----
    String dtEnd =
        calculateEndTimeFromInts(e.year, e.month, e.day, e.hour, e.minute, e.durationMinutes);

    // ---- UID & DTSTAMP ----
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);

    char stampBuf[32];
    strftime(stampBuf, sizeof(stampBuf), "%Y%m%dT%H%M%SZ", &t);

    String dtStamp = String(stampBuf);
    String uid = "PM_" + String(i) + "_" + dtStamp + "@pocketmage";

    // ---- Write VEVENT ----
    f.println("BEGIN:VEVENT");
    f.println("UID:" + uid);
    f.println("DTSTAMP:" + dtStamp);
    f.println("SUMMARY:" + e.eventName);
    f.println("DTSTART:" + dtStart);
    f.println("DTEND:" + dtEnd);

    if (e.repeat != "NO") {
      String rrule = repeatCodeToICS(e.repeat);
      if (rrule.length() > 0) {
        f.println("RRULE:" + rrule);
      }
    }

    f.println("DESCRIPTION:" + e.note);
    f.println("END:VEVENT");
  }

  f.println("END:VCALENDAR");
  f.close();

  if (SAVE_POWER)
    pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

  SDActive = false;
}

void parseHHMM(const String& hhmm, int& hour, int& minute) {
  hour = 0;
  minute = 0;

  int colonPos = hhmm.indexOf(':');
  if (colonPos != -1) {
    hour = hhmm.substring(0, colonPos).toInt();
    minute = hhmm.substring(colonPos + 1).toInt();
  } else if (hhmm.length() >= 3) {
    // Fallback: assume "HHMM" format without colon
    int timeInt = hhmm.toInt();
    hour = timeInt / 100;
    minute = timeInt % 100;
  }
}
void addEvent(String eventName, String startDate, String startTime, String duration, String repeat,
              String note) {
  // Convert startDate "YYYYMMDD" to integers
  int year = startDate.substring(0, 4).toInt();
  int month = startDate.substring(4, 6).toInt();
  int day = startDate.substring(6, 8).toInt();

  // Parse startTime "HH:MM" or "HHMM" correctly
  int hour, minute;
  parseHHMM(startTime, hour, minute);

  // Convert duration string to int
  int durMinutes = duration.toInt();

  // Build CalendarEvent
  CalendarEvent newEvent;
  newEvent.eventName = eventName;
  newEvent.year = year;
  newEvent.month = month;
  newEvent.day = day;
  newEvent.hour = hour;
  newEvent.minute = minute;
  newEvent.durationMinutes = durMinutes;
  newEvent.repeat = repeat;
  newEvent.note = note;

  // Add to calendarEvents
  updateEventArray();  // if needed
  calendarEvents.push_back(newEvent);

  // Sort and save
  sortEventsByDate(calendarEvents);
  updateEventsFile();
}

void deleteEvent(int index) {
  if (index >= 0 && index < calendarEvents.size()) {
    calendarEvents.erase(calendarEvents.begin() + index);
  }
}

void deleteEventByIndex(int indexToDelete) {
  if (indexToDelete < 0 || indexToDelete >= dayEvents.size())
    return;

  // Grab the event to delete
  CalendarEvent targetEvent = dayEvents[indexToDelete];

  // Remove from dayEvents
  dayEvents.erase(dayEvents.begin() + indexToDelete);

  // Remove matching event from calendarEvents
  for (auto it = calendarEvents.begin(); it != calendarEvents.end(); ++it) {
    if (*it == targetEvent) {  // uses operator== in CalendarEvent
      calendarEvents.erase(it);
      break;  // stop after first match
    }
  }
}

void updateEventByIndex(int indexToUpdate) {
  if (indexToUpdate < 0 || indexToUpdate >= dayEvents.size())
    return;

  CalendarEvent& evt = dayEvents[indexToUpdate];

  // Convert new event strings to integers
  int newYear = newEventStartDate.substring(0, 4).toInt();
  int newMonth = newEventStartDate.substring(4, 6).toInt();
  int newDay = newEventStartDate.substring(6, 8).toInt();

  int hour, minute;
  parseHHMM(newEventStartTime, hour, minute);

  int durMinutes = newEventDuration.toInt();

  // Update the dayEvents copy
  evt.eventName = newEventName;
  evt.year = newYear;
  evt.month = newMonth;
  evt.day = newDay;
  evt.hour = hour;
  evt.minute = minute;
  evt.durationMinutes = durMinutes;
  evt.repeat = newEventRepeat;
  evt.note = newEventNote;

  // Update the original calendarEvents using the stored index
  int idx = evt.calendarIndex;
  if (idx >= 0 && idx < calendarEvents.size()) {
    calendarEvents[idx] = evt;
  }
}

// Loops
void processKB_CALENDAR() {
  int currentMillis = millis();
  DateTime now = CLOCK().nowDT();

  switch (CurrentCalendarState) {
    case MONTH:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // HOME Recieved
        else if (inchar == 12) {
          HOME_INIT();
        }
        // CR Recieved
        else if (inchar == 13) {
          commandSelectMonth(currentLine);
          currentLine = "";
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
        // Space Recieved
        else if (inchar == 32) {
          currentLine += " ";
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // LEFT Recieved
        else if (inchar == 19) {
          monthOffsetCount--;
          newState = true;
        }
        // RIGHT Recieved
        else if (inchar == 21) {
          monthOffsetCount++;
          newState = true;
        }
        // CENTER Recieved
        else if (inchar == 20 || inchar == 7) {
          CurrentCalendarState = WEEK;
          KB().setKeyboardState(NORMAL);
          newState = true;
          delay(200);
          break;
        } else {
          currentLine += inchar;
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
          OLED().oledLine(currentLine, currentLine.length(), false);
        }
      }
      break;
    case WEEK:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // HOME Recieved
        else if (inchar == 12) {
          HOME_INIT();
        }
        // CR Recieved
        else if (inchar == 13) {
          // commandSelectMonth(currentLine);
          commandSelectWeek(currentLine);
          currentLine = "";
        }
        // SHIFT Recieved
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(SHIFT);
        }
        // FN Recieved
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(FUNC);
        }
        // Space Recieved
        else if (inchar == 32) {
          currentLine += " ";
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        // LEFT Recieved
        else if (inchar == 19) {
          weekOffsetCount--;
          newState = true;
        }
        // RIGHT Recieved
        else if (inchar == 21) {
          weekOffsetCount++;
          newState = true;
        }
        // CENTER Recieved
        else if (inchar == 20 || inchar == 7) {
          CurrentCalendarState = MONTH;
          KB().setKeyboardState(NORMAL);
          newState = true;
          delay(200);
          break;
        } else {
          currentLine += inchar;
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
          OLED().oledLine(currentLine, currentLine.length(), false);
        }
      }
      break;
    case NEW_EVENT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // HOME Recieved
        else if (inchar == 12) {
          newEventState--;
          currentLine = "";
          if (newEventState < 0) {
            CurrentCalendarState = MONTH;
            currentLine = "";
            newState = true;
            KB().setKeyboardState(NORMAL);
          }
        }
        // CR Recieved
        else if (inchar == 13) {
          switch (newEventState) {
            case 0:
              // Event Name: must be non-empty
              if (currentLine.length() > 0) {
                newEventName = currentLine;
                newEventState++;
                currentLine = newEventStartDate;
              } else {
                OLED().oledWord("Error: Empty event name");
                delay(2000);
                currentLine = "";
              }
              break;

            case 1:
              // Start Date: must be YYYYMMDD (8-digit number)
              if (currentLine.length() == 8 && currentLine.toInt() > 10000000) {
                newEventStartDate = currentLine;
                newEventState++;
                currentLine = "";
              } else {
                OLED().oledWord("Error: Invalid date (YYYYMMDD)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 2:
              // Start Time: must be HH:MM
              if (currentLine.length() == 5 && currentLine.charAt(2) == ':' &&
                  isDigit(currentLine.charAt(0)) && isDigit(currentLine.charAt(1)) &&
                  isDigit(currentLine.charAt(3)) && isDigit(currentLine.charAt(4))) {
                newEventStartTime = currentLine;
                newEventState++;
                currentLine = "";
              } else {
                OLED().oledWord("Error: Invalid time (HH:MM)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 3:
              // Duration: must be H:MM or HH:MM
              {
                int colonIdx = currentLine.indexOf(':');
                if ((colonIdx == 1 || colonIdx == 2) && isDigit(currentLine.charAt(0)) &&
                    isDigit(currentLine.charAt(colonIdx + 1)) &&
                    isDigit(currentLine.charAt(colonIdx + 2))) {
                  newEventDuration = currentLine;
                  newEventState++;
                  currentLine = "";
                } else {
                  OLED().oledWord("Error: Invalid duration (H:MM)");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 4:
              // Repeat: must be NO, DAILY, WEEKLY xx, MONTHLY xx, or YEARLY xx
              {
                String code = currentLine;
                code.toUpperCase();
                if (code == "HELP") {
                  // Display help screen here
                  OLED().oledWord("Help screen coming soon!");
                  delay(5000);
                  currentLine = "";
                } else if (code == "NO" || code == "DAILY" || code.startsWith("WEEKLY ") ||
                           code.startsWith("MONTHLY ") || code.startsWith("YEARLY ")) {
                  newEventRepeat = code;
                  newEventState++;
                  currentLine = "";
                } else {
                  OLED().oledWord("Error: Invalid repeat value");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 5:
              // Note: no restrictions
              newEventNote = currentLine;
              newEventState++;
              currentLine = "";
              break;
          }

          if (newEventState > 5) {
            // Create Event
            addEvent(newEventName, newEventStartDate, newEventStartTime, newEventDuration,
                     newEventRepeat, newEventNote);
            // Return to app
            OLED().oledWord("New Event \"" + newEventName + "\" Created");
            delay(2000);
            CurrentCalendarState = MONTH;
            KB().setKeyboardState(NORMAL);
          }
          newState = true;
        }
        // SHIFT Recieved
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(SHIFT);
        }
        // FN Recieved
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(FUNC);
        }
        // Space Recieved
        else if (inchar == 32) {
          currentLine += " ";
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        } else {
          currentLine += inchar;
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
          switch (newEventState) {
            case 0:
              OLED().oledLine(currentLine, currentLine.length(), false, "Enter the Event Name");
              break;
            case 1:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Start Date (YYYYMMDD)");
              break;
            case 2:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Start Time (HH:MM)");
              break;
            case 3:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Event Duration (HH:MM)");
              break;
            case 4:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Repeat Code or \"Help\"");
              break;
            case 5:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Attach a Note to the Event");
              break;
          }
        }
      }
      break;
    case VIEW_EVENT:
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // HOME Recieved
        else if (inchar == 12) {
          CurrentCalendarState = MONTH;
          currentLine = "";
          newState = true;
          KB().setKeyboardState(NORMAL);
        }
        // CR Recieved
        else if (inchar == 13) {
          switch (newEventState) {
            case -1:
              if (currentLine == "1") {
                newEventState = 0;
              } else if (currentLine == "2") {
                newEventState = 1;
              } else if (currentLine == "3") {
                newEventState = 2;
              } else if (currentLine == "4") {
                newEventState = 3;
              } else if (currentLine == "5") {
                newEventState = 4;
              } else if (currentLine == "6") {
                newEventState = 5;
              } else if (currentLine == "d" || currentLine == "D") {
                deleteEventByIndex(editingEventIndex);
                updateEventsFile();
                OLED().oledWord("Event : \"" + newEventName + "\" Deleted");
                delay(2000);
                CurrentCalendarState = MONTH;
                currentLine = "";
                newState = true;
                KB().setKeyboardState(NORMAL);
              } else if (currentLine == "s" || currentLine == "S") {
                updateEventByIndex(editingEventIndex);
                updateEventsFile();
                OLED().oledWord("Event : \"" + newEventName + "\" Edited");
                delay(2000);
                CurrentCalendarState = MONTH;
                currentLine = "";
                newState = true;
                KB().setKeyboardState(NORMAL);
              }
              currentLine = "";
              break;
            case 0:
              // Event Name: must be non-empty
              if (currentLine.length() > 0) {
                newEventName = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                OLED().oledWord("Error: Empty event name");
                delay(2000);
                currentLine = "";
              }
              break;

            case 1:
              // Start Date: must be YYYYMMDD (8-digit number)
              if (currentLine.length() == 8 && currentLine.toInt() > 10000000) {
                newEventStartDate = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                OLED().oledWord("Error: Invalid date (YYYYMMDD)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 2:
              // Start Time: must be HH:MM
              if (currentLine.length() == 5 && currentLine.charAt(2) == ':' &&
                  isDigit(currentLine.charAt(0)) && isDigit(currentLine.charAt(1)) &&
                  isDigit(currentLine.charAt(3)) && isDigit(currentLine.charAt(4))) {
                newEventStartTime = currentLine;
                currentLine = "";
                newEventState = -1;
              } else {
                OLED().oledWord("Error: Invalid time (HH:MM)");
                delay(2000);
                currentLine = "";
              }
              break;

            case 3:
              // Duration: must be H:MM or HH:MM
              {
                int colonIdx = currentLine.indexOf(':');
                if ((colonIdx == 1 || colonIdx == 2) && isDigit(currentLine.charAt(0)) &&
                    isDigit(currentLine.charAt(colonIdx + 1)) &&
                    isDigit(currentLine.charAt(colonIdx + 2))) {
                  newEventDuration = currentLine;
                  currentLine = "";
                  newEventState = -1;
                } else {
                  OLED().oledWord("Error: Invalid duration (H:MM)");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 4:
              // Repeat: must be NO, DAILY, WEEKLY xx, MONTHLY xx, or YEARLY xx
              {
                String code = currentLine;
                code.toUpperCase();
                if (code == "HELP") {
                  // Display help screen here
                  OLED().oledWord("Help screen coming soon!");
                  delay(5000);
                  currentLine = "";
                } else if (code == "NO" || code == "DAILY" || code.startsWith("WEEKLY ") ||
                           code.startsWith("MONTHLY ") || code.startsWith("YEARLY ")) {
                  newEventRepeat = code;
                  currentLine = "";
                  newEventState = -1;
                } else {
                  OLED().oledWord("Error: Invalid repeat value");
                  delay(2000);
                  currentLine = "";
                }
              }
              break;

            case 5:
              // Note: no restrictions
              newEventNote = currentLine;
              currentLine = "";
              newEventState = -1;
              break;
          }

          if (newEventState > 5) {
            // Create Event
            addEvent(newEventName, newEventStartDate, newEventStartTime, newEventDuration,
                     newEventRepeat, newEventNote);
            // Return to app
            OLED().oledWord("New Event \"" + newEventName + "\" Created");
            delay(2000);
            CurrentCalendarState = MONTH;
            KB().setKeyboardState(NORMAL);
          }
          newState = true;
        }
        // SHIFT Recieved
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(SHIFT);
        }
        // FN Recieved
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(FUNC);
        }
        // Space Recieved
        else if (inchar == 32) {
          currentLine += " ";
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        } else {
          currentLine += inchar;
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
          switch (newEventState) {
            case -1:
              OLED().oledLine(currentLine, currentLine.length(), false);
              break;
            case 0:
              OLED().oledLine(currentLine, currentLine.length(), false, "Enter the Event Name");
              break;
            case 1:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Start Date (YYYYMMDD)");
              break;
            case 2:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Start Time (HH:MM)");
              break;
            case 3:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Event Duration (HH:MM)");
              break;
            case 4:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Enter the Repeat Code or \"Help\"");
              break;
            case 5:
              OLED().oledLine(currentLine, currentLine.length(), false,
                              "Attach a Note to the Event");
              break;
          }
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
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        char inchar = KB().updateKeypress();
        // HANDLE INPUTS
        // No char recieved
        if (inchar == 0)
          ;
        // HOME Recieved
        else if (inchar == 12) {
          CurrentCalendarState = MONTH;
          currentLine = "";
          newState = true;
          KB().setKeyboardState(NORMAL);
        }
        // CR Recieved
        else if (inchar == 13) {
          commandSelectDay(currentLine);
          currentLine = "";
        }
        // SHIFT Recieved
        else if (inchar == 17) {
          if (KB().getKeyboardState() == SHIFT)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(SHIFT);
        }
        // FN Recieved
        else if (inchar == 18) {
          if (KB().getKeyboardState() == FUNC)
            KB().setKeyboardState(NORMAL);
          else
            KB().setKeyboardState(FUNC);
        }
        // Space Recieved
        else if (inchar == 32) {
          currentLine += " ";
        }
        // BKSP Recieved
        else if (inchar == 8) {
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }

        // LEFT Received
        else if (inchar == 19) {
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
            case 0:
              CurrentCalendarState = SUN;
              break;
            case 1:
              CurrentCalendarState = MON;
              break;
            case 2:
              CurrentCalendarState = TUE;
              break;
            case 3:
              CurrentCalendarState = WED;
              break;
            case 4:
              CurrentCalendarState = THU;
              break;
            case 5:
              CurrentCalendarState = FRI;
              break;
            case 6:
              CurrentCalendarState = SAT;
              break;
          }

          newState = true;
        }

        // RIGHT Received
        else if (inchar == 21) {
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
            case 0:
              CurrentCalendarState = SUN;
              break;
            case 1:
              CurrentCalendarState = MON;
              break;
            case 2:
              CurrentCalendarState = TUE;
              break;
            case 3:
              CurrentCalendarState = WED;
              break;
            case 4:
              CurrentCalendarState = THU;
              break;
            case 5:
              CurrentCalendarState = FRI;
              break;
            case 6:
              CurrentCalendarState = SAT;
              break;
          }

          newState = true;
        }

        // CENTER Recieved
        else if (inchar == 20 || inchar == 7) {
          CurrentCalendarState = WEEK;
          KB().setKeyboardState(NORMAL);
          newState = true;
          delay(200);
          break;
        } else {
          currentLine += inchar;
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
          OLED().oledLine(currentLine, currentLine.length(), false);
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

        // DRAW APP
        drawCalendarWeek(weekOffsetCount);

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
        // EINK().multiPassRefresh(2);
      }
      break;
    case MONTH:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        // DRAW APP
        drawCalendarMonth(monthOffsetCount);

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
        // EINK().multiPassRefresh(2);
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
        display.print(newEventStartDate);

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
    case VIEW_EVENT:
      if (newState) {
        newState = false;
        EINK().resetDisplay();

        switch (newEventState) {
          case -1:
            EINK().drawStatusBar("Type 1-6,(D)elete,or (S)ave");
            break;
          default:
            EINK().drawStatusBar("Type the info!");
            break;
        }
        display.drawBitmap(0, 0, calendar_allArray[3], 320, 218, GxEPD_BLACK);

        display.setFont(&FreeSerif9pt7b);

        display.setCursor(106, 68);
        display.print(newEventName);

        display.setCursor(106, 90);
        display.print(newEventStartDate);

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
    // All days use the same basic code
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

        // Draw background
        // CurrentCalendarState enumerations somehow line up with calendar app bitmaps?
        // SUN = 4, SAT = 10
        EINK().drawStatusBar("Events 1-7 or (N)ew");
        display.drawBitmap(0, 0, calendar_allArray[CurrentCalendarState], 320, 218, GxEPD_BLACK);

        // Draw Date
        display.setFont(&FreeSerif9pt7b);
        display.setTextColor(GxEPD_BLACK);
        // Set cursor based on the day of the week
        display.setCursor(9 + (44 * (CurrentCalendarState - 4)), 59);
        display.print(String(currentMonth) + "/" + String(currentDate));

        // Load events
        String YYYYMMDD = intToYYYYMMDD(currentYear, currentMonth, currentDate);
        int eventCount = checkEvents(YYYYMMDD, false);
        if (eventCount > 7)
          eventCount = 7;

        // Blank out extra space
        display.fillRect(12, 66 + (eventCount * 19), 297, ((7 - eventCount) * 19), GxEPD_WHITE);

        // Display events data
        for (int j = 0; j < eventCount; j++) {
          CalendarEvent& e = dayEvents[j];

          String name = e.eventName;

          // Convert hour/minute to HHMM string
          char timeBuf[5];
          snprintf(timeBuf, sizeof(timeBuf), "%02d%02d", e.hour, e.minute);
          String startTime = String(timeBuf);

          String duration = String(e.durationMinutes);
          String repeatCode = e.repeat;
          String bottomInfo =
              "Starts: " + startTime + ", Dur: " + duration + ", Rep: " + repeatCode;

          // Print event name
          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 74 + (j * 19));
          display.print(name);

          // Print bottom info
          display.setFont(&Font5x7Fixed);
          display.setCursor(48, 82 + (j * 19));
          display.print(bottomInfo);
        }

        EINK().forceSlowFullUpdate(true);
        EINK().refresh();
        // EINK().multiPassRefresh(2);
      }
      break;
  }
}
#endif
