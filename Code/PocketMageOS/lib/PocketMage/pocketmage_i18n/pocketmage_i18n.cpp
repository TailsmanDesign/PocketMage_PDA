#include "pocketmage_i18n.h"
#include <string.h>

Lang I18n::lang_ = Lang::English;

void I18n::setLanguage(Lang lang) {
  if (static_cast<int>(lang) < 0 || static_cast<int>(lang) >= _LANG_COUNT) {
    lang = Lang::English;
  }
  lang_ = lang;
}

bool I18n::setLanguageByCode(const char* code) {
  for (int i = 0; i < _LANG_COUNT; i++) {
    if (strcmp(kLanguageCodes[i], code) == 0) {
      lang_ = static_cast<Lang>(i);
      return true;
    }
  }
  return false;
}

Lang I18n::language() {
  return lang_;
}

int I18n::languageCount() {
  return _LANG_COUNT;
}

const char* I18n::code() {
  return code(static_cast<int>(lang_));
}

const char* I18n::code(int idx) {
  if (idx < 0 || idx >= _LANG_COUNT) return "";
  return kLanguageCodes[idx];
}

const char* I18n::nativeName() {
  return nativeName(static_cast<int>(lang_));
}

const char* I18n::nativeName(int idx) {
  if (idx < 0 || idx >= _LANG_COUNT) return "";
  return kLanguageNames[idx];
}

const char* I18n::get(StringID id) {
  if (static_cast<int>(id) < 0 || static_cast<int>(id) >= _STR_COUNT) return "";
  return kStrings[static_cast<int>(lang_)][static_cast<int>(id)];
}

const char* I18n::monthName(int month) {
  if (month < 1 || month > 12) return get(STR_MONTH_ERR);
  return get(static_cast<StringID>(STR_MONTH_JAN + (month - 1)));
}

const char* I18n::dayName(int idx) {
  if (idx < 0 || idx > 6) return get(STR_MONTH_ERR);
  return get(static_cast<StringID>(STR_DAY_SUNDAY + idx));
}

const char* I18n::appName(int idx) {
  if (idx < 0 || idx > 11) return "";
  return get(static_cast<StringID>(STR_GRID_TXT + idx));
}

const char* I18n::kbAppName(int idx) {
  if (idx < 0 || idx > 12) return "";
  return get(static_cast<StringID>(STR_KB_APP_CANCEL + idx));
}

static char foldAccent(uint8_t hi, uint8_t lo) {
  if (hi != 0xC3 || lo < 0x80 || lo > 0xBF) return 0;
  if (lo <= 0x9E) lo |= 0x20; 
  switch (lo) {
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: return 'a';
    case 0xA7: return 'c';
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: return 'e';
    case 0xAC: case 0xAD: case 0xAE: case 0xAF: return 'i';
    case 0xB1: return 'n';
    case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: return 'o';
    case 0xB9: case 0xBA: case 0xBB: case 0xBC: return 'u';
    case 0xBD: return 'y';
    case 0x9F: return 's'; 
    case 0xBF: return 'y'; 
    default: return 0;     
  }
}

String I18n::fold(const String& s) {
  String out;
  out.reserve(s.length());
  for (unsigned int i = 0; i < s.length(); i++) {
    uint8_t c = static_cast<uint8_t>(s[i]);
    if (c >= 'A' && c <= 'Z') {
      out += static_cast<char>(c + 32);
    } else if (c >= 0x80) {
      unsigned int seq = 1;
      if (c >= 0xC2 && c <= 0xDF) seq = 2;
      else if (c >= 0xE0 && c <= 0xEF) seq = 3;
      else if (c >= 0xF0 && c <= 0xF4) seq = 4;
      if (seq == 2 && c == 0xC3 && i + 1 < s.length()) {
        char folded = foldAccent(c, static_cast<uint8_t>(s[i + 1]));
        if (folded) out += folded;
      }
      i += seq - 1;
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
}

const char* I18n::aliasFor(const char* folded) {
  const char* const* pairs = kCommandAliases[static_cast<int>(lang_)];
  const uint8_t count = kCommandAliasCounts[static_cast<int>(lang_)];
  for (uint8_t i = 0; i < count; i++) {
    if (strcmp(pairs[2 * i], folded) == 0) return pairs[2 * i + 1];
  }
  return nullptr;
}

String I18n::normalizeCommand(const String& raw) {
  int space = raw.indexOf(' ');
  String head = (space == -1) ? raw : raw.substring(0, space);
  const char* word = aliasFor(fold(head).c_str());
  if (word) {
    if (space == -1) return String(word);
    return String(word) + raw.substring(space);
  }
  const char* whole = aliasFor(fold(raw).c_str());
  if (whole) return String(whole);
  return raw;
}