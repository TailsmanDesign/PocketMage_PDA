//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#include <pocketmage.h>

static constexpr const char* tag = "EINK";

GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

TaskHandle_t einkHandlerTaskHandle = NULL; // E-Ink handler task

// Fast full update flag for e-ink
volatile bool GxEPD2_310_GDEQ031T10::useFastFullUpdate = true;

// Initialization of eink display class
static PocketmageEink pm_eink(display);

// Access for other apps 
PocketmageEink& EINK() { return pm_eink; }

// ===================== main functions =====================
void PocketmageEink::refresh() {
  // USE A SLOW FULL UPDATE EVERY N FAST UPDATES OR WHEN SPECIFIED
  if ((partialCounter_ >= fullRefreshAfter_) || forceSlowFullUpdate_) {
    forceSlowFullUpdate_ = false;
    partialCounter_ = 0;
    setFastFullRefresh(false);
  } 
  // OTHERWISE USE A FAST FULL UPDATE
  else {
    setFastFullRefresh(true);
    partialCounter_++;
  }

  display_.display(false);

  display_.setFullWindow();
  display_.fillScreen(GxEPD_WHITE);
  display_.hibernate();
}
void PocketmageEink::multiPassRefresh(int passes) {
  display_.display(false);
  if (passes > 0) {
    for (int i = 0; i < passes; i++) {
      delay(250);
      display_.display(true);
    }
  }

  delay(100);
  display_.setFullWindow();
  display_.fillScreen(GxEPD_WHITE);
  display_.hibernate();
}
void PocketmageEink::setFastFullRefresh(bool setting) {
  PanelT::useFastFullUpdate = setting;
  /*if (PanelT::useFastFullUpdate != setting) {
    PanelT::useFastFullUpdate = setting;
  }*/
}
void PocketmageEink::statusBar(const String& input, bool fullWindow) {
  setTXTFont(&FreeMonoBold9pt7b);
  if (!fullWindow){
    display_.setPartialWindow(0, display_.height() - 20, display_.width(), 20);
    display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
    display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
    display_.setCursor(4, display_.height() - 6);
    display_.print(input);
  }

  display_.drawRect(display_.width() - 30, display_.height() - 20, 30, 20, GxEPD_BLACK);
}
void PocketmageEink::drawStatusBar(const String& input) {
  display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
  display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
  setTXTFont(&FreeMonoBold9pt7b);
  display_.setCursor(4, display_.height() - 6);
  display_.print(input);
}
void PocketmageEink::computeFontMetrics_() {
  int16_t x1, y1; 
  uint16_t charWidth, charHeight;
  // GET AVERAGE CHAR WIDTH
  display_.getTextBounds("abcdefghijklmnopqrstuvwxyz", 0, 0, &x1, &y1, &charWidth, &charHeight);
  charWidth = charWidth / 52; // check if intended 
  maxCharsPerLine_  = display_.width() / charWidth;

  display_.getTextBounds("H", 0, 0, &x1, &y1, &charWidth, &charHeight);
  fontHeight_ = charHeight;
  maxLines_   = (display_.height() - 26) / (fontHeight_ + lineSpacing_);
}
void PocketmageEink::setTXTFont(const GFXfont* font) {
  // SET THE FONT
  const bool changed = (currentFont_ != font);
  currentFont_ = font; 
  display_.setFont(currentFont_); 
  // maxCharsPerLine and maxLines
  if (changed) computeFontMetrics_();
}
void PocketmageEink::einkTextDynamic(bool doFull, bool noRefresh) {
  if (!currentFont_) return;
  
   // SET FONT
  setTXTFont(currentFont_);

  // ITERATE AND DISPLAY
  uint8_t size = allLines.size();
  uint8_t displayLines = maxLines_;

  if (displayLines > size) displayLines = size;

  int scrollOffset = TOUCH().getDynamicScroll();
  if (scrollOffset < 0) scrollOffset = 0;
  if (scrollOffset > size - displayLines) scrollOffset = size - displayLines;
  

  if (doFull) {
    display_.fillScreen(GxEPD_WHITE);
    for (uint8_t i = size - displayLines - scrollOffset; i < size - scrollOffset; i++) {
      if ((allLines)[i].length() == 0) continue;
      display_.setFullWindow();
      //display_.fillRect(0, (fontHeight_ + lineSpacing_) * (i - (size - displayLines - scrollOffset)), display.width(), (fontHeight_ + lineSpacing_), GxEPD_WHITE)
      display_.setCursor(0, fontHeight_ + ((fontHeight_ + lineSpacing_) * (i - (size - displayLines - scrollOffset))));
      display_.print((allLines)[i]);
    }
  } 
  // PARTIAL REFRESH, ONLY SEND LAST LINE
  else {
    if ((allLines)[size - displayLines - scrollOffset].length() > 0) {
      int y = (fontHeight_ + lineSpacing_) * (size - displayLines - scrollOffset);
      display_.setPartialWindow(0, y, display_.width(), (fontHeight_ + lineSpacing_));
      display_.fillRect(0, y, display_.width(), (fontHeight_ + lineSpacing_), GxEPD_WHITE);
      display_.setCursor(0, fontHeight_ + y);
      display_.print((allLines)[size - displayLines - scrollOffset]);
    }
  }
  

  drawStatusBar(String("L:") + String(allLines.size()) + " " + PM_SDAUTO().getEditingFile());
}

void PocketmageEink::resetDisplay(bool clearScreen, uint16_t color) {
  display_.setRotation(3);
  display_.setFullWindow();
  if (clearScreen) display_.fillScreen(color);
}
int PocketmageEink::countLines(const String& input, size_t maxLineLength) {
  size_t inputLength = input.length();
  uint8_t charCounter = 0;
  uint16_t lineCounter = 1;
  for (size_t c = 0; c < inputLength; c++) {
    if (input[c] == '\n') { 
        charCounter = 0; 
        lineCounter++; 
        continue;
    }
    if (charCounter > (maxLineLength - 1)) { 
        charCounter = 0; 
        lineCounter++; 
    }
    charCounter++;
  }

  return lineCounter;
}
void PocketmageEink::forceSlowFullUpdate(bool force)            { forceSlowFullUpdate_ = force; }

// Setup for Eink Class
void setupEink() {
  display.init(115200);
  display.setRotation(3);
  display.setFullWindow();
  display.setTextColor(GxEPD_BLACK);
  EINK().setTXTFont(&FreeMonoBold9pt7b); // default font, computeFontMetrics_()

  xTaskCreatePinnedToCore(
    einkHandler,             // Function name
    "einkHandlerTask",       // Task name
    10000,                   // Stack size
    NULL,                    // Parameters 
    1,                       // Priority 
    &einkHandlerTaskHandle,  // Task handle
    0                        // Core ID 
  );

}

uint16_t PocketmageEink::getEinkTextWidth(const String& s) {
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// ===================== getter functions =====================
const GFXfont* PocketmageEink::getCurrentFont() { return currentFont_; }
uint8_t PocketmageEink::maxCharsPerLine() const { return maxCharsPerLine_; }
uint8_t PocketmageEink::maxLines()        const { return maxLines_; }
