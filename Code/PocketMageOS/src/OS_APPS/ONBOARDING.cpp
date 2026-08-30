#include <globals.h>
#include <vector>

#if !OTA_APP // POCKETMAGE_OS

// First-boot setup wizard.

enum OnboardStep {
  ONB_WELCOME,
  ONB_LANGUAGE,
  ONB_WIFI_ASK,
  ONB_WIFI,
  ONB_WIFI_PASS,
  ONB_WIFI_CONNECTING,
  ONB_WIFI_CONNECTED,
  ONB_DATETIME,
  ONB_CONTROLS,
  ONB_SDCARD,
  ONB_DONE
};

constexpr int kOnboardStepCount = 8;

enum OnboardScanPhase { SCAN_IDLE, SCAN_QUEUED, SCAN_RUNNING, SCAN_DONE };

struct OnboardState {
  OnboardStep step         = ONB_WELCOME;
  ulong index              = 0;   // cursor for the language / Wi-Fi lists
  OnboardScanPhase scan    = SCAN_IDLE;
  unsigned long phaseStart = 0;   // scan watchdog anchor
  String pendingSSID       = "";
  String joinedSSID        = "";
  String joinedIP          = "";
  unsigned long connectDeadline = 0;
  int dots                 = 0;
  unsigned long dotsAt     = 0;
  String oledMsg           = "";  // canonical OLED line, repainted when wiped
  bool oledLarge           = false;
};

static OnboardState ob;

// Layout (e-ink 320x240, status bar occupies the bottom 26px)
constexpr int ONB_TITLE_BASE     = 26;
constexpr int ONB_DIVIDER_Y      = 34;
constexpr int ONB_CONTENT_TOP_Y  = 64;
constexpr int ONB_LANG_Y0        = 76;
constexpr int ONB_LANG_PITCH     = 30;
constexpr int ONB_WIFI_Y0        = 64;
constexpr int ONB_WIFI_PITCH     = 26;
constexpr int ONB_WIFI_VISIBLE   = 5;
constexpr int ONB_RSSI_GUTTER    = 58;
constexpr int ONB_RIGHT_COL_X    = 198;
constexpr int ONB_RIGHT_COL_W    = kEinkWidth - ONB_RIGHT_COL_X - 10;
constexpr unsigned long ONB_SCAN_START_TIMEOUT_MS = 10000;
constexpr unsigned long ONB_WIFI_TIMEOUT_MS       = 20000;

// Controls page: fixed key-chip column + description column
constexpr int ONB_CTRL_Y0        = 66;
constexpr int ONB_CTRL_PITCH     = 26;
constexpr int ONB_KEY_X          = 18;
constexpr int ONB_KEY_CHIP_MAX_W = 84;
constexpr int ONB_DESC_X         = 112;

// Header cascade capped one size below Heading2 so localized titles never
// collide with the step indicator.
static constexpr FontStyle kTitleCascade[] = {FontStyle::Heading3, FontStyle::Caption};
static constexpr int kTitleCascadeCount =
    sizeof(kTitleCascade) / sizeof(kTitleCascade[0]);

static void resetOnboardState() {
  ob = OnboardState{};
}

static void setOledMsg(const String& msg, bool allowLarge = false) {
  ob.oledMsg   = msg;
  ob.oledLarge = allowLarge;
  OLED().oledWord(msg, allowLarge, false);
}

static int onboardStepNum() {
  switch (ob.step) {
    case ONB_WELCOME:         return 1;
    case ONB_LANGUAGE:        return 2;
    case ONB_WIFI_ASK:        return 3;
    case ONB_WIFI:
    case ONB_WIFI_PASS:
    case ONB_WIFI_CONNECTING:
    case ONB_WIFI_CONNECTED:  return 4;
    case ONB_DATETIME:        return 5;
    case ONB_CONTROLS:        return 6;
    case ONB_SDCARD:          return 7;
    case ONB_DONE:            return 8;
  }
  return 0;
}

static void onboardFinish() {
  prefs.begin("PocketMage", false);
  prefs.putBool("Onboarded", true);
  prefs.end();
  HOME_INIT();
}

