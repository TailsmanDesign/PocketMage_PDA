#include <UTILS.h>
#include "Keycodes.h"

static constexpr const char* TAG = "UTILS";
static uint8_t prevSec = 0;

void printDebug() {
    DateTime now = CLOCK().nowDT();
    if (now.second() != prevSec) {
        prevSec = now.second();
        float batteryVoltage = getBatteryVoltage();
        ESP_LOGD(
            TAG,
            "PWR_BTN: %d, KB_INT: %d, CHRG: %d, RTC_INT: %d, BAT: %.2f, CPU_FRQ: %.1f, FFU: %d",
            digitalRead(PWR_BTN), digitalRead(KB_IRQ), digitalRead(CHRG_SENS),
            digitalRead(RTC_INT), batteryVoltage,
            (float)getCpuFrequencyMhz(),
            (int)GxEPD2_310_GDEQ031T10::useFastFullUpdate);
        ESP_LOGD(TAG,
            "SYSTEM_CLOCK: %d/%d/%d (%s) %d:%d:%d",
            now.month(), now.day(), now.year(),
            I18n::dayName(now.dayOfTheWeek()),
            now.hour(), now.minute(), now.second());
    }
}

// ---------------------------------------------------------------------------
// Shared idle-grace helper: shows the sleep warning and gives the user 4 s
// to cancel. Returns true if the user cancelled (pressed any key).
// ---------------------------------------------------------------------------
static bool idleGraceWindow() {
    OLED().oledWord(TR(STR_UTILS_SLEEP));
    unsigned long start = millis();
    while ((millis() - start) <= 4000) {
        if (KB().updateKeypress() != 0) {
            OLED().oledWord(TR(STR_GOOD_SAVE));
            delay(500);
            CLOCK().setPrevTimeMillis(millis());
            keypad.flush();
            return true;  // user cancelled sleep
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;  // proceed to sleep
}

#if OTA_APP
void checkTimeout() {
    CLOCK().setTimeoutMillis(millis());
    ESP_LOGV(TAG, "checking timeout");
    if (!disableTimeout) {
        if (CLOCK().getTimeDiff() >= TIMEOUT * 1000) {
            ESP_LOGD(TAG, "Device idle... Deep sleeping");
            if (idleGraceWindow()) return;  // user cancelled
            // user did not cancel; check reboot flag then sleep
            if (!pocketmage::setRebootFlagOTA()) return;
            display.setFullWindow();
            pocketmage::deepSleep();
        }
    } else {
        CLOCK().setPrevTimeMillis(millis());
    }
    // Power Button Event sleep
    if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
        PWR_BTN_event = false;
        ESP_LOGE(TAG, "Power Button Event: Sleeping now");
        pocketmage::deepSleep();
    } else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
        ESP_LOGE(TAG, "Power Button Event: powering off from NOWLATER");
        PWR_BTN_event = false;
        pocketmage::deepSleep();
    }
}
#else // OS APPLICATION
void checkTimeout() {
    CLOCK().setTimeoutMillis(millis());
    ESP_LOGV(TAG, "checking timeout");
    if (!disableTimeout) {
        if (CLOCK().getTimeDiff() >= TIMEOUT * 1000) {
            ESP_LOGD(TAG, "Device idle... Deep sleeping");
            if (idleGraceWindow()) return;  // user cancelled
            saveEditingFile();
            switch (CurrentAppState) {
                case TXT:
                    if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "") {
                        pocketmage::deepSleep(true);
                    } else {
                        pocketmage::deepSleep();
                    }
                    break;
                default:
                    pocketmage::deepSleep();
                    break;
            }
        }
    } else {
        CLOCK().setPrevTimeMillis(millis());
    }
    // Power Button Event sleep
    if (PWR_BTN_event && CurrentHOMEState != NOWLATER) {
        PWR_BTN_event = false;
        ESP_LOGE(TAG, "Power Button Event: Sleeping now");
        saveEditingFile();
        if (digitalRead(CHRG_SENS) == HIGH) {
            // Save last state
            prefs.begin("PocketMage", false);
            prefs.putInt("CurrentAppState", static_cast<int>(CurrentAppState));
            prefs.putString("editingFile", PM_SDAUTO().getEditingFile());
            prefs.end();
            CurrentAppState = HOME;
            CurrentHOMEState = NOWLATER;
            updateTaskArray();
            sortTasksByDueDate(tasks);
            u8g2.clearBuffer();
            OLED().oledWord(" ");
            OLED().setPowerSave(true);
            disableTimeout = true;
            newState = true;
            BZ().playJingle(Jingles::Shutdown);
            display.setFullWindow();
            display.fillScreen(GxEPD_WHITE);
        } else {
            ESP_LOGD(TAG, "Not charging");
            switch (CurrentAppState) {
                case TXT:
                    if (SLEEPMODE == "TEXT" && PM_SDAUTO().getEditingFile() != "") {
                        ESP_LOGE(TAG, "text sleep mode");
                        EINK().setFullRefreshAfter(FULL_REFRESH_AFTER + 1);
                        display.setFullWindow();
                        display.fillRect(0, display.height() - 26, display.width(), 26, GxEPD_WHITE);
                        display.drawRect(0, display.height() - 20, display.width(), 20, GxEPD_BLACK);
                        EINK().statusBar(PM_SDAUTO().getEditingFile(), true);
                        display.fillRect(320 - 86, 240 - 52, 87, 52, GxEPD_WHITE);
                        display.drawBitmap(320 - 86, 240 - 52, sleep1, 87, 52, GxEPD_BLACK);
                        pocketmage::deepSleep(true);
                    } else {
                        pocketmage::deepSleep();
                    }
                    break;
                default:
                    pocketmage::deepSleep();
                    break;
            }
        }
    } else if (PWR_BTN_event && CurrentHOMEState == NOWLATER) {
        ESP_LOGE(TAG, "Power Button Event: powering off from NOWLATER");
        PWR_BTN_event = false;
        pocketmage::deepSleep();
    }
}

