// AUDIT 1

#include <globals.h>
#include <vector>
#include <pocketmage_ui/pocketmage_ui.h>

#if !OTA_APP // POCKETMAGE_OS

enum class GuideItemType { TEXT, KEY_COMBO, IMAGE };

struct GuideItem {
  GuideItemType type;
  StringID str1;              // Primary Text or Key string
  StringID str2;              // Description string
  const unsigned char* bmp;   // Bitmap pointer
  int w;                      // Bitmap width
  int h;                      // Bitmap height
};

struct GuidePage {
  StringID title;
  std::vector<GuideItem> items;
};

static std::vector<GuidePage> guidePages;
static int currentGuidePage = 0;
static ulong guideScrollPixel = 0;

// --- Helper Functions to Easily Build Pages ---
static void addKey(GuidePage& p, StringID key, StringID desc) {
  p.items.push_back({GuideItemType::KEY_COMBO, key, desc, nullptr, 0, 0});
}
static void addText(GuidePage& p, StringID text) {
  p.items.push_back({GuideItemType::TEXT, text, (StringID)0, nullptr, 0, 0});
}
static void addImg(GuidePage& p, const unsigned char* bmp, int w, int h) {
  p.items.push_back({GuideItemType::IMAGE, (StringID)0, (StringID)0, bmp, w, h});
}

