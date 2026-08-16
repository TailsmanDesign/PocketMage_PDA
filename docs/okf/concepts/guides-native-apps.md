---
type: guide
title: "Native Apps"
description: "How PocketMageOS native apps are structured."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/guides/native-apps/"
path: /guides/native-apps/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T22:45:57.802Z"
---
---
title: "Native Apps"
description: "How PocketMageOS native apps are structured."
---

# Native Apps

Native apps live inside PocketMageOS and are compiled into the firmware. Each one is a set of three functions wired into the OS dispatch loop. This page explains the pattern and the exact wiring you need when adding one.

## The pattern

Every native app follows this shape:

- `*_INIT()` - set up app state and enter the app
- `processKB_*()` - handle keyboard input
- `einkHandler_*()` - draw the screen

The names are per-app, not generic. Examples from the codebase:

| App | Init | Keyboard | Draw |
| --- | --- | --- | --- |
| Home | `HOME_INIT()` | `processKB_HOME()` | `einkHandler_HOME()` |
| TXT | `TXT_INIT()` | `processKB_TXT_NEW()` | `einkHandler_TXT_NEW()` |
| FileWiz | `FILEWIZ_INIT()` | `processKB_FILEWIZ()` | `einkHandler_FILEWIZ()` |
| Calendar | `CALENDAR_INIT()` | `processKB_CALENDAR()` | `einkHandler_CALENDAR()` |
| Tasks | `TASKS_INIT()` | `processKB_TASKS()` | `einkHandler_TASKS()` |
| Journal | `JOURNAL_INIT()` | `processKB_JOURNAL()` | `einkHandler_JOURNAL()` |
| Settings | `SETTINGS_INIT()` | `processKB_SETTINGS()` | `einkHandler_SETTINGS()` |
| Loader | `APPLOADER_INIT()` | `processKB_APPLOADER()` | `einkHandler_APPLOADER()` |
| Terminal | `TERMINAL_INIT()` | `processKB_TERMINAL()` | `einkHandler_TERMINAL()` |

## How a native app is wired in

There are three places an app touches the OS, all marked or grouped for easy editing:

1. **The `AppState` enum** in `Code/PocketMageOS/include/globals.h`. Add your app name here.
2. **The prototypes** in the same header, grouped next to the other apps.
3. **The dispatch switch** in `PocketMageV3.cpp`. Add a case to both `processKB()` and `applicationEinkHandler()`; the spots are marked `// ADD APP CASES HERE`.

Optionally, add a home-screen command so the app can be launched by typing its name or shortcut. See the [home commands](../command-manual/index.md) section of the command manual.

## What a native app owns

- app state (kept local to the app file where possible)
- screen draw logic
- key handling
- save and exit behavior
- transitions back to home or to another app

## What the OS provides

The OS calls your three functions, but you share everything else through helpers and the library:

- `loadState()` - transition to another app and run its `*_INIT()`
- `wakeFromNowlater()` - restore state when waking into an app
- `checkTimeout()` / `resetIdle()` - cooperate with the idle and sleep system
- `textPrompt()`, `boolPrompt()`, `datePrompt()`, `timePrompt()` - user input dialogs
- `waitForKeypress()` - block until a key is pressed
- `fileWizardMini()` - file selection
- `EINK()`, `OLED()`, `KB()`, `SD()` - hardware access through the library

## Read next

- [Making Apps](making-apps.md)
- [PocketMageOS overview](../development/index.md)
- [App API Reference](../reference/app-api.md)