// Exit the NOWLATER (charging shutdown) screen back to home.
void wakeFromNowlater(char bootKey) {
    loadState(true, bootKey);
    keypad.flush();
    disableTimeout = false;
    CurrentHOMEState = HOME_HOME;
    OLED().setPowerSave(false);
    BZ().playJingle(Jingles::Startup);
    newState = true;
    if (deviceLocked) return;
    display.fillScreen(GxEPD_WHITE);
    EINK().refresh();
    delay(200);
}

// Boot shortcut letters map to an app state.
// TODO: This should be i18n-ed?
AppState bootShortcutApp(char inchar) {
    switch (inchar) {
        case 'h': return HOME;
        case 'u': return USB_APP;
        case 'f': return FILEWIZ;
        case 't': return TASKS;
        case 'n': return TXT;
        case 's': return SETTINGS;
        case 'c': return CALENDAR;
        case 'j': return JOURNAL;
        case 'd': return LEXICON;
        case 'x': return TERMINAL;
        case 'l': return APPLOADER;
        default:  return CurrentAppState;
    }
}
#endif

void loadState(bool changeState, char bootKey) {
    prefs.begin("PocketMage", true);  // Read-Only
    // Misc
    TIMEOUT            = prefs.getInt("TIMEOUT", 120);
    DEBUG_VERBOSE      = prefs.getBool("DEBUG_VERBOSE", true);
    SYSTEM_CLOCK       = prefs.getBool("SYSTEM_CLOCK", true);
    SHOW_YEAR          = prefs.getBool("SHOW_YEAR", true);
    SAVE_POWER         = prefs.getBool("SAVE_POWER", true);
    ALLOW_NO_MICROSD   = prefs.getBool("ALLOW_NO_SD", true);
    PM_SDAUTO().setEditingFile(prefs.getString("editingFile", ""));
    HOME_ON_BOOT       = prefs.getBool("HOME_ON_BOOT", false);
    FAST_REFRESH       = prefs.getBool("FAST_REFRESH", POCKETMAGE_HW_VERSION == 1);
    OLED_BRIGHTNESS    = prefs.getInt("OLED_BRIGHTNESS", 255);
    OLED_MAX_FPS       = prefs.getInt("OLED_MAX_FPS", 60);
    MUTE_BUZZER        = prefs.getBool("MUTE_BUZZER", false);
    I18n::setLanguage(static_cast<Lang>(prefs.getInt("Language", static_cast<int>(Lang::English))));
    OTA1_APP = prefs.getString("OTA1", "-");
    OTA2_APP = prefs.getString("OTA2", "-");
    OTA3_APP = prefs.getString("OTA3", "-");
    OTA4_APP = prefs.getString("OTA4", "-");
#if !OTA_APP
    deviceLocked = prefs.getBool("LOCK_ENABLED", false);
#endif
    if (!changeState) {
        prefs.end();
        return;
    }
    u8g2.setContrast(OLED_BRIGHTNESS);
#if !OTA_APP
    if (HOME_ON_BOOT) {
        CurrentAppState = HOME;
        HOME_INIT();
    } else {
        CurrentAppState = static_cast<AppState>(prefs.getInt("CurrentAppState", HOME));
        char inchar = bootKey;
        if (inchar == 0) {
            KB().setKeyboardState(NORMAL);
            inchar = KB().updateKeypress();
        }
        CurrentAppState = bootShortcutApp(inchar);
        keypad.flush();
        switch (CurrentAppState) {
            case HOME:      HOME_INIT();      break;
            case TXT:       TXT_INIT();       break;
            case FILEWIZ:   FILEWIZ_INIT();   break;
            case SETTINGS:  SETTINGS_INIT();  break;
            case TASKS:     TASKS_INIT();     break;
            case USB_APP:   HOME_INIT();      break;
            case COMM:      COMM_INIT();      break;
            case CALENDAR:  CALENDAR_INIT();  break;
            case LEXICON:   LEXICON_INIT();   break;
            case JOURNAL:   JOURNAL_INIT();   break;
            case TERMINAL:  TERMINAL_INIT();  break;
            case APPLOADER: APPLOADER_INIT(); break;
            default:        HOME_INIT();      break;
        }
    }
#endif
    prefs.end();
}

