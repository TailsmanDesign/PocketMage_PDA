#pragma once

#include <Arduino.h>
#include "pocketmage_i18n_gen.h"

class I18n {
public:
  static void setLanguage(Lang lang);
  static bool setLanguageByCode(const char* code);
  
  static Lang language();
  static int languageCount();
  static const char* code();
  static const char* code(int idx);
  static const char* nativeName();
  static const char* nativeName(int idx);
  static const char* get(StringID id);

  static const char* monthName(int month);
  static const char* dayName(int idx); 
  static const char* appName(int idx);       // home grid label, 0..11
  static const char* kbAppName(int idx);     // app-switcher badge, 0..12
  static String normalizeCommand(const String& raw);

private:
  static Lang lang_;
  static const char* aliasFor(const char* folded);
  static String fold(const String& s);
};

#define TR(id) I18n::get(id)