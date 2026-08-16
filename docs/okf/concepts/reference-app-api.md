---
type: reference
title: "App API Reference"
description: "Entry points and helper functions for PocketMage apps."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/reference/app-api/"
path: /reference/app-api/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T21:40:21.053Z"
---
---
title: "App API Reference"
description: "Entry points and helper functions for PocketMage apps."
---

# App API Reference

Every PocketMage app, native or OTA, is built from the same entry points and shared helpers. This page documents the exact names and what each one is for. Declarations live in `Code/PocketMageOS/include/globals.h`.

## The two app styles

- **Native OS apps** live in `Code/PocketMageOS/src/OS_APPS/` and are wired into the dispatch switch.
- **OTA apps** use `Code/PocketMageOS/src/APP_TEMPLATE.cpp` and build standalone.

Both call the same shared helpers below.

## OTA app entry points

A third-party app implements three functions:

```cpp
#include <globals.h>
#if OTA_APP
void APP_INIT() {
  // set up state; runs once when the app is entered
}

void processKB_APP() {
  // handle one keypress
}

void einkHandler_APP() {
  // draw the E-Ink screen
}
#endif
```

The `OTA_APP` build path defines `OTA_APP_FLAG=1` in `platformio.ini`, which sets `OTA_APP` and excludes the native OS app set.

## Native OS app entry points

Native apps follow the same shape with app-specific names. Prototypes are declared in `globals.h`.

- `HOME_INIT()` / `processKB_HOME()` / `einkHandler_HOME()`
- `TXT_INIT()` / `processKB_TXT_NEW()` / `einkHandler_TXT_NEW()`
- `FILEWIZ_INIT()` / `processKB_FILEWIZ()` / `einkHandler_FILEWIZ()`
- `SETTINGS_INIT()` / `processKB_SETTINGS()` / `einkHandler_SETTINGS()`
- `TASKS_INIT()` / `processKB_TASKS()` / `einkHandler_TASKS()`
- `CALENDAR_INIT()` / `processKB_CALENDAR()` / `einkHandler_CALENDAR()`
- `JOURNAL_INIT()` / `processKB_JOURNAL()` / `einkHandler_JOURNAL()`
- `LEXICON_INIT()` / `processKB_LEXICON()` / `einkHandler_LEXICON()`
- `APPLOADER_INIT()` / `processKB_APPLOADER()` / `einkHandler_APPLOADER()`
- `TERMINAL_INIT()` / `processKB_TERMINAL()` / `einkHandler_TERMINAL()`

To add a native app, extend the `AppState` enum and add cases to the `switch (CurrentAppState)` in `PocketMageV3.cpp`, at the spots marked `// ADD APP CASES HERE`.

## Dispatch helpers

- `processKB()` - the main keyboard dispatch loop; routes a keypress to the active app
- `applicationEinkHandler()` - the main screen dispatch; routes a refresh to the active app

## App transitions

- `loadState(bool changeState = true, char bootKey = 0)` - enter another app; runs its `*_INIT()`
- `wakeFromNowlater(char bootKey = 0)` - restore the previous app state when waking
- `bootShortcutApp(char inchar)` - maps a boot shortcut key to its app

## Idle and timeout

- `checkTimeout()` - check the idle timer and sleep if the timeout elapsed
- `resetIdle()` - reset the idle clock so the app stays awake
- `disableTimeout` - global flag; while set, the OS never sleeps

## User input prompts

- `textPrompt(promptText = "", prefix = "", mask = false, lockGlyph = false)` - free-text input
- `boolPrompt(promptText = "Are you sure?")` - yes/no, returns `1` or `0`
- `datePrompt(defaultYYYYMMDD = "")` - date input, returns `YYYYMMDD`
- `timePrompt(defaultTime = -1)` - time input, returns an integer like `1430`
- `waitForKeypress(message = "Press any button to continue...")` - block until a key is pressed

## Files and apps

- `saveEditingFile()` - save the current native text editing file
- `fileWizardMini(bool allowRecentSelect = false, String rootDir = "/", char inchar_ = 0)` - file selection dialog, returns the chosen path
- `rebootToAppSlot(int otaIndex)` - reboot straight into an OTA app slot

## Terminal (Wrench)

- `termPrint(const String& line)` - append a line to the terminal output buffer
- `termReturnToPrompt()` - move the terminal back to the prompt
- `compileWrench(const char* wrenchCode)` - compile and run a Wrench script
- `readCFile(const String& path)` - read a source file from the SD card

See the [scripting reference](../scripting/index.md) for the Wrench bindings themselves.

## Shared state

Common state is declared in `globals.h` and `config.h`.

- `CurrentAppState` - the active app; drives the dispatch switch
- `CurrentHOMEState` - the home screen mode
- `prefs` - persistent settings
- `global_fs` - the active filesystem
- `newState` - set to re-run the target app's `*_INIT()`
- `disableTimeout` - global timeout lockout
- `OTA1_APP` through `OTA4_APP` - installed OTA app names

If you are unsure where a function belongs, start with the existing app in `src/OS_APPS/` that already does the same job.

## Next steps

::: grids
::: grid
::: button "PocketMage Library" ./pocketmage-library.md icon:box
:::
::: grid
::: button "System State" ./system-state.md icon:settings
:::
::: grid
::: button "Making Apps" ../guides/making-apps.md icon:code
:::
::: grid
::: button "Native Apps" ../guides/native-apps.md icon:terminal
:::
:::