void updateBattState() {
    float rawVoltage = getBatteryVoltage();
    static float filteredVoltage = rawVoltage;
    static float prevVoltage     = 0.0f;
    static int   prevBattState   = -1;
    constexpr float alpha     = 0.1f;
    constexpr float threshold = 0.05f;
    filteredVoltage = alpha * rawVoltage + (1.0f - alpha) * filteredVoltage;
    int newBattState = battState;
    MP2722::MP2722_ChargeStatus chg;
    if (PowerSystem.getChargeStatus(chg) &&
        (chg.code >= 0b001 && chg.code <= 0b101)) {
        newBattState = 5;
    } else {
        bool low;
        if (PowerSystem.isBatteryLow(low) && low) {
            OLED().sysMessage(TR(STR_UTILS_BATT_CRITICAL), 1000);
#if !OTA_APP
            saveEditingFile();
#endif
            pocketmage::deepSleep(false);
        }
        // Voltage thresholds with hysteresis
        if      (filteredVoltage > 4.1f || (prevBattState == 4 && filteredVoltage > 4.1f - threshold)) newBattState = 4;
        else if (filteredVoltage > 3.9f || (prevBattState == 3 && filteredVoltage > 3.9f - threshold)) newBattState = 3;
        else if (filteredVoltage > 3.8f || (prevBattState == 2 && filteredVoltage > 3.8f - threshold)) newBattState = 2;
        else if (filteredVoltage > 3.7f || (prevBattState == 1 && filteredVoltage > 3.7f - threshold)) newBattState = 1;
        else newBattState = 0;
    }
    if (newBattState != battState) {
        battState    = newBattState;
        prevBattState = newBattState;
    }
    prevVoltage = filteredVoltage;
}

#pragma region Basic Inputs