static void enterStep(OnboardStep step) {
  ob.step = step;
  ob.index = 0;

  switch (step) {
    case ONB_WELCOME:
      setOledMsg("PocketMage", true);
      break;
    case ONB_LANGUAGE:
      setOledMsg(TR(STR_ONBOARD_LANG_TITLE));
      break;
    case ONB_WIFI_ASK:
      setOledMsg(TR(STR_ONBOARD_WIFIASK_TITLE));
      break;
    case ONB_WIFI:
      ob.pendingSSID = "";
      // Already associated (auto-reconnect beat us to it): show status
      // instead of a pointless rescan.
      if (P_WIFI.getState() == WifiRadioState::Connected) {
        enterStep(ONB_WIFI_CONNECTED);
        return;
      }
      // Radio may already be on (re-run, retry after failure); commands are
      // FIFO so Scan lands after Enable completes.
      if (P_WIFI.getState() == WifiRadioState::Off) P_WIFI.enable();
      P_WIFI.scan();
      ob.scan = SCAN_QUEUED;
      ob.phaseStart = millis();
      setOledMsg(TR(STR_ONBOARD_WIFI_TITLE));
      break;
    case ONB_WIFI_CONNECTED:
      ob.joinedSSID = P_WIFI.getConnectedSSID();
      ob.joinedIP   = P_WIFI.getIpAddress();
      setOledMsg(truncateWithEllipsis(String(TR(STR_ONBOARD_WIFI_CONNECTED)) + " " + ob.joinedSSID,
                                      kOledWidth - 16, FontStyle::Tiny, DisplayTarget::OLED));
      break;
    case ONB_DATETIME:
      setOledMsg(TR(STR_ONBOARD_TIME_TITLE));
      break;
    case ONB_CONTROLS:
      setOledMsg(TR(STR_ONBOARD_CTRL_TITLE));
      break;
    case ONB_SDCARD:
      setOledMsg(TR(STR_ONBOARD_SD_TITLE));
      break;
    default:
      break;
  }
  newState = true;
}

static void startConnect(const char* ssid, const char* pass) {
  ob.pendingSSID = ssid;
  P_WIFI.connect(ssid, pass, true);
  ob.connectDeadline = millis() + ONB_WIFI_TIMEOUT_MS;
  ob.dots = 0;
  ob.step = ONB_WIFI_CONNECTING;
  newState = true;
}

static void skipWifi() {
  // Leave a working connection (and its saved credentials) alone; kill the
  // radio otherwise so an abandoned wizard doesn't drain the battery.
  if (!P_WIFI.isConnected()) P_WIFI.disable();
  enterStep(ONB_DATETIME);
}

void ONBOARDING_INIT() {
  CurrentAppState = ONBOARDING;
  KB().setKeyboardState(NORMAL);
  resetOnboardState();
  newState = true;
  setOledMsg("PocketMage", true);
}

