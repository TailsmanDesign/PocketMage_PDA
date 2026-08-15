---
type: reference
title: "System State Reference"
description: "The AppState enum, boot shortcuts, and how the OS dispatches between apps."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/reference/system-state/"
path: /reference/system-state/
updated: 2026-08-15
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-15T18:08:50.954Z"
---
---
title: "System State Reference"
description: "The AppState enum, boot shortcuts, and how the OS dispatches between apps."
---

# System State Reference

PocketMageOS is a state machine. The single `CurrentAppState` variable decides which app owns the keyboard and the screen at any moment. This page documents that state and everything around it: the enum, the home screen, boot shortcuts, and how dispatch works.

## The AppState enum

`CurrentAppState` holds one value from `AppState`, declared in `Code/PocketMageOS/include/globals.h`:

```cpp
enum AppState { HOME, TXT, FILEWIZ, USB_APP, COMM, SETTINGS, TASKS, CALENDAR, JOURNAL, LEXICON, APPLOADER, TERMINAL };
```

| Value | App |
| --- | --- |
| `HOME` | Home screen |
| `TXT` | TXT editor |
| `FILEWIZ` | FileWiz file manager |
| `USB_APP` | USB mode |
| `COMM` | COMM (mesh chat) |
| `SETTINGS` | Settings |
| `TASKS` | Tasks |
| `CALENDAR` | Calendar |
| `JOURNAL` | Journal |
| `LEXICON` | Lexicon |
| `APPLOADER` | App loader (OTA apps) |
| `TERMINAL` | Terminal (Wrench shell) |

## How dispatch works

The OS has two dispatch functions in `PocketMageV3.cpp`:

- `processKB()` - reads the keyboard and routes the keypress to the active app's keyboard handler
- `applicationEinkHandler()` - routes screen refresh to the active app's draw handler

Both switch on `CurrentAppState` and call the matching per-app function. When you add a native app, you extend `AppState` and add cases in the spots marked `// ADD APP CASES HERE`.

## The HOME state

The home screen has its own sub-state machine, `HOMEState`:

```cpp
enum HOMEState { HOME_HOME, NOWLATER };
```

- `HOME_HOME` - the normal home screen with the app grid
- `NOWLATER` - the "save state" view that shows unsaved data before going to sleep

`CurrentHOMEState` tracks which one is active.

## Boot shortcuts

Holding a key during boot launches an app directly. The mapping lives in `bootShortcutApp()` in `UTILS.cpp`:

| Key | App |
| --- | --- |
| `h` | Home |
| `u` | USB mode |
| `f` | FileWiz |
| `t` | Tasks |
| `n` | TXT editor |
| `s` | Settings |
| `c` | Calendar |
| `j` | Journal |
| `d` | Lexicon |
| `x` | Terminal |
| `l` | App loader |

## Shared state you will touch

Declared in `globals.h` and `config.h`:

- `CurrentAppState` - the active app; drives the dispatch switch
- `CurrentHOMEState` - home screen mode
- `newState` - when set, the target app's `*_INIT()` runs on entry
- `prefs` - persistent settings
- `global_fs` - the active filesystem
- `disableTimeout` - global idle-timeout lockout
- `OTA1_APP` through `OTA4_APP` - installed OTA app names

## Transitions

- `loadState()` - switch to another app, running its `*_INIT()`
- `wakeFromNowlater()` - restore state when waking out of sleep
- `checkTimeout()` - sleep when the idle timer elapses
- `resetIdle()` - reset the idle clock

## Next steps

::: grids
::: grid
::: button "App API Reference" ./app-api.md icon:book
:::
::: grid
::: button "PocketMage Library" ./pocketmage-library.md icon:box
:::
::: grid
::: button "OS Overview" ../development/index.md icon:settings
:::
::: grid
::: button "Making Apps" ../guides/making-apps.md icon:code
:::
:::