#if !OTA_APP
String textPrompt(String promptText, String prefix, bool mask, bool lockGlyph) {
    String currentLine  = "";
    int    cursor_pos   = 0;
    long   lastInput    = CLOCK().getPrevTimeMillis();
    bool   redraw       = true;

    for (;;) {
        if (!noTimeout) checkTimeout();
        if (DEBUG_VERBOSE) printDebug();
        if (CurrentHOMEState == NOWLATER) return "_RETURN_";

        updateBattState();
        KB().checkUSBKB();

        static int lastBattState = -1;
        if (battState != lastBattState) {
            redraw = true;
            lastBattState = battState;
        }

        int currentMillis = millis();
        unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
        if (currentSystemTime > (unsigned long)lastInput) {
            lastInput = currentSystemTime;
            redraw = true;
        }

        char inchar = KB().updateKeypress();
        if (currentMillis - KBBounceMillis >= KB_COOLDOWN && inchar != 0) {
            lastInput       = millis();
            KBBounceMillis  = currentMillis;
            redraw          = true;

            if (inchar == Keycode::ESCAPE) {
                currentLine = "_RETURN_";
                break;
            } else if (inchar == Keycode::ENTER) {
                cursor_pos = 0;
                break;
            } else if (inchar == Keycode::SHIFT) {
                if (KB().getKeyboardState() == SHIFT || KB().getKeyboardState() == FN_SHIFT)
                    KB().setKeyboardState(NORMAL);
                else if (KB().getKeyboardState() == FUNC)
                    KB().setKeyboardState(FN_SHIFT);
                else
                    KB().setKeyboardState(SHIFT);
            } else if (inchar == Keycode::FUNC) {
                if (KB().getKeyboardState() == FUNC || KB().getKeyboardState() == FN_SHIFT)
                    KB().setKeyboardState(NORMAL);
                else if (KB().getKeyboardState() == SHIFT)
                    KB().setKeyboardState(FN_SHIFT);
                else
                    KB().setKeyboardState(FUNC);
            } else if (inchar == Keycode::BACKSPACE) {
                if (cursor_pos > 0) {
                    int old_cursor = cursor_pos;
                    do { cursor_pos--; }
                    while (cursor_pos > 0 && (currentLine[cursor_pos] & 0xC0) == 0x80);
                    currentLine.remove(cursor_pos, old_cursor - cursor_pos);
                }
            } else if (inchar == Keycode::LEFT) {
                if (cursor_pos > 0) {
                    do { cursor_pos--; }
                    while (cursor_pos > 0 && (currentLine[cursor_pos] & 0xC0) == 0x80);
                }
            } else if (inchar == Keycode::RIGHT) {
                if (cursor_pos < (int)currentLine.length()) {
                    do { cursor_pos++; }
                    while (cursor_pos < (int)currentLine.length() &&
                           (currentLine[cursor_pos] & 0xC0) == 0x80);
                }
            } else if (inchar == Keycode::CENTER) {
                currentLine = "_CENTER_";
                break;
            } else if (inchar == Keycode::HOME_KEY) {
                cursor_pos = 0;
                KB().setKeyboardState(NORMAL);
            } else if (inchar == Keycode::END_KEY) {
                cursor_pos = (int)currentLine.length();
                KB().setKeyboardState(NORMAL);
            } else if (inchar == 29) {  // FN+something: reset state
                KB().setKeyboardState(NORMAL);
            } else if (inchar == Keycode::EXIT) {
                currentLine = "_EXIT_";
                break;
            } else if (inchar == Keycode::CTRL_F) {
                KB().setKeyboardState(NORMAL);
            } else if (inchar == Keycode::CLEAR_LINE) {
                currentLine = "";
                cursor_pos  = 0;
                KB().setKeyboardState(NORMAL);
            } else if (inchar == Keycode::ACCENT1 || inchar == Keycode::ACCENT2 ||
                       inchar == Keycode::ACCENT3 || inchar == Keycode::TAB    ||
                       inchar == Keycode::CTRL_N) {
                KB().setKeyboardState(NORMAL);
            } else {
                // Printable character — insert at cursor
                String chStr = String(inchar);
                if (cursor_pos == 0) {
                    currentLine = chStr + currentLine;
                } else if (cursor_pos == (int)currentLine.length()) {
                    currentLine += chStr;
                } else {
                    currentLine = currentLine.substring(0, cursor_pos) + chStr +
                                  currentLine.substring(cursor_pos);
                }
                // Clamp cursor before advancing (belt-and-suspenders)
                cursor_pos = min(cursor_pos + 1, (int)currentLine.length());
                if (KB().getKeyboardState() != NORMAL && !(inchar >= 48 && inchar <= 57))
                    KB().setKeyboardState(NORMAL);
            }
        }

        // Idle animation handling
        bool isIdle = (millis() - (unsigned long)lastInput > IDLE_TIME);
        static bool wasIdle = false;
        if (isIdle != wasIdle) {
            wasIdle = isIdle;
            if (!isIdle) { resetIdle(); redraw = true; }
        }

        if (currentMillis - OLEDFPSMillis >= (1000 / OLED_MAX_FPS)) {
            if (isIdle) {
                OLEDFPSMillis = currentMillis;
                if (!lockGlyph) mageIdle(true);
            } else if (redraw) {
                OLEDFPSMillis = currentMillis;
                redraw = false;
                String displayLine = currentLine;
                if (mask) {
                    for (int i = 0; i < (int)displayLine.length(); i++)
                        displayLine[i] = '*';
                }
                if (lockGlyph) {
                    OLED().oledLine(prefix + displayLine,
                                    cursor_pos + prefix.length(), false, promptText, true);
                    u8g2.drawXBMP(u8g2.getDisplayWidth() - kOledLockGlyphX,
                                  kOledLockGlyphY, kOledLockGlyphW, kOledLockGlyphH, _lockIcon);
                    u8g2.sendBuffer();
                } else if (prefix != "") {
                    OLED().oledLine(prefix + displayLine,
                                    cursor_pos + prefix.length(), false, promptText);
                } else {
                    OLED().oledLine(displayLine, cursor_pos, false, promptText);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        yield();
    }
    return currentLine;
}

// ---------------------------------------------------------------------------
// fitPromptText — pick the largest font that fits within maxWidth pixels.
// Returns chosen FontStyle, x_offset (centred) and y_offset for drawing.
// ---------------------------------------------------------------------------
struct PromptLayout {
    FontStyle style;
    int x_offset;
    int y_offset;
};

static PromptLayout fitPromptText(const String& msg, uint16_t maxWidth, int baseY) {
    struct Candidate { FontStyle style; int yExtra; };
    static constexpr Candidate opts[] = {
        {FontStyle::OledWord,  3},
        {FontStyle::Heading3,  2},
        {FontStyle::BodyBold,  1},
        {FontStyle::Caption,   0},
    };
    for (const auto& c : opts) {
        int w = FontEngine::textWidth(DisplayTarget::OLED, msg, c.style);
        if (w < (int)maxWidth - 8) {
            return {c.style, (maxWidth - w) / 2, baseY + c.yExtra};
        }
    }
    // Falls back to Caption even if it overflows; right-align
    int w = FontEngine::textWidth(DisplayTarget::OLED, msg, FontStyle::Caption);
    return {FontStyle::Caption, (int)maxWidth - w, baseY};
}

int boolPrompt(String promptText) {
    KB().setKeyboardState(NORMAL);
    pocketmage::setCpuSpeed(240);
    String msg = promptText + TR(STR_UTILS_YN_SUFFIX);
    u8g2.clearBuffer();
    const uint16_t dw = u8g2.getDisplayWidth();
    const uint16_t dh = u8g2.getDisplayHeight();

    PromptLayout layout = fitPromptText(msg, dw, 16 + 5);

    for (int y = dh; y > 0; y -= 2) {
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, y + layout.y_offset,
                             msg, layout.style);
        u8g2.drawRFrame(0, y, dw, dh + 16, 10);
        u8g2.sendBuffer();
        delay(5);
    }
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    unsigned long lastSystemTime = CLOCK().getPrevTimeMillis();
    int retVal = -1;
    for (;;) {
        if (!noTimeout) checkTimeout();
        if (DEBUG_VERBOSE) printDebug();
        CLOCK().setPrevTimeMillis(millis());
        updateBattState();
        unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
        if (currentSystemTime > lastSystemTime) {
            lastSystemTime = currentSystemTime;
            u8g2.clearBuffer();
            FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, layout.y_offset,
                                 msg, layout.style);
            u8g2.drawRFrame(0, 0, dw, dh + 16, 10);
            u8g2.sendBuffer();
        }
        int currentMillis = millis();
        char inchar = KB().updateKeypress();
        if (currentMillis - KBBounceMillis >= KB_COOLDOWN && inchar != 0) {
            KBBounceMillis = currentMillis;
            if (inchar == 'y' || inchar == 'Y')      { retVal = 1; break; }
            else if (inchar == 'n' || inchar == 'N') { retVal = 0; break; }
            else if (inchar == Keycode::ESCAPE)       { retVal = 0; break; }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        yield();
    }
    pocketmage::setCpuSpeed(240);
    for (int y = 0; y <= dh; y += 2) {
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, y + layout.y_offset,
                             msg, layout.style);
        u8g2.drawRFrame(0, y, dw, dh + 16, 10);
        u8g2.sendBuffer();
        delay(5);
    }
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
    return retVal;
}

int timePrompt(int defaultTime) {
    uint8_t digits[4] = {0, 0, 0, 0};
    ulong   currentIndex = 0;
    if (defaultTime >= 0 && defaultTime <= 2359) {
        digits[0] = (defaultTime / 1000) % 10;
        digits[1] = (defaultTime / 100)  % 10;
        digits[2] = (defaultTime / 10)   % 10;
        digits[3] =  defaultTime         % 10;
    }
    // Static so the array is not re-constructed on every loop iteration
    static constexpr int tX[4] = {93, 110, 131, 148};

    for (;;) {
        if (!noTimeout) checkTimeout();
        if (DEBUG_VERBOSE) printDebug();

        int scrollVec = TOUCH().getScrollVector();
        if (scrollVec != 0) {
            if (currentIndex == 0) {
                int d0 = digits[0] + scrollVec;
                if (d0 > 2) d0 = 0;
                if (d0 < 0) d0 = 2;
                digits[0] = d0;
                if (digits[0] == 2 && digits[1] > 3) digits[1] = 3;
            } else {
                int total_mins = (digits[0] * 10 + digits[1]) * 60 +
                                  (digits[2] * 10 + digits[3]);
                switch (currentIndex) {
                    case 1: total_mins += scrollVec * 60; break;
                    case 2: total_mins += scrollVec * 10; break;
                    case 3: total_mins += scrollVec *  1; break;
                }
                total_mins = total_mins % 1440;
                if (total_mins < 0) total_mins += 1440;
                int h = total_mins / 60;
                int m = total_mins % 60;
                digits[0] = h / 10; digits[1] = h % 10;
                digits[2] = m / 10; digits[3] = m % 10;
            }
        }

        updateBattState();
        KB().checkUSBKB();
        KB().setKeyboardState(FUNC);
        int  currentMillis = millis();
        char inchar        = KB().updateKeypress();
        if (currentMillis - KBBounceMillis >= KB_COOLDOWN && inchar != 0) {
            KBBounceMillis = currentMillis;
            if (inchar == Keycode::EXIT || inchar == Keycode::BACKSPACE) {
                if (currentIndex > 0) currentIndex--;
            } else if (inchar == Keycode::CTRL_F) {
                if (currentIndex < 3) currentIndex++;
            } else if (inchar >= '0' && inchar <= '9') {
                int val = inchar - '0';
                switch (currentIndex) {
                    case 0:
                        digits[0] = (val > 2) ? 2 : val;
                        if (digits[0] == 2 && digits[1] > 3) digits[1] = 3;
                        break;
                    case 1:
                        digits[1] = (digits[0] == 2) ? min(val, 3) : val;
                        break;
                    case 2:
                        digits[2] = (val > 5) ? 5 : val;
                        break;
                    case 3:
                        digits[3] = val;
                        break;
                }
                if (currentIndex < 3) currentIndex++;
            } else if (inchar == Keycode::ENTER) {
                int returnInt = digits[3]     + digits[2] * 10 +
                                digits[1] * 100 + digits[0] * 1000;
                if (returnInt >= 2400) returnInt = 0;
                return returnInt;
            }
        }

        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, 256, 32, timeInput);
        switch (currentIndex) {
            case 0: u8g2.drawXBMP(89,  21, 24, 11, leftRightIndicator0); break;
            case 1: u8g2.drawXBMP(106, 21, 24, 11, leftRightIndicator1); break;
            case 2: u8g2.drawXBMP(127, 21, 24, 11, leftRightIndicator1); break;
            case 3: u8g2.drawXBMP(144, 21, 24, 11, leftRightIndicator2); break;
        }
        for (int i = 0; i < 4; i++) {
            if (i == (int)currentIndex) {
                u8g2.setDrawColor(1);
                u8g2.drawBox(tX[i], 0, 15, 20);
                u8g2.setDrawColor(0);
            } else {
                u8g2.setDrawColor(1);
            }
            FontEngine::drawGlyph(DisplayTarget::OLED, tX[i], 16,
                                  '0' + digits[i], FontStyle::ClockDigit);
        }
        u8g2.setDrawColor(1);
        u8g2.sendBuffer();
        vTaskDelay(pdMS_TO_TICKS(10));
        yield();
    }
}
#endif  // !OTA_APP

