#pragma once
// PocketMage keycode constants
// Replaces bare magic numbers scattered throughout the input-handling code.
// All values match the raw bytes returned by KB().updateKeypress().
namespace Keycode {
    constexpr char ENTER      = 13;
    constexpr char BACKSPACE  = 8;
    constexpr char ESCAPE     = 23;  // Cancel / return _RETURN_
    constexpr char SHIFT      = 17;
    constexpr char FUNC       = 18;
    constexpr char LEFT       = 19;
    constexpr char CENTER     = 20;
    constexpr char RIGHT      = 21;
    constexpr char EXIT       = 12;  // App-level exit / left-nav in date/time prompts
    constexpr char HOME_KEY   = 28;  // Jump cursor to start
    constexpr char END_KEY    = 30;  // Jump cursor to end
    constexpr char FN_RIGHT   = 6;   // Right nav in time/date prompts
    constexpr char CLEAR_LINE = 7;
    constexpr char ACCENT1    = 24;
    constexpr char ACCENT2    = 26;
    constexpr char ACCENT3    = 25;
    constexpr char TAB        = 9;
    constexpr char CTRL_N     = 14;
    constexpr char CTRL_F     = 6;
    constexpr char TODAY      = 9;   // Alias used in datePrompt "insert today" shortcut
} // namespace Keycode
