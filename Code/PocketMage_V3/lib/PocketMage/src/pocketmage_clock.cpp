//  888888ba  d888888P  a88888b. //
//  88    `8b    88    d8'   `88 //
// a88aaaa8P'    88    88        //
//  88   `8b.    88    88        //
//  88     88    88    Y8.   .88 //
//  dP     dP    dP     Y88888P' //

#include "pocketmage.h"

static constexpr const char* TAG = "CLOCK";

RTC_PCF8563 rtc;

const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Initialization of clock class
static PocketmageCLOCK pm_clock(rtc);

// Setup for Clock Class
void setupClock(){
  pinMode(RTC_INT, INPUT);
  if (!CLOCK().begin()) {
    ESP_LOGE(TAG, "Couldn't find RTC");
    delay(1000);
  }
  // SET CLOCK IF NEEDED
  if (SET_CLOCK_ON_UPLOAD || CLOCK().getRTC().lostPower()) {
    CLOCK().setToCompileTimeUTC();
  }
  CLOCK().getRTC().start();
  wireClock();
}

// Wire function  for Clock class
// add any global references here + add set function to class header file
void wireClock(){
    
}

// Access for other apps
PocketmageCLOCK& CLOCK() { return pm_clock; }

bool PocketmageCLOCK::begin() {
  if (!rtc_.begin()) { begun_ = false; return false; }
  begun_ = true;
  return true;
}

PocketmageCLOCK::TimeParseResult PocketmageCLOCK::parseTimeString(const String& timeStr,
                                                                  int& outHours, int& outMinutes) {
  String s = timeStr;
  s.trim();

  outHours = -1;
  outMinutes = -1;

  // --- Detect AM / PM suffix ---
  bool isAM = false;
  bool isPM = false;

  String lower = s;
  lower.toLowerCase();

  if (lower.endsWith("am")) {
    isAM = true;
    s = s.substring(0, s.length() - 2);
  } else if (lower.endsWith("pm")) {
    isPM = true;
    s = s.substring(0, s.length() - 2);
  } else if (lower.endsWith("a")) {
    isAM = true;
    s = s.substring(0, s.length() - 1);
  } else if (lower.endsWith("p")) {
    isPM = true;
    s = s.substring(0, s.length() - 1);
  }

  s.trim();

  int colonIdx = s.indexOf(':');

  if (colonIdx != -1) {
    // H:MM or HH:MM
    String hStr = s.substring(0, colonIdx);
    String mStr = s.substring(colonIdx + 1);

    if (mStr.length() != 2 || hStr.length() < 1 || hStr.length() > 2)
      return TIME_INVALID_FORMAT;

    outHours = hStr.toInt();
    outMinutes = mStr.toInt();
  } else {
    // HMM or HHMM
    if (s.length() < 3 || s.length() > 4)
      return TIME_INVALID_FORMAT;

    int value = s.toInt();
    outHours = value / 100;
    outMinutes = value % 100;
  }

  // --- Apply AM/PM conversion ---
  if (isAM || isPM) {
    if (outHours < 1 || outHours > 12)
      return TIME_OUT_OF_RANGE;

    if (isAM && outHours == 12)
      outHours = 0;
    else if (isPM && outHours != 12)
      outHours += 12;
  }

  if (outHours < 0 || outHours > 23 || outMinutes < 0 || outMinutes > 59)
    return TIME_OUT_OF_RANGE;

  return TIME_OK;
}

void PocketmageCLOCK::setTimeFromString(String timeStr) {
  int hours, minutes;

  TimeParseResult res = parseTimeString(timeStr, hours, minutes);

  if (res != TIME_OK) {
    ESP_LOGE(TAG, "Invalid time (%d): %s", res, timeStr.c_str());
    OLED().oledWord("Invalid");
    delay(500);
    return;
  }

  DateTime now = CLOCK().nowDT();
  CLOCK().getRTC().adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, 0));

  ESP_LOGI(TAG, "Time updated to %02d:%02d", hours, minutes);
}

bool PocketmageCLOCK::isValid() {
  if (!begun_) return false;
  DateTime t = rtc_.now();
  const bool saneYear = t.year() >= 2020 && t.year() < 2099;  // check for reasonable year for DateTime t
  return saneYear;
}