static int getDaysInMonth(int month, int year) {
    if (month == 2)
        return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

#if !OTA_APP
String datePrompt(String defaultYYYYMMDD) {
    uint8_t digits[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ulong   currentIndex = 0;

    // Parse default date — use separate names to avoid shadowing in the loop below
    int initD, initM, initY;
    if (defaultYYYYMMDD.length() == 8) {
        initY = defaultYYYYMMDD.substring(0, 4).toInt();
        initM = defaultYYYYMMDD.substring(4, 6).toInt();
        initD = defaultYYYYMMDD.substring(6, 8).toInt();
    } else {
        DateTime now = CLOCK().nowDT();
        initY = now.year(); initM = now.month(); initD = now.day();
    }
    digits[0] = initD / 10;       digits[1] = initD % 10;
    digits[2] = initM / 10;       digits[3] = initM % 10;
    digits[4] = initY / 1000;     digits[5] = (initY / 100) % 10;
    digits[6] = (initY / 10) % 10; digits[7] = initY % 10;

    static constexpr int dX[8] = {57, 74, 96, 113, 135, 152, 168, 185};

    for (;;) {
        if (!noTimeout) checkTimeout();
        if (DEBUG_VERBOSE) printDebug();

        int scrollVec = TOUCH().getScrollVector();
        if (scrollVec != 0) {
            // Use distinct loop-local names (curD/curM/curY) to avoid shadowing initD/M/Y
            int curD = digits[0] * 10 + digits[1];
            int curM = digits[2] * 10 + digits[3];
            int curY = digits[4] * 1000 + digits[5] * 100 + digits[6] * 10 + digits[7];
            if (curD == 0) curD = 1;
            if (curM == 0) curM = 1;

            if (currentIndex == 0) {
                int d_tens = digits[0] + scrollVec;
                if (d_tens > 3) d_tens = 0;
                if (d_tens < 0) d_tens = 3;
                curD = d_tens * 10 + digits[1];
                int maxDays = getDaysInMonth(curM, curY);
                if (curD > maxDays) curD = maxDays;
                if (curD == 0)      curD = 1;
            } else if (currentIndex == 1) {
                curD += scrollVec;
                while (curD > getDaysInMonth(curM, curY)) {
                    curD -= getDaysInMonth(curM, curY); curM++;
                    if (curM > 12) { curM = 1; curY++; }
                }
                while (curD < 1) {
                    curM--;
                    if (curM < 1) { curM = 12; curY--; }
                    curD += getDaysInMonth(curM, curY);
                }
            } else if (currentIndex == 2) {
                int m_tens = digits[2] + scrollVec;
                if (m_tens > 1) m_tens = 0;
                if (m_tens < 0) m_tens = 1;
                curM = m_tens * 10 + digits[3];
                if (curM > 12) curM = 12;
                if (curM == 0) curM = 1;
                int maxDays = getDaysInMonth(curM, curY);
                if (curD > maxDays) curD = maxDays;
            } else if (currentIndex == 3) {
                curM += scrollVec;
                while (curM > 12) { curM -= 12; curY++; }
                while (curM < 1)  { curM += 12; curY--; }
                int maxDays = getDaysInMonth(curM, curY);
                if (curD > maxDays) curD = maxDays;
            } else {
                if (currentIndex == 4) curY += scrollVec * 1000;
                if (currentIndex == 5) curY += scrollVec * 100;
                if (currentIndex == 6) curY += scrollVec * 10;
                if (currentIndex == 7) curY += scrollVec *  1;
                curY = constrain(curY, 2000, 2199);
                int maxDays = getDaysInMonth(curM, curY);
                if (curD > maxDays) curD = maxDays;
            }
            digits[0] = curD / 10;       digits[1] = curD % 10;
            digits[2] = curM / 10;       digits[3] = curM % 10;
            digits[4] = curY / 1000;     digits[5] = (curY / 100) % 10;
            digits[6] = (curY / 10) % 10; digits[7] = curY % 10;
        }

        updateBattState();
        KB().checkUSBKB();
        KB().setKeyboardState(FUNC);
        int  currentMillis = millis();
        char inchar        = KB().updateKeypress();
        if (currentMillis - KBBounceMillis >= KB_COOLDOWN && inchar != 0) {
            KBBounceMillis = currentMillis;
            if (inchar == Keycode::TODAY || inchar == Keycode::CTRL_N) {
                DateTime now = CLOCK().nowDT();
                char dateBuf[11];
                snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d",
                         now.day(), now.month(), now.year());
                return String(dateBuf);
            } else if (inchar == Keycode::EXIT || inchar == Keycode::BACKSPACE) {
                if (currentIndex > 0) currentIndex--;
            } else if (inchar == Keycode::CTRL_F) {
                if (currentIndex < 7) currentIndex++;
            } else if (inchar >= '0' && inchar <= '9') {
                int val = inchar - '0';
                digits[currentIndex] = val;
                int curD = digits[0] * 10 + digits[1];
                int curM = digits[2] * 10 + digits[3];
                int curY = digits[4] * 1000 + digits[5] * 100 + digits[6] * 10 + digits[7];
                if (curM > 12) curM = 12;
                if (currentIndex > 1 && curM == 0) curM = 1;
                int maxDays = getDaysInMonth(curM == 0 ? 1 : curM, curY);
                if (curD > maxDays) curD = maxDays;
                if (currentIndex <= 1 && curD == 0 && currentIndex == 1) curD = 1;
                digits[0] = curD / 10;       digits[1] = curD % 10;
                digits[2] = curM / 10;       digits[3] = curM % 10;
                digits[4] = curY / 1000;     digits[5] = (curY / 100) % 10;
                digits[6] = (curY / 10) % 10; digits[7] = curY % 10;
                if (currentIndex < 7) currentIndex++;
            } else if (inchar == Keycode::ENTER) {
                char dateBuf[11];
                snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d",
                         digits[0] * 10 + digits[1],
                         digits[2] * 10 + digits[3],
                         digits[4] * 1000 + digits[5] * 100 + digits[6] * 10 + digits[7]);
                return String(dateBuf);
            }
        }

        u8g2.clearBuffer();
        u8g2.drawXBMP(0, 0, 256, 32, dateInput);
        const uint8_t* ind = leftRightIndicator1;
        if (currentIndex == 0) ind = leftRightIndicator0;
        else if (currentIndex == 7) ind = leftRightIndicator2;
        u8g2.drawXBMP(dX[currentIndex] - 4, 21, 24, 11, ind);
        for (int i = 0; i < 8; i++) {
            if (i == (int)currentIndex) {
                u8g2.setDrawColor(1);
                u8g2.drawBox(dX[i], 0, 15, 20);
                u8g2.setDrawColor(0);
            } else {
                u8g2.setDrawColor(1);
            }
            FontEngine::drawGlyph(DisplayTarget::OLED, dX[i], 16,
                                  '0' + digits[i], FontStyle::ClockDigit);
        }
        u8g2.setDrawColor(1);
        u8g2.sendBuffer();
        vTaskDelay(pdMS_TO_TICKS(10));
        yield();
    }
}
#endif  // !OTA_APP