static void handleOnboardKey(char inchar) {
  if (inchar == 12 || inchar == 23) return;  // exit / app-switcher: swallowed

  switch (ob.step) {
    case ONB_WELCOME:
      if (inchar == 13 || inchar == 20) enterStep(ONB_LANGUAGE);
      else if (inchar == 21)            onboardFinish();
      break;

    case ONB_LANGUAGE:
      if (inchar == 13 || inchar == 20) {
        if (ob.index >= (ulong)I18n::languageCount()) break;
        I18n::setLanguage(static_cast<Lang>(ob.index));
        prefs.begin("PocketMage", false);
        prefs.putInt("Language", static_cast<int>(I18n::language()));
        prefs.end();
        enterStep(ONB_WIFI_ASK);
      }
      else if (inchar == 21) enterStep(ONB_WIFI_ASK);
      else if (inchar == 19) enterStep(ONB_WELCOME);
      break;

    case ONB_WIFI_ASK:
      // index 0 = "No" (default), 1 = "Yes"
      if (inchar == 13 || inchar == 20) {
        if (ob.index == 0) skipWifi();
        else               enterStep(ONB_WIFI);
      }
      else if (inchar == 21) skipWifi();
      else if (inchar == 19) enterStep(ONB_LANGUAGE);
      break;

    case ONB_WIFI:
      if (inchar == 21 && ob.scan != SCAN_DONE) {
        enterStep(ONB_DATETIME);  // skip while scanning: don't touch the radio
      }
      else if (inchar == 21) {
        skipWifi();
      }
      else if (inchar == 19) {
        enterStep(ONB_WIFI_ASK);
      }
      else if (inchar == 13 || inchar == 20) {
        WifiApInfo ap;
        if (ob.scan == SCAN_DONE && P_WIFI.getScanResult(ob.index, ap)) {
          if (ap.authmode == WIFI_AUTH_OPEN) {
            startConnect(ap.ssid, "");
          } else {
            String pass = textPrompt(String(TR(STR_ONBOARD_WIFI_PASS_PROMPT)) + " " + ap.ssid, "", true);
            KB().setKeyboardState(NORMAL);
            if (pass != "_EXIT_" && pass != "_RETURN_" && pass != "_CENTER_" && pass.length() > 0) {
              startConnect(ap.ssid, pass.c_str());
            } else {
              newState = true;  // aborted prompt: back to the network list
            }
          }
        }
      }
      break;

    case ONB_WIFI_PASS:
      break;  // input owned by the blocking textPrompt

    case ONB_WIFI_CONNECTING:
      if (inchar == 21) skipWifi();
      break;

    case ONB_WIFI_CONNECTED:
      if (inchar == 19) enterStep(ONB_WIFI_ASK);
      else if (inchar == 13 || inchar == 20 || inchar == 21) enterStep(ONB_DATETIME);
      break;

    case ONB_DATETIME:
      if (inchar == 13 || inchar == 20) {
        runClockSetupFlow(false);  // wizard screen already asked; go straight to prompts
        KB().setKeyboardState(NORMAL);
        enterStep(ONB_CONTROLS);
      }
      else if (inchar == 21) enterStep(ONB_CONTROLS);
      else if (inchar == 19) enterStep(ONB_WIFI);
      break;

    case ONB_CONTROLS:
      if (inchar == 19) enterStep(ONB_DATETIME);
      else if (inchar == 13 || inchar == 20 || inchar == 21) enterStep(ONB_SDCARD);
      break;

    case ONB_SDCARD:
      if (inchar == 19) enterStep(ONB_CONTROLS);
      else if (inchar == 13 || inchar == 20 || inchar == 21) enterStep(ONB_DONE);
      break;

    case ONB_DONE:
      onboardFinish();
      break;
  }
}

static void pollOnboarding() {
  switch (ob.step) {
    case ONB_WIFI: {
      // The service's auto-connect may land a saved network while we wait
      // for scan results; adopt it instead of fighting over the radio.
      if (ob.scan != SCAN_IDLE && P_WIFI.getState() == WifiRadioState::Connected) {
        ob.joinedSSID = P_WIFI.getConnectedSSID();
        ob.joinedIP   = P_WIFI.getIpAddress();
        OLED().sysMessage(String(TR(STR_ONBOARD_WIFI_CONNECTED)) + " " + ob.joinedSSID, 1200);
        enterStep(ONB_DATETIME);
        break;
      }
      switch (ob.scan) {
        case SCAN_QUEUED:
          if (P_WIFI.getState() == WifiRadioState::Scanning) {
            ob.scan = SCAN_RUNNING;
          } else if (millis() - ob.phaseStart > ONB_SCAN_START_TIMEOUT_MS) {
            ob.scan = SCAN_DONE;  // radio never came up; show "no networks"
            newState = true;
            setOledMsg(TR(STR_ONBOARD_WIFI_NONE));
          }
          break;
        case SCAN_RUNNING:
          if (P_WIFI.getState() != WifiRadioState::Scanning) {
            ob.scan = SCAN_DONE;
            ob.index = 0;
            newState = true;
            // Terminal OLED state: dots loop stops here, so replace the
            // "Scanning..." message explicitly.
            if (P_WIFI.getScanResultCount() == 0)
              setOledMsg(TR(STR_ONBOARD_WIFI_NONE));
            else
              setOledMsg(TR(STR_ONBOARD_WIFI_TITLE));
          }
          break;
        default:
          break;
      }
      break;
    }

    case ONB_WIFI_CONNECTING: {
      if (P_WIFI.isConnected()) {
        ob.joinedSSID = P_WIFI.getConnectedSSID();
        ob.joinedIP   = P_WIFI.getIpAddress();
        OLED().sysMessage(String(TR(STR_ONBOARD_WIFI_CONNECTED)) + " " + ob.joinedSSID, 1200);
        enterStep(ONB_DATETIME);
      } else if ((long)(millis() - ob.connectDeadline) >= 0) {
        OLED().sysMessage(TR(STR_ONBOARD_WIFI_FAILED), 1500);
        enterStep(ONB_WIFI);  // fresh scan; user can retry or skip
      }
      break;
    }

    default:
      break;
  }
}

