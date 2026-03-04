//  888888ba  d888888P  a88888b. //
//  88    `8b    88    d8'   `88 //
// a88aaaa8P'    88    88        //
//  88   `8b.    88    88        //
//  88     88    88    Y8.   .88 //
//  dP     dP    dP     Y88888P' //

#pragma once
#include <Arduino.h>
#include <RTClib.h>

// Real-time clock
extern const char daysOfTheWeek[7][12]; // Day names

class PocketmageCLOCK {
public:
  explicit PocketmageCLOCK(RTC_PCF8563 &rtc) : rtc_(rtc) {}

  bool begin();
  void setTimeFromString(String timeStr);
  bool isValid();

  void setToCompileTimeUTC() { rtc_.adjust(DateTime(F(__DATE__), F(__TIME__))); }

  DateTime nowDT()                                         { return rtc_.now(); }
  RTC_PCF8563& getRTC()                                          { return rtc_; }
  // To Do: create a task on core 1 that checks for timeout and sets a flag for OS
  long getTimeDiff()                { return timeoutMillis_ - prevTimeMillis_; }
  volatile long getTimeoutMillis() const                    { return timeoutMillis_; }

  volatile long  getPrevTimeMillis() const                    { return prevTimeMillis_; }
  void setTimeoutMillis(long t)                            { timeoutMillis_ = t;  }
  void setPrevTimeMillis(long t)                        { prevTimeMillis_ = t; } 

private:
  RTC_PCF8563 &rtc_;  
  bool begun_ = false;
  volatile long timeoutMillis_ = 0;   // Timeout tracking
  volatile long prevTimeMillis_ = 0;  // Previous time for timeout
  enum TimeParseResult { TIME_OK = 0, TIME_INVALID_FORMAT, TIME_OUT_OF_RANGE };
  static TimeParseResult parseTimeString(const String& timeStr, int& outHours, int& outMinutes);
};

void wireClock();
void setupClock();
PocketmageCLOCK& CLOCK();