void waitForKeypress(String message) {
    KB().setKeyboardState(NORMAL);
    pocketmage::setCpuSpeed(240);
    const String& msg       = message;
    String        bottomMsg = TR(STR_UTILS_PRESS_KEY);
    u8g2.clearBuffer();
    const uint16_t dw = u8g2.getDisplayWidth();
    const uint16_t dh = u8g2.getDisplayHeight();

    // Re-use the shared font-fit helper (keeps an ~8 px horizontal margin from the frame)
    PromptLayout layout = fitPromptText(msg, dw, 16);

    for (int y = dh; y > 0; y -= 2) {
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, y + layout.y_offset,
                             msg, layout.style);
        FontEngine::drawText(DisplayTarget::OLED,
                             (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2,
                             y + dh - 2, bottomMsg, FontStyle::Tiny);
        u8g2.drawRFrame(0, y, dw, dh + 16, 10);
        u8g2.sendBuffer();
        delay(5);
    }
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);

    unsigned long lastSystemTime = CLOCK().getPrevTimeMillis();
    for (;;) {
        if (!noTimeout) checkTimeout();
        if (DEBUG_VERBOSE) printDebug();
        updateBattState();
        unsigned long currentSystemTime = CLOCK().getPrevTimeMillis();
        if (currentSystemTime > lastSystemTime) {
            lastSystemTime = currentSystemTime;
            u8g2.clearBuffer();
            FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, layout.y_offset,
                                 msg, layout.style);
            FontEngine::drawText(DisplayTarget::OLED,
                                 (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2,
                                 dh - 2, bottomMsg, FontStyle::Tiny);
            u8g2.drawRFrame(0, 0, dw, dh + 16, 10);
            u8g2.sendBuffer();
        }
        int currentMillis = millis();
        char inchar = KB().updateKeypress();
        if (currentMillis - KBBounceMillis >= KB_COOLDOWN && inchar != 0) {
            KBBounceMillis = currentMillis;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        yield();
    }
    pocketmage::setCpuSpeed(240);
    for (int y = 0; y <= dh; y += 2) {
        u8g2.clearBuffer();
        FontEngine::drawText(DisplayTarget::OLED, layout.x_offset, y + layout.y_offset,
                             msg, layout.style);
        FontEngine::drawText(DisplayTarget::OLED,
                             (dw - FontEngine::textWidth(DisplayTarget::OLED, bottomMsg, FontStyle::Tiny)) / 2,
                             y + dh - 2, bottomMsg, FontStyle::Tiny);
        u8g2.drawRFrame(0, y, dw, dh + 16, 10);
        u8g2.sendBuffer();
        delay(5);
    }
    if (SAVE_POWER) pocketmage::setCpuSpeed(POWER_SAVE_FREQ);
}