void buildGuidePages() {
  guidePages.clear();

  // PAGE 1: General OS
  GuidePage pGen;
  pGen.title = STR_GUIDE_TITLE_GENERAL;
  addKey(pGen, STR_GUIDE_KEY_LEFT, STR_GUIDE_DESC_CURSOR_L);
  addKey(pGen, STR_GUIDE_KEY_RIGHT, STR_GUIDE_DESC_CURSOR_R);
  addKey(pGen, STR_GUIDE_KEY_SHFT_L, STR_GUIDE_DESC_LINE_START);
  addKey(pGen, STR_GUIDE_KEY_SHFT_R, STR_GUIDE_DESC_LINE_END);
  addKey(pGen, STR_GUIDE_KEY_FN_O, STR_GUIDE_DESC_CLEAR_LINE);
  addKey(pGen, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  addKey(pGen, STR_GUIDE_KEY_FN_R, STR_GUIDE_DESC_SAVE);
  addKey(pGen, STR_GUIDE_KEY_SHFT_K, STR_GUIDE_DESC_CAPITAL);
  addKey(pGen, STR_GUIDE_KEY_FN_K, STR_GUIDE_DESC_FN_LAYER);
  addKey(pGen, STR_GUIDE_KEY_FNSHFT_K, STR_GUIDE_DESC_FNSHFT_LAYER);
  addKey(pGen, STR_GUIDE_KEY_ENTER, STR_GUIDE_DESC_SELECT);
  guidePages.push_back(pGen);

  // PAGE 2: Sleep / Wakeup
  GuidePage pSleep;
  pSleep.title = STR_GUIDE_TITLE_SLEEP;
  addText(pSleep, STR_GUIDE_DESC_SLEEP_HINT);
  addKey(pSleep, STR_GUIDE_KEY_SPACE, STR_GUIDE_DESC_RESUME);
  addKey(pSleep, STR_GUIDE_KEY_H, STR_GUIDE_DESC_HOME);
  addKey(pSleep, STR_GUIDE_KEY_U, STR_GUIDE_DESC_USB);
  addKey(pSleep, STR_GUIDE_KEY_F, STR_GUIDE_DESC_FILEWIZ);
  addKey(pSleep, STR_GUIDE_KEY_T, STR_GUIDE_DESC_TASKS);
  addKey(pSleep, STR_GUIDE_KEY_N, STR_GUIDE_DESC_TXT);
  addKey(pSleep, STR_GUIDE_KEY_S, STR_GUIDE_DESC_SETTINGS);
  addKey(pSleep, STR_GUIDE_KEY_C, STR_GUIDE_DESC_CALENDAR);
  addKey(pSleep, STR_GUIDE_KEY_J, STR_GUIDE_DESC_JOURNAL);
  addKey(pSleep, STR_GUIDE_KEY_D, STR_GUIDE_DESC_LEXICON);
  addKey(pSleep, STR_GUIDE_KEY_X, STR_GUIDE_DESC_TERMINAL);
  addKey(pSleep, STR_GUIDE_KEY_L, STR_GUIDE_DESC_LOADER);
  guidePages.push_back(pSleep);

  // PAGE 3: Home App
  GuidePage pHome;
  pHome.title = STR_GUIDE_TITLE_HOME;
  addText(pHome, STR_GUIDE_DESC_HOME_HINT);
  addText(pHome, STR_GUIDE_DESC_HOME_APPS);
  guidePages.push_back(pHome);

  // PAGE 4: TXT App
  GuidePage pTxt;
  pTxt.title = STR_GUIDE_TITLE_TXT;
  addKey(pTxt, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  addKey(pTxt, STR_GUIDE_KEY_FN_R, STR_GUIDE_DESC_SAVE);
  addKey(pTxt, STR_GUIDE_KEY_FN_O, STR_GUIDE_DESC_TXT_FS);
  addKey(pTxt, STR_GUIDE_KEY_SHFT_O, STR_GUIDE_DESC_TXT_NEW);
  addKey(pTxt, STR_GUIDE_KEY_FN_TAB, STR_GUIDE_DESC_TXT_FONT);
  addKey(pTxt, STR_GUIDE_KEY_FNSHFT_O, STR_GUIDE_DESC_TXT_JUMP);
  addKey(pTxt, STR_GUIDE_KEY_ENTER_NAME, STR_GUIDE_DESC_TXT_LINE);
  addKey(pTxt, STR_GUIDE_KEY_SHFT_L, STR_GUIDE_DESC_TXT_STYLE);
  addKey(pTxt, STR_GUIDE_KEY_SHFT_R, STR_GUIDE_DESC_TXT_FORMAT);
  addKey(pTxt, STR_GUIDE_KEY_SLIDER, STR_GUIDE_DESC_SCROLL);
  guidePages.push_back(pTxt);

  // PAGE 5: FileWiz
  GuidePage pFilewiz;
  pFilewiz.title = STR_GUIDE_TITLE_FILEWIZ;
  addKey(pFilewiz, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  addKey(pFilewiz, STR_GUIDE_KEY_LR, STR_GUIDE_DESC_FW_SCROLL);
  addKey(pFilewiz, STR_GUIDE_KEY_ENTER, STR_GUIDE_DESC_FW_SELECT);
  addKey(pFilewiz, STR_GUIDE_KEY_0_9, STR_GUIDE_DESC_FW_RECENT);
  addKey(pFilewiz, STR_GUIDE_KEY_BKSP, STR_GUIDE_DESC_FW_BACK);
  guidePages.push_back(pFilewiz);

  // PAGE 6: USB
  GuidePage pUSB;
  pUSB.title = STR_GUIDE_TITLE_USB;
  addText(pUSB, STR_GUIDE_DESC_USB_HINT);
  addKey(pUSB, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pUSB);

  // PAGE 7: COMM
  GuidePage pCOMM;
  pCOMM.title = STR_GUIDE_TITLE_COMM;
  addText(pCOMM, STR_GUIDE_DESC_COMM_HINT);
  addKey(pCOMM, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pCOMM);

  // PAGE 8: Settings
  GuidePage pSettings;
  pSettings.title = STR_GUIDE_TITLE_SETTINGS;
  addText(pSettings, STR_GUIDE_DESC_SET_HINT);
  addKey(pSettings, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pSettings);

  // PAGE 9: Tasks
  GuidePage pTasks;
  pTasks.title = STR_GUIDE_TITLE_TASKS;
  addKey(pTasks, STR_GUIDE_KEY_N, STR_GUIDE_DESC_TSK_NEW);
  addKey(pTasks, STR_GUIDE_KEY_ENTER_NAME, STR_GUIDE_DESC_TSK_ENTER);
  addKey(pTasks, STR_GUIDE_KEY_0_9, STR_GUIDE_DESC_TSK_EDIT);
  addKey(pTasks, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pTasks);

  // PAGE 10: Calendar
  GuidePage pCalendar;
  pCalendar.title = STR_GUIDE_TITLE_CALENDAR;
  addText(pCalendar, STR_GUIDE_DESC_CAL_MONTH);
  addText(pCalendar, STR_GUIDE_DESC_CAL_WEEK);
  addText(pCalendar, STR_GUIDE_DESC_CAL_DAY);
  addText(pCalendar, STR_GUIDE_DESC_CAL_REP);
  addKey(pCalendar, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pCalendar);

  // PAGE 11: Journal
  GuidePage pJournal;
  pJournal.title = STR_GUIDE_TITLE_JOURNAL;
  addText(pJournal, STR_GUIDE_DESC_JRN_HINT);
  addKey(pJournal, STR_GUIDE_KEY_T, STR_GUIDE_DESC_JRN_TODAY);
  addKey(pJournal, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pJournal);

  // PAGE 12: Lexicon
  GuidePage pLexicon;
  pLexicon.title = STR_GUIDE_TITLE_LEXICON;
  addText(pLexicon, STR_GUIDE_DESC_LEX_HINT);
  addKey(pLexicon, STR_GUIDE_KEY_ENTER_NAME, STR_GUIDE_DESC_LEX_SEARCH);
  addKey(pLexicon, STR_GUIDE_KEY_LR, STR_GUIDE_DESC_LEX_PREVNEXT);
  addKey(pLexicon, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pLexicon);

  // PAGE 13: App Loader
  GuidePage pLoader;
  pLoader.title = STR_GUIDE_TITLE_LOADER;
  addText(pLoader, STR_GUIDE_DESC_LDR_HINT);
  addKey(pLoader, STR_GUIDE_KEY_A_D, STR_GUIDE_DESC_LDR_SLOT);
  addKey(pLoader, STR_GUIDE_KEY_S, STR_GUIDE_DESC_LDR_SWAP);
  addKey(pLoader, STR_GUIDE_KEY_D, STR_GUIDE_DESC_LDR_DEL);
  addKey(pLoader, STR_GUIDE_KEY_FN_L, STR_GUIDE_DESC_EXIT);
  guidePages.push_back(pLoader);

  // PAGE 14: Sleep Modes
  GuidePage pSleepModes;
  pSleepModes.title = STR_GUIDE_TITLE_MODES;
  addText(pSleepModes, STR_GUIDE_DESC_MODES_HINT);
  guidePages.push_back(pSleepModes);
}

void GUIDE_INIT() {
  CurrentAppState = GUIDE;
  currentGuidePage = 0;
  guideScrollPixel = 0;
  
  if (guidePages.empty()) {
    buildGuidePages();
  }
  
  KB().setKeyboardState(NORMAL);
  newState = true;
}

void processKB_GUIDE() {
  pocketmage::setCpuSpeed(240);
  char inchar = KB().updateKeypress();
  if (inchar == 0) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }

  int currentMillis = millis();

  // 1. Calculate max scroll limit dynamically based on page contents
  int totalH = 0;
  for (auto& item : guidePages[currentGuidePage].items) {
    if (item.type == GuideItemType::IMAGE) totalH += item.h + 10;
    else if (item.type == GuideItemType::KEY_COMBO) totalH += 34; // Chip height + padding
    else {
      // Use the utility to measure how tall wrapped text gets
      std::vector<String> lines = wordWrap(TR(item.str1), kEinkWidth - 20, FontStyle::Body);
      totalH += lines.size() * einkRowPitch(FontStyle::Body) + 10;
    }
  }
  
  // kEinkHeight - 40 ensures we leave room for the status bar and margins
  int maxScroll = max(0, totalH - (kEinkHeight - 40));

  // Update hardware touchpad scroll (pixel-based for smoothness)
  if (TOUCH().updateScroll(maxScroll, guideScrollPixel, 15)) {
    newState = true;
  }

  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    if (inchar != 0) {
      KBBounceMillis = currentMillis;

      if (inchar == 12) { // FN + < (Exit App)
        HOME_INIT();
        return;
      } 
      else if (inchar == 19) { // Left arrow (Previous Page)
        if (currentGuidePage > 0) {
          currentGuidePage--;
          guideScrollPixel = 0; // Reset scroll on page turn
          newState = true;
        }
      } 
      else if (inchar == 21) { // Right arrow (Next Page)
        if (currentGuidePage < (int)guidePages.size() - 1) {
          currentGuidePage++;
          guideScrollPixel = 0; // Reset scroll on page turn
          newState = true;
        }
      }
    }
  }

  // 2. OLED Real-time Update
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;
    u8g2.clearBuffer();

    // Draw Left Pagination Button (White outline with white '<' creates a black circle on OLED)
    if (currentGuidePage > 0) {
      u8g2.setDrawColor(1);
      u8g2.drawCircle(16, 16, 12);
      FontEngine::drawText(DisplayTarget::OLED, 12, 21, "<", FontStyle::BodyBold);
    }

    // Draw Right Pagination Button
    if (currentGuidePage < (int)guidePages.size() - 1) {
      u8g2.setDrawColor(1);
      u8g2.drawCircle(240, 16, 12); 
      FontEngine::drawText(DisplayTarget::OLED, 236, 21, ">", FontStyle::BodyBold);
    }

    // Draw Current Page Title (Centered)
    u8g2.setDrawColor(1);
    String title = TR(guidePages[currentGuidePage].title);
    int tw = FontEngine::textWidth(DisplayTarget::OLED, title, FontStyle::BodyBold);
    FontEngine::drawText(DisplayTarget::OLED, (256 - tw) / 2, 21, title, FontStyle::BodyBold);

    u8g2.sendBuffer();
  }
}

