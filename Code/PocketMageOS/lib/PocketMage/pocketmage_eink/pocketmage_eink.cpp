//  oooooooooooo         ooooo ooooo       ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#include <pocketmage.h>

static constexpr const char* tag = "EINK";

GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT>
  display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

U8G2_FOR_ADAFRUIT_GFX u8g2f;

TaskHandle_t einkHandlerTaskHandle = NULL;

// Recursive mutex serializing panel access between the main task (keystroke
// driven refresh()) and the einkHandler task (50ms poll).  Recursive so
// refresh() can take it and still call lockPanel() from deepSleep() etc.
static SemaphoreHandle_t einkMutex = NULL;

volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;

static PocketmageEink pm_eink(display);

PocketmageEink& EINK() { return pm_eink; }

void PocketmageEink::refresh() {
  lockPanel();
  if (FAST_REFRESH) {
    // Fast refresh stack (experimental, default off): differential partial
    // redraws with a boot full and a periodic slow clean; each full is followed
    // by a beta-only same-frame partial reinforce for the under-saturated black.
    if (panelNeedsFullRefresh_) {
      // First refresh after boot/deep-sleep wake: a true full update resyncs the
      // panel with GxEPD2's previous-image RAM.  On beta hardware, the fast
      // full waveform under-saturates black, so reinforce with a same-frame
      // partial (0x10 == 0x13); production has good contrast without it.
      panelNeedsFullRefresh_ = false;
      partialCounter_ = 0;
      setFastFullRefresh(true);
      display_.display(false);
      #if POCKETMAGE_HW_VERSION != 2
        display_.display(true);
      #endif
    } else if ((partialCounter_ >= FAST_REFRESH_AFTER) || forceSlowFullUpdate_) {
      forceSlowFullUpdate_ = false;
      partialCounter_ = 0;
      // Slow "clean" full update to clear accumulated ghosting from the fast
      // partial refreshes; uses the extended (default) waveform, ~3s.  Followed
      // by a beta-only same-frame partial refresh.
      setFastFullRefresh(false);
      display_.display(false);
      #if POCKETMAGE_HW_VERSION != 2
        display_.display(true);
      #endif
    } else {
      // Fast full-screen partial update (~0.65s).  GxEPD2's display() keeps the
      // panel's previous-image RAM (0x10) in sync, so the differential update
      // stays correct; ghosting is bounded by the periodic clean above.
      partialCounter_++;
      display_.display(true);
    }
    display_.setFullWindow();
    display_.fillScreen(GxEPD_WHITE);
    display_.powerOff();
  } else {
    // Legacy refresh (production default, "tried and true"): always a full
    // refresh, fast waveform for normal redraws and slow waveform for the
    // periodic clean / forced slow update.  Deep sleep between refreshes; the
    // full update rebuilds both RAM buffers, so the hibernate wake-reset that
    // wipes panel RAM is harmless here.
    if ((partialCounter_ >= fullRefreshAfter_) || forceSlowFullUpdate_) {
      forceSlowFullUpdate_ = false;
      partialCounter_ = 0;
      setFastFullRefresh(false);
    } else {
      setFastFullRefresh(true);
      partialCounter_++;
    }
    display_.display(false);
    display_.setFullWindow();
    display_.fillScreen(GxEPD_WHITE);
    display_.hibernate();
  }
  unlockPanel();
}

void PocketmageEink::lockPanel() {
  if (einkMutex) xSemaphoreTakeRecursive(einkMutex, portMAX_DELAY);
}

void PocketmageEink::unlockPanel() {
  if (einkMutex) xSemaphoreGiveRecursive(einkMutex);
}

void PocketmageEink::setFastFullRefresh(bool setting) {
  if (PanelT::useFastFullUpdate != setting) {
    PanelT::useFastFullUpdate = setting;
  }
}

void PocketmageEink::statusBar(const String& input, bool fullWindow) {
  if (!fullWindow) {
    display_.setPartialWindow(0, display_.height() - 20, display_.width(), 20);
    drawStatusBar(input);
  } else {
    drawStatusBar(input);
  }
  display_.drawRect(display_.width() - 30, display_.height() - 20, 30, 20, GxEPD_BLACK);
}

void PocketmageEink::drawStatusBar(const String& input) {
  FontEngine::setTextColor(DisplayTarget::EINK, GxEPD_BLACK);
  u8g2f.setBackgroundColor(GxEPD_WHITE);
  display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
  display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);

  // Leave room for the battery box on the right
  constexpr int kBarPadL = 4;
  constexpr int kBatteryW = 30;
  const int maxTextW = display_.width() - kBarPadL - kBatteryW - 2;

  FontStyle style = FontEngine::fitStyle(DisplayTarget::EINK, input.c_str(), maxTextW,
                                         kLabelCascade, kLabelCascadeCount);
  String text = truncateWithEllipsis(input, maxTextW, style);
  FontEngine::drawText(DisplayTarget::EINK, kBarPadL, display_.height() - 6, text, style);
}

void PocketmageEink::resetDisplay(bool clearScreen, uint16_t color) {
  display_.setRotation(3);
  display_.setFullWindow();
  if (clearScreen) display_.fillScreen(color);
}

int PocketmageEink::countLines(const String& input, size_t maxLineLength) {
  if (maxLineLength == 0) return 1;
  size_t inputLength = input.length();
  size_t charCounter = 0;
  uint16_t lineCounter = 1;
  for (size_t c = 0; c < inputLength; c++) {
    if (input[c] == '\n') {
        charCounter = 0;
        lineCounter++;
        continue;
    }
    if (charCounter >= maxLineLength) {
        charCounter = 0;
        lineCounter++;
    }
    charCounter++;
  }
  return lineCounter;
}

void PocketmageEink::forceSlowFullUpdate(bool force) { forceSlowFullUpdate_ = force; }

void setupEink() {
  einkMutex = xSemaphoreCreateRecursiveMutex();
  display.init(115200);
  pm_eink.markPanelNeedsFullRefresh();
  display.setRotation(3);
  display.setFullWindow();
  u8g2f.begin(display);
  u8g2f.setForegroundColor(GxEPD_BLACK);
  u8g2f.setBackgroundColor(GxEPD_WHITE);
  u8g2f.setFontMode(1);
  display.fillScreen(GxEPD_WHITE);

  xTaskCreatePinnedToCore(
    einkHandler,
    "einkHandlerTask",
    10000,
    NULL,
    1,
    &einkHandlerTaskHandle,
    0
  );
}

uint8_t PocketmageEink::getFontHeight() {
  return FontEngine::fontHeight(DisplayTarget::EINK, FontStyle::Body);
}

uint16_t PocketmageEink::getEinkTextWidth(const String& s) {
  return FontEngine::textWidth(DisplayTarget::EINK, s, FontStyle::Body);
}