void checkCrashState() {
    esp_reset_reason_t reset_reason = esp_reset_reason();
    if (reset_reason == ESP_RST_PANIC  || reset_reason == ESP_RST_WDT ||
        reset_reason == ESP_RST_TASK_WDT || reset_reason == ESP_RST_INT_WDT) {
        String crashMsg = TR(STR_UTILS_CRASH_PREFIX);
        switch (reset_reason) {
            case ESP_RST_PANIC:    crashMsg += TR(STR_UTILS_CRASH_PANIC);    break;
            case ESP_RST_WDT:      crashMsg += TR(STR_UTILS_CRASH_WDT);      break;
            case ESP_RST_TASK_WDT: crashMsg += TR(STR_UTILS_CRASH_TASK_WDT); break;
            case ESP_RST_INT_WDT:  crashMsg += TR(STR_UTILS_CRASH_INT_WDT);  break;
            default:               crashMsg += TR(STR_UTILS_CRASH_UNKNOWN);  break;
        }
        int romReason = (int)esp_rom_get_reset_reason(0);
        crashMsg += TR(STR_UTILS_CRASH_CODE_OPEN) + String(romReason) + ")";
        prefs.begin("PocketMage", false);
        prefs.putInt("CurrentAppState", HOME);
        prefs.end();
        PM_SDAUTO().setEditingFile("");
        waitForKeypress(crashMsg);
    }
}