void einkHandler_GUIDE() {
  if (newState) {
    newState = false;
    beginEinkScreen();
    display.fillScreen(GxEPD_WHITE);

    // Apply scroll offset and initial top margin
    int y = 20 - guideScrollPixel; 

    for (auto& item : guidePages[currentGuidePage].items) {
      int itemH = 0;
      std::vector<String> wrappedText;

      if (item.type == GuideItemType::TEXT) {
        wrappedText = wordWrap(TR(item.str1), kEinkWidth - 20, FontStyle::Body);
        itemH = wrappedText.size() * einkRowPitch(FontStyle::Body) + 10;
      }
      else if (item.type == GuideItemType::KEY_COMBO) itemH = 34;
      else if (item.type == GuideItemType::IMAGE) itemH = item.h + 10;

      // Only draw the item if it's visible on the screen (Culling for speed)
      if (y + itemH > 0 && y < kEinkHeight - 26) {
        
        if (item.type == GuideItemType::TEXT) {
          int lineY = y + 16;
          for (const String& line : wrappedText) {
             FontEngine::drawText(DisplayTarget::EINK, 10, lineY, line, FontStyle::Body);
             lineY += einkRowPitch(FontStyle::Body);
          }
        }
        else if (item.type == GuideItemType::KEY_COMBO) {
          String keyText = TR(item.str1);
          String descText = TR(item.str2);

          // Draw Inverted Badge (Creates the "Black circle with white letter" effect natively)
          int nextX = 10;
          nextX += drawChipText(nextX, y + 22, keyText, FontStyle::BodyBold, -1, true, -1, 10, 26, 6) + 12;

          // Draw Description aligned to the badge
          FontEngine::drawText(DisplayTarget::EINK, nextX, y + 22, descText, FontStyle::Body);
        }
        else if (item.type == GuideItemType::IMAGE) {
          display.drawBitmap(10, y, item.bmp, item.w, item.h, GxEPD_BLACK);
        }
      }
      y += itemH;
    }

    // Calculate absolute bounds for scrollbar rendering
    int totalH = y + guideScrollPixel - 20; 
    if (totalH > kEinkHeight - 40) {
      drawScrollbar(totalH, kEinkHeight - 40, guideScrollPixel, 314, 0, kEinkHeight - 26, 4, true, GxEPD_BLACK, GxEPD_WHITE);
    }

    String pageStatus = String(currentGuidePage + 1) + " / " + String(guidePages.size());
    endEinkScreen(pageStatus.c_str());
  }
}
#endif