// Live OLED feedback while dragging the touch slider; updateScroll only
// reports on release + timeout, so without this preview scrolling looks dead.
static void oledScrollPreview(const String& name, int count) {
  String msg = name + "  " + String(ob.index + 1) + "/" + String(count);
  OLED().oledWord(truncateWithEllipsis(msg, kOledWidth - 16, FontStyle::Large, DisplayTarget::OLED),
                  false, false);
}

void processKB_ONBOARDING() {
  pocketmage::setCpuSpeed(240);
  char inchar = KB().updateKeypress();
  if (inchar == 0) {
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
  }

  // List scrolling (touch slider), matching the SETTINGS/TASKS convention
  if (ob.step == ONB_LANGUAGE) {
    int scrollStep = (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) ? 5 : 1;
    if (TOUCH().updateScroll(I18n::languageCount() - 1, ob.index, scrollStep)) newState = true;
  }
  else if (ob.step == ONB_WIFI_ASK) {
    if (TOUCH().updateScroll(1, ob.index, 1)) newState = true;
  }
  else if (ob.step == ONB_WIFI && ob.scan == SCAN_DONE && P_WIFI.getScanResultCount() > 0) {
    int scrollStep = (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT) ? 5 : 1;
    if (TOUCH().updateScroll(P_WIFI.getScanResultCount() - 1, ob.index, scrollStep)) newState = true;
  }

  pollOnboarding();

  int currentMillis = millis();
  if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
    if (inchar != 0) {
      KBBounceMillis = currentMillis;
      handleOnboardKey(inchar);
    }
  }

  // Live list preview while dragging the touch slider, at OLED frame rate
  currentMillis = millis();
  if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
    OLEDFPSMillis = currentMillis;
    if (TOUCH().getLastTouch() != -1) {
      if (ob.step == ONB_LANGUAGE) {
        oledScrollPreview(I18n::nativeName((int)ob.index), I18n::languageCount());
      }
      else if (ob.step == ONB_WIFI_ASK) {
        oledScrollPreview(ob.index == 0 ? TR(STR_ONBOARD_WIFIASK_NO) : TR(STR_ONBOARD_WIFIASK_YES), 2);
      }
      else if (ob.step == ONB_WIFI && ob.scan == SCAN_DONE && P_WIFI.getScanResultCount() > 0) {
        WifiApInfo ap;
        if (P_WIFI.getScanResult(ob.index, ap))
          oledScrollPreview(ap.ssid, P_WIFI.getScanResultCount());
      }
    }
    // Idle repaint
    else if (!ob.oledMsg.isEmpty()) {
      OLED().oledWord(ob.oledMsg, ob.oledLarge, false);
    }
  }

  // OLED progress dots for the two waiting states
  bool waiting = (ob.step == ONB_WIFI && (ob.scan == SCAN_QUEUED || ob.scan == SCAN_RUNNING))
              || (ob.step == ONB_WIFI_CONNECTING);
  if (waiting && currentMillis - ob.dotsAt >= 500) {
    ob.dotsAt = currentMillis;
    ob.dots = (ob.dots + 1) % 4;
    String dots;
    for (int i = 0; i < ob.dots; i++) dots += ".";
    String msg = (ob.step == ONB_WIFI)
        ? String(TR(STR_ONBOARD_WIFI_SCANNING))
        : String(TR(STR_ONBOARD_WIFI_CONNECTING)) + " " + ob.pendingSSID + dots;
    setOledMsg(truncateWithEllipsis(msg, kOledWidth - 16, FontStyle::Tiny, DisplayTarget::OLED));
  }
}

