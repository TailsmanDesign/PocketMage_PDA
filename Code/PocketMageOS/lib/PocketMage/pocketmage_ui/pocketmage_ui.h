#pragma once
#include <Arduino.h>
#include <vector>
#include <GxEPD2_BW.h>
#include <pocketmage_font/pocketmage_font.h>

enum class EinkRefresh : uint8_t {
    Normal,
    ForceFull,
};

void drawScrollbar(int total, int visible, int index,
                   int barX = -1, int barY = 0, int barH = -1,
                   int barW = 3, bool clearBg = false,
                   uint16_t fg = GxEPD_BLACK, uint16_t bg = GxEPD_WHITE);

void beginEinkScreen(bool preserveBg = false);
void endEinkScreen(const char* statusText, EinkRefresh mode = EinkRefresh::Normal);

void drawListItem(int x, int y, const String& text, int maxWidth = -1);

class U8G2;

void drawCyclePickerOLED(U8G2& u8g2, const char* const* items, int count,
                         int selected, const char* badge = nullptr);
