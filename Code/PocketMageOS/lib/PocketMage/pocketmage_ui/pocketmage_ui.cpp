#include "pocketmage_ui.h"
#include <pocketmage_eink/pocketmage_eink.h>
#include <pocketmage_layout/pocketmage_layout.h>
#include <U8g2lib.h>

void drawScrollbar(int total, int visible, int index,
                   int barX, int barY, int barH, int barW, bool clearBg,
                   uint16_t fg, uint16_t bg) {
  int maxScroll = total - visible;
  if (maxScroll <= 0) return;

  if (barX < 0) barX = display.width() - barW;
  if (barH < 0) barH = display.height();

  float visibleRatio = min((float)visible / total, 1.0f);
  int handleHeight = max((int)(barH * visibleRatio), 15);

  int clamped = index < 0 ? 0 : (index > maxScroll ? maxScroll : index);
  float scrollFraction = (float)clamped / maxScroll;
  int handleY = barY + (int)(scrollFraction * (barH - handleHeight));

  if (clearBg) {
    display.fillRect(barX - 1, barY, barW + 1, barH, bg);
  }

  display.fillRect(barX, handleY, barW, handleHeight, fg);

  display.drawFastHLine(barX, display.height() - 1, barW, bg);
  display.drawFastHLine(barX, 0, barW, bg);
}

void beginEinkScreen(bool preserveBg) {
  EINK().resetDisplay(!preserveBg);
}

void endEinkScreen(const char* statusText, EinkRefresh mode) {
  if (!statusText) statusText = "";
  EINK().drawStatusBar(statusText);
  switch (mode) {
    case EinkRefresh::Normal:
      EINK().refresh();
      break;
    case EinkRefresh::ForceFull:
      EINK().forceSlowFullUpdate(true);
      EINK().refresh();
      break;
  }
}

void drawListItem(int x, int y, const String& text, int maxWidth) {
  if (maxWidth > 0) {
    String s = text;
    int w = FontEngine::textWidth(DisplayTarget::EINK, s, FontStyle::Body);
    if (w > maxWidth) {
      s = truncateWithEllipsis(s, maxWidth, FontStyle::Body);
    }
    FontEngine::drawText(DisplayTarget::EINK, x, y, s, FontStyle::Body);
  } else {
    FontEngine::drawText(DisplayTarget::EINK, x, y, text, FontStyle::Body);
  }
}

int drawChipText(int x, int baselineY, const String& text,
                 FontStyle style, int maxTextW,
                 bool inverted, int chipMaxW, int padX, int chipH, int bottomPad) {
  String s = text;
  if (maxTextW > 0) {
    style = FontEngine::fitStyle(DisplayTarget::EINK, text.c_str(), maxTextW,
                                 kLabelCascade, kLabelCascadeCount);
    s = truncateWithEllipsis(text, maxTextW, style);
  }

  const int inkW = FontEngine::textWidth(DisplayTarget::EINK, s, style);
  int chipW = inkW + padX * 2;
  if (chipMaxW > 0 && chipW > chipMaxW) chipW = chipMaxW;

  if (inverted) {
    display.fillRoundRect(x, baselineY + bottomPad - chipH, chipW, chipH, 4, GxEPD_BLACK);
    u8g2f.setForegroundColor(GxEPD_WHITE);
  }
  FontEngine::drawText(DisplayTarget::EINK, x + padX, baselineY, s, style);
  if (inverted) {
    u8g2f.setForegroundColor(GxEPD_BLACK);
  }
  return chipW;
}

void drawCyclePickerOLED(U8G2& u8g2, const char* const* items, int count,
                         int selected, const char* badge) {
  if (count <= 0 || items == nullptr) return;
  selected = constrain(selected, 0, count - 1);

  constexpr int kEdgeMargin  = 4;   // breathing room left/right of the panel
  constexpr int kMaxCellW    = 22;
  constexpr int kPanelY      = 2;
  constexpr int kPanelH      = 29;
  constexpr int kLeftInset   = 1;   // extra left padding; panel shifts right
  constexpr int kGlyphBaseY  = 25;
  constexpr int kGlyphBaseYBadged = 16;
  constexpr int kRadius      = 4;
  constexpr int kBadgeH      = 11;
  // Centering uses advance widths (getUTF8Width)
  constexpr int kGlyphCenterNudge = 1;

  const int dw = u8g2.getDisplayWidth();
  const int dh = u8g2.getDisplayHeight();
  const bool hasBadge = (badge != nullptr && badge[0] != '\0');
  const int glyphBaseY = hasBadge ? kGlyphBaseYBadged : kGlyphBaseY;

  // Fit the row inside the display: narrow cells rather than clipping the
  // outer frame on long lists (12 cells * 22px used to overflow 256px).
  const int cellW  = min(kMaxCellW, (dw - 2 * kEdgeMargin) / count);
  const int panelW = cellW * count;
  const int panelX = (dw - panelW) / 2 + kLeftInset;

  // Opaque interior. Square fill, so rounded-corner residue from the
  // previous frame goes with it in one stroke.
  u8g2.setDrawColor(0);
  u8g2.drawBox(panelX, kPanelY, panelW, kPanelH);

  u8g2.setDrawColor(1);
  u8g2.drawRFrame(panelX, kPanelY, panelW, kPanelH, kRadius);

  for (int i = 0; i < count; i++) {
    const int cx = panelX + i * cellW;
    if (i == selected) {
      u8g2.setDrawColor(1);
      u8g2.drawRBox(cx, kPanelY, cellW, kPanelH, kRadius);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }
    const int gw = FontEngine::textWidth(DisplayTarget::OLED, items[i], FontStyle::BodyBold);
    FontEngine::drawText(DisplayTarget::OLED,
                         cx + (cellW - gw) / 2 + kGlyphCenterNudge,
                         glyphBaseY, items[i], FontStyle::BodyBold);
  }

  if (hasBadge) {
    const int bw = FontEngine::textWidth(DisplayTarget::OLED, badge, FontStyle::Tiny) + 6;
    const int bx = (dw - bw) / 2;
    const int by = dh - 13;  // sits over the panel's bottom edge
    u8g2.setDrawColor(1);
    u8g2.drawRBox(bx, by, bw, kBadgeH, kRadius);
    u8g2.setDrawColor(0);
    FontEngine::drawText(DisplayTarget::OLED, bx + 3, by + 10, badge, FontStyle::Tiny);
  }

  u8g2.setDrawColor(1);
}