static void drawOnboardChrome(const char* title) {
  FontStyle style = FontEngine::fitStyle(DisplayTarget::EINK, title, kEinkWidth - 70,
                                         kTitleCascade, kTitleCascadeCount);
  FontEngine::drawText(DisplayTarget::EINK, 10, ONB_TITLE_BASE, title, style);
  String idx = String(onboardStepNum()) + "/" + String(kOnboardStepCount);
  int w = FontEngine::textWidth(DisplayTarget::EINK, idx, FontStyle::Caption);
  FontEngine::drawText(DisplayTarget::EINK, kEinkWidth - 10 - w, ONB_TITLE_BASE, idx, FontStyle::Caption);
  display.drawFastHLine(10, ONB_DIVIDER_Y, kEinkWidth - 20, GxEPD_BLACK);
}

static void drawCentered(const char* text, int y, FontStyle style) {
  int w = FontEngine::textWidth(DisplayTarget::EINK, text, style);
  FontEngine::drawText(DisplayTarget::EINK, (kEinkWidth - w) / 2, y, text, style);
}

static void drawWrappedBlock(const char* text, int x, int y, int maxWidth, FontStyle style) {
  std::vector<String> lines = wordWrap(text, maxWidth, style);
  for (const String& line : lines) {
    FontEngine::drawText(DisplayTarget::EINK, x, y, line, style);
    y += einkRowPitch(style);
  }
}

static void drawWelcomePage() {
  display.drawBitmap(8, 4, onboardMage, 185, 204, GxEPD_BLACK);

  FontEngine::drawText(DisplayTarget::EINK, ONB_RIGHT_COL_X, 80,
                       TR(STR_ONBOARD_WELCOME), FontStyle::Heading3);
  drawWrappedBlock(TR(STR_ONBOARD_WELCOME_SUB), ONB_RIGHT_COL_X, 112,
                   ONB_RIGHT_COL_W, FontStyle::Body);
}

static void drawWifiAskPage() {
  drawOnboardChrome(TR(STR_ONBOARD_WIFIASK_TITLE));

  drawWrappedBlock(TR(STR_ONBOARD_WIFIASK_BODY), 24, 72, kEinkWidth - 48, FontStyle::Body);

  // Two-option choice, "No" first (default), styled like the language rows
  const StringID optionIDs[] = {STR_ONBOARD_WIFIASK_NO, STR_ONBOARD_WIFIASK_YES};
  int y = 172;
  for (int i = 0; i < 2; i++) {
    drawChipText(26, y, TR(optionIDs[i]), FontStyle::Body, kEinkWidth - 80,
                 i == (int)ob.index, kEinkWidth - 60);
    y += ONB_LANG_PITCH;
  }
}

static void drawLanguagePage() {
  drawOnboardChrome(TR(STR_ONBOARD_LANG_TITLE));

  constexpr int nameX = 34;
  constexpr int nameMaxW = kEinkWidth - nameX - 30;
  int y = ONB_LANG_Y0;
  for (int i = 0; i < I18n::languageCount(); i++) {
    drawChipText(nameX - 8, y, I18n::nativeName(i), FontStyle::Body, nameMaxW,
                 (ulong)i == ob.index, nameMaxW + 16);
    y += ONB_LANG_PITCH;
  }
}

static void drawWifiList() {
  uint16_t count = P_WIFI.getScanResultCount();
  if (count == 0) {
    drawCentered(TR(STR_ONBOARD_WIFI_NONE), 120, FontStyle::Body);
    return;
  }

  int visible = min((int)count, ONB_WIFI_VISIBLE);
  int startIdx = (int)ob.index - (visible / 2);
  startIdx = max(0, min(startIdx, (int)count - visible));

  int ssidMaxW = kEinkWidth - 44 - ONB_RSSI_GUTTER;
  int y = ONB_WIFI_Y0;
  for (int i = startIdx; i < startIdx + visible; i++) {
    WifiApInfo ap;
    if (!P_WIFI.getScanResult(i, ap)) break;

    FontStyle style = FontEngine::fitStyle(DisplayTarget::EINK, ap.ssid, ssidMaxW,
                                           kLabelCascade, kLabelCascadeCount);
    String label = truncateWithEllipsis(ap.ssid, ssidMaxW, style);
    if (ap.authmode != WIFI_AUTH_OPEN) label += " *";

    drawChipText(18, y, label, style, 0, i == (int)ob.index,
                 ssidMaxW + ONB_RSSI_GUTTER, 6, 18, 5);

    String rssi = String(ap.rssi) + "dBm";
    int rw = FontEngine::textWidth(DisplayTarget::EINK, rssi, FontStyle::Caption);
    FontEngine::drawText(DisplayTarget::EINK, kEinkWidth - 14 - rw, y, rssi, FontStyle::Caption);
    y += ONB_WIFI_PITCH;
  }
  u8g2f.setForegroundColor(GxEPD_BLACK);

  drawScrollbar(count, visible, ob.index, kEinkWidth - 8, ONB_WIFI_Y0 - 12,
                visible * ONB_WIFI_PITCH, 3, false, GxEPD_BLACK, GxEPD_WHITE);
}