void checkRTCPowerLoss() {
    bool in = false;
    if (SET_CLOCK_ON_UPLOAD || CLOCK().getRTC().lostPower()) {
        CLOCK().setToCompileTimeUTC();
        in = true;
    }
    if (in) {
#if !OTA_APP
        bool previousTimeoutState = noTimeout;
        noTimeout = true;
        DateTime now = CLOCK().nowDT();
        char defaultDate[9];
        snprintf(defaultDate, sizeof(defaultDate), "%04d%02d%02d",
                 now.year(), now.month(), now.day());
        int defaultTime = (now.hour() * 100) + now.minute();
        bool setTime = boolPrompt(TR(STR_UTILS_POWER_LOST));
        if (!setTime) {
            noTimeout = previousTimeoutState;
            return;
        }
        String newDateStr = datePrompt(String(defaultDate));
        int    newTimeInt = timePrompt(defaultTime);
        int d   = newDateStr.substring(0, 2).toInt();
        int m   = newDateStr.substring(3, 5).toInt();
        int y   = newDateStr.substring(6, 10).toInt();
        int h   = newTimeInt / 100;
        int min = newTimeInt % 100;
        CLOCK().getRTC().adjust(DateTime(y, m, d, h, min, 0));
        OLED().sysMessage(TR(STR_UTILS_TIME_SET), 500);
        noTimeout = previousTimeoutState;
#endif
    }
}

#if !OTA_APP
void saveEditingFile() {
    OLED().oledWord(TR(STR_UTILS_SAVING_WORK));
    String savePath = PM_SDAUTO().getEditingFile();
    if (savePath != "" && savePath != "-" && savePath != "/temp.txt" && fileLoaded) {
        if (!savePath.startsWith("/")) savePath = "/" + savePath;
        ESP_LOGI(TAG, "Saving MarkdownFile");
        saveMarkdownFile(savePath);
        ESP_LOGI(TAG, "Done saving MarkdownFile");
    }
}
#endif
