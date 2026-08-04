---
type: reference
title: "PocketMage Library"
description: "Shared hardware, UI, and system modules used by PocketMage."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/reference/pocketmage-library/"
path: /reference/pocketmage-library/
updated: 2026-08-04
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-04T02:13:37.655Z"
---
---
title: "PocketMage Library"
description: "Shared hardware, UI, and system modules used by PocketMage."
---

# PocketMage Library

The PocketMage Library is the shared layer under `Code/PocketMageOS/lib/PocketMage/`. It is the hardware wrapper and utility layer for PocketMageOS. Reach for it when you want to talk to the screen, storage, keyboard, audio, clock, or other shared device systems without rewriting them in an app.

## Access pattern

Each hardware module is a singleton exposed as a function, so methods read `CLASS().method()`:

| Module | Accessor | Use it for |
| --- | --- | --- |
| E-Ink | `EINK()` | E-Ink drawing and refresh |
| OLED | `OLED()` | OLED drawing and status output |
| SD card | `PM_SD()` | SD card and filesystem access |
| Keyboard | `KB()` | Keyboard input and layers |
| Buzzer | `BZ()` | Buzzer and tone feedback |
| Touch | `TOUCH()` | Capacitive touch input |
| Clock | `CLOCK()` | Real-time clock |
| WiFi | `PocketMageWifi::getInstance()` | WiFi helpers and connectivity |

For example:

```cpp
KB().setKeyboardState(NORMAL);
OLED().sysMessage("Saved", 1500);
PM_SD().setWorkingFile("/notes/ideas.txt");
CLOCK().setTimeFromString("14:30");
```

Note the SD accessor is `PM_SD()`, not `SD()` - the `fs::SD` object is part of the Arduino filesystem API and is a different thing.

## The umbrella header

Include `pocketmage.h` to pull in all the major feature modules plus the project config and shared assets. Inside PocketMageOS code, include `globals.h` first, which already pulls in the library.

## Library layout

```txt
lib/PocketMage/
  pocketmage.h           # umbrella header
  pocketmage_oled/       # OLED helpers
  pocketmage_eink/       # E-Ink helpers
  pocketmage_sd/         # SD and filesystem helpers
  pocketmage_kb/         # keyboard helpers
  pocketmage_bz/         # buzzer helpers
  pocketmage_touch/      # touch helpers
  pocketmage_clock/      # clock helpers
  pocketmage_wifi/       # WiFi helpers
  pocketmage_font/       # font and text helpers
  pocketmage_i18n/       # translation tables and generator output
  pocketmage_sys/        # system helpers and shared state
  pocketmage_layout/     # layout helpers
  pocketmage_ui/         # UI helpers
  pocketmage_io/         # file and IO helpers
  assets/                # shared bitmaps and assets
  frames/                # display frame buffers
```

## Module map

### PocketmageOLED

Drives the 256x32 OLED status display and on-screen system messages.

- `oledWord(String word, bool allowLarge = false, bool showInfo = true, String bottomText = "")` - print a word or line, optionally in the large font
- `oledLine(int y)` - move to a specific OLED line
- `sysMessage(String msg, int showTime = 1500)` - show a transient system message
- `oledScroll(...)` - scrollable text output
- `infoBar(...)` - status bar content
- `setPowerSave(bool)` - OLED power management

### PocketmageEink

Drives the 320x240 E-Ink display and its refresh modes.

- `refresh()` - schedule a full refresh
- `setFastFullRefresh(bool)` - enable or disable the faster refresh mode
- `statusBar(...)` - draw the status bar
- `resetDisplay()` - reset the panel and its frame buffers
- `countLines(...)` - measure how many lines fit in a box before you draw

### PocketmageSD

SD card and filesystem access, including file metadata.

- `listDir(fs, dirname)` / `readFile(fs, path)` / `readFileToString(fs, path)` - core FS operations
- `writeFile(fs, path, message)` / `appendFile(fs, path, message)` - write and append
- `renameFile(fs, path1, path2)` / `deleteFile(fs, path)` - file management
- `readBinaryFile(path, buf, len)` / `getFileSize(path)` - binary helpers
- `setMode(PocketmageSD::SDMMC | SDSPI)` - bus mode selection
- `setWorkingFile(path)` / `setEditingFile(path)` - the OS tracks these for save/load
- `beginIO()` / `endIO()` - bracketing I/O when the FS needs explicit sessions

### PocketmageKB

Keyboard input and shift/Fn layers via the TCA8418 keypad controller.

- `setKeyboardState(int)` / `getKeyboardState()` - current layer
- `toggleShift()` / `toggleFn()` / `acceptKey()` - layer toggling
- `updateKeypress()` - poll for the next key
- `checkUSBKB()` - merge USB keyboard input
- `disableInterrupts()` / `enableInterrupts()` - interrupt control

### PocketmageBZ

Buzzer and tone feedback.

- Beeper wrappers for key press confirmation, warnings, and the app boot sound

### PocketmageTOUCH

Capacitive touch input.

- `getScrollVector()` - swipe direction for scrolling lists

### PocketmageCLOCK

Real-time clock wrapper around the PCF8563 RTC.

- `begin()` - start the RTC
- `setTimeFromString("14:30")` - set the clock from a `HH:MM` string
- `isValid()` - is the clock running
- `setToCompileTimeUTC()` - set from compile time (useful before `timeset`)
- `nowDT()` / `getRTC()` - current time as `DateTime` or the RTC object
- `getTimeDiff()` / `getTimeoutMillis()` - idle timeout tracking

### PocketMageWifi

WiFi helpers and connectivity.

- `PocketMageWifi::getInstance()` - the singleton accessor
- Connect, scan, and network status helpers

### System and UI modules

- `pocketmage_sys/` - shared runtime state: boot flags, app boot shortcuts, and OS-wide helpers
- `pocketmage_layout/` - layout and spacing helpers, including the 320x240 layout constants
- `pocketmage_ui/` - UI primitives: icons, glyphs, lists, and dialogs
- `pocketmage_io/` - file and IO helpers
- `pocketmage_i18n/` - translation tables and generated strings
- `pocketmage_font/` - fonts and text rendering

## How to use it

1. Include `globals.h` inside PocketMageOS code.
2. Include `pocketmage.h` when you want the hardware wrapper and helpers directly.
3. Look at the app code in `Code/PocketMageOS/src/OS_APPS/` to see the library in context.
4. Pick the smallest helper that solves the problem.
5. Keep app-specific behavior in the app, not in the library.

## Next steps

::: grids
::: grid
::: button "App API Reference" ./app-api.md icon:book
:::
::: grid
::: button "System State Reference" ./system-state.md icon:settings
:::
::: grid
::: button "Making Apps" ../guides/making-apps.md icon:code
:::
::: grid
::: button "OLED Scripting" ../scripting/oled-c.md icon:monitor
:::
:::