static void drawWifiPage() {
  drawOnboardChrome(TR(STR_ONBOARD_WIFI_TITLE));

  if (ob.scan == SCAN_QUEUED || ob.scan == SCAN_RUNNING) {
    drawCentered(TR(STR_ONBOARD_WIFI_SCANNING), 120, FontStyle::Body);
  } else {
    drawWifiList();
  }
}

static void drawConnectingPage() {
  drawOnboardChrome(TR(STR_ONBOARD_WIFI_TITLE));

  constexpr int maxW = kEinkWidth - 60;
  FontStyle style = FontEngine::fitStyle(DisplayTarget::EINK, ob.pendingSSID.c_str(),
                                         maxW, kLabelCascade, kLabelCascadeCount);
  String label = truncateWithEllipsis(ob.pendingSSID, maxW, style);

  drawCentered(TR(STR_ONBOARD_WIFI_CONNECTING), 104, FontStyle::Body);
  int w = FontEngine::textWidth(DisplayTarget::EINK, label, style);
  FontEngine::drawText(DisplayTarget::EINK, (kEinkWidth - w) / 2, 136, label, style);
}

static void drawConnectedPage() {
  drawOnboardChrome(TR(STR_ONBOARD_WIFI_TITLE));

  constexpr int maxW = kEinkWidth - 60;
  FontStyle style = FontEngine::fitStyle(DisplayTarget::EINK, ob.joinedSSID.c_str(),
                                         maxW, kLabelCascade, kLabelCascadeCount);
  String label = truncateWithEllipsis(ob.joinedSSID, maxW, style);

  drawCentered(TR(STR_ONBOARD_WIFI_CONNECTED), 96, FontStyle::Body);
  int w = FontEngine::textWidth(DisplayTarget::EINK, label, style);
  FontEngine::drawText(DisplayTarget::EINK, (kEinkWidth - w) / 2, 128, label, style);
  if (!ob.joinedIP.isEmpty())
    drawCentered(ob.joinedIP.c_str(), 152, FontStyle::Caption);
}

static void drawDatetimePage() {
  drawOnboardChrome(TR(STR_ONBOARD_TIME_TITLE));

  DateTime now = CLOCK().nowDT();
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", now.hour(), now.minute());
  char dateBuf[11];
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", now.day(), now.month(), now.year());

  drawCentered(TR(STR_ONBOARD_TIME_CURRENT), 96, FontStyle::Caption);
  drawCentered(timeBuf, 140, FontStyle::Heading1);
  drawCentered(dateBuf, 172, FontStyle::Heading3);
}

static void drawControlsPage() {
  drawOnboardChrome(TR(STR_ONBOARD_CTRL_TITLE));

  const StringID keyIDs[] = {
    STR_ONBOARD_KEY_1, STR_ONBOARD_KEY_2, STR_ONBOARD_KEY_3,
    STR_ONBOARD_KEY_4, STR_ONBOARD_KEY_5, STR_ONBOARD_KEY_6,
  };
  const StringID descIDs[] = {
    STR_ONBOARD_CTRL_DESC_1, STR_ONBOARD_CTRL_DESC_2, STR_ONBOARD_CTRL_DESC_3,
    STR_ONBOARD_CTRL_DESC_4, STR_ONBOARD_CTRL_DESC_5, STR_ONBOARD_CTRL_DESC_6,
  };

  int y = ONB_CTRL_Y0;
  for (int i = 0; i < 6; i++) {
    drawChipText(ONB_KEY_X, y, TR(keyIDs[i]), FontStyle::Caption, 0,
                 true, ONB_KEY_CHIP_MAX_W, 6, 18, 6);

    // Shrink through the label cascade before resorting to an ellipsis
    constexpr int descMaxW = kEinkWidth - ONB_DESC_X - 14;
    FontStyle descStyle = FontEngine::fitStyle(DisplayTarget::EINK, TR(descIDs[i]),
                                               descMaxW, kLabelCascade,
                                               kLabelCascadeCount);
    String desc = truncateWithEllipsis(TR(descIDs[i]), descMaxW, descStyle);
    FontEngine::drawText(DisplayTarget::EINK, ONB_DESC_X, y, desc, descStyle);
    y += ONB_CTRL_PITCH;
  }
}

static void drawSdcardPage() {
  drawOnboardChrome(TR(STR_ONBOARD_SD_TITLE));
  const char* text = PM_SD().getNoSD() ? TR(STR_ONBOARD_SD_MISSING) : TR(STR_ONBOARD_SD_OK);
  drawWrappedBlock(text, 24, 96, kEinkWidth - 48, FontStyle::Body);
}

static void drawDonePage() {
  drawCentered(TR(STR_ONBOARD_DONE_TITLE), 76, FontStyle::Heading1);

  struct SummaryRow { const char* label; String value; };
  DateTime now = CLOCK().nowDT();
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", now.hour(), now.minute());

  const SummaryRow rows[] = {
    {TR(STR_ONBOARD_SUM_LANG), String(I18n::nativeName())},
    {TR(STR_ONBOARD_SUM_WIFI), ob.joinedSSID.isEmpty() ? String(TR(STR_ONBOARD_SKIPPED))
                                                       : ob.joinedSSID + " (" + ob.joinedIP + ")"},
    {TR(STR_ONBOARD_SUM_CLOCK), String(timeBuf)},
  };

  int y = 118;
  for (const SummaryRow& row : rows) {
    FontEngine::drawText(DisplayTarget::EINK, 60, y, row.label, FontStyle::Caption);
    int labelW = FontEngine::textWidth(DisplayTarget::EINK, row.label, FontStyle::Caption);
    FontStyle vStyle = FontEngine::fitStyle(DisplayTarget::EINK, row.value.c_str(), 150,
                                            kLabelCascade, kLabelCascadeCount);
    String value = truncateWithEllipsis(row.value, 150, vStyle);
    FontEngine::drawText(DisplayTarget::EINK, 60 + labelW + 8, y, value, vStyle);
    y += 24;
  }
}

void einkHandler_ONBOARDING() {
  if (!newState) return;
  newState = false;

  beginEinkScreen();

  switch (ob.step) {
    case ONB_WELCOME:         drawWelcomePage(); break;
    case ONB_LANGUAGE:        drawLanguagePage(); break;
    case ONB_WIFI_ASK:        drawWifiAskPage(); break;
    case ONB_WIFI:            drawWifiPage(); break;
    case ONB_WIFI_PASS:       drawWifiPage(); break;
    case ONB_WIFI_CONNECTING: drawConnectingPage(); break;
    case ONB_WIFI_CONNECTED:  drawConnectedPage(); break;
    case ONB_DATETIME:        drawDatetimePage(); break;
    case ONB_CONTROLS:        drawControlsPage(); break;
    case ONB_SDCARD:          drawSdcardPage(); break;
    case ONB_DONE:            drawDonePage(); break;
  }

  const char* hint;
  switch (ob.step) {
    case ONB_WELCOME:  hint = TR(STR_ONBOARD_HINT_START);  break;
    case ONB_LANGUAGE: hint = TR(STR_ONBOARD_HINT_SELECT); break;
    case ONB_WIFI_ASK: hint = TR(STR_ONBOARD_HINT_ASK);    break;
    case ONB_DONE:     hint = TR(STR_ONBOARD_DONE_HINT);   break;
    default:           hint = TR(STR_ONBOARD_HINT_NEXT);   break;
  }
  endEinkScreen(hint);
}

#endif // POCKETMAGE_OS
