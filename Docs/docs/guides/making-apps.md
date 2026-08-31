---
title: "Making Apps"
description: "How to create PocketMage apps, both native and OTA."
---

# Making Apps

PocketMage apps come in two forms: native apps that ship inside PocketMageOS, and OTA apps that are loaded onto the device later. The two share the same core shape, so this page walks through that shape once and then explains where the paths diverge.

::: grids
::: grid
::: card "Native apps" icon:terminal
Compiled into the OS and wired into the app dispatch switch.
:::
:::
::: grid
::: card "OTA apps" icon:package
Compiled separately and loaded into one of four OTA slots at runtime.
:::
:::
::: grid
::: card "Same shape" icon:code
Both are three functions: init, handle input, draw.
:::
:::
:::

## The basic app shape

Every PocketMage app has the same three jobs, and they map directly to three functions.

1. **Initialize state** - run once when the app opens. Set up whatever the app needs and mark the screen for a redraw.
2. **Process keyboard input** - run on every keypress. Read the key, update app state, and queue a redraw.
3. **Draw the screen** - run when the E-Ink needs to update. Render the current app state.

This pattern is visible in `Code/PocketMageOS/include/globals.h` (the prototypes) and in the app files under `Code/PocketMageOS/src/OS_APPS/`.

## Start with the template

The app template is your starting point:

- [APP_TEMPLATE.cpp](https://github.com/TalismanDesign/PocketMage_PDA/blob/main/Code/PocketMageOS/src/APP_TEMPLATE.cpp)

It defines the three entry points an OTA app needs:

```cpp
#include <globals.h>
#if OTA_APP
void APP_INIT() {
  // 1. set up app state
}

void processKB_APP() {
  // 2. handle a keypress
}

void einkHandler_APP() {
  // 3. draw the screen
}
#endif
```

## Building the app step by step

1. **Decide native vs OTA.** If the app belongs to the device experience, make it native. If it is third-party or experimental, make it an OTA app so it can be installed and removed without reflashing the OS.
2. **Pick the shared helpers.** Browse the [PocketMage Library](../reference/pocketmage-library.md) before writing anything. Prompts (`textPrompt()`), file selection (`fileWizardMini()`), and timeouts (`checkTimeout()`) already exist; do not reimplement them.
3. **Wire it into the OS (native only).** Add an `AppState` entry, the three prototypes, and the `switch` cases marked `// ADD APP CASES HERE` in `PocketMageV3.cpp`. Then add a home-screen command if you want it launchable by name.
4. **Test the keyboard path first.** Get a keypress into your handler and onto the OLED before worrying about the E-Ink.
5. **Test the E-Ink path second.** Make sure drawing and refresh behave, including fast partial refreshes.
6. **Check timeout, wake, and save last.** Confirm the app resets the idle clock, handles sleep/wake without corrupting state, and saves before exit where it should.

## Good app rules

- Keep app state local and explicit; do not scatter globals.
- Use shared helpers instead of duplicating them.
- Keep draw code separate from input code.
- Make exit, save, and sleep behavior obvious.
- For native apps, follow the existing app in `src/OS_APPS/` that does the closest job.

## Native vs OTA: which path

| | Native | OTA |
| --- | --- | --- |
| Where the code lives | `src/OS_APPS/` inside PocketMageOS | Standalone project using `APP_TEMPLATE.cpp` |
| How it is built | With the OS (`PM_PRODUCTION`/`PM_BETA`) | With the `OTA_APP` environment |
| How it ships | Flashed as part of the firmware | Packaged as a `.tar` and loaded through the app loader |
| Dispatch | Wired into the switch in `PocketMageV3.cpp` | Entry points called by the loader |
| Hardware access | PocketMage Library | PocketMage Library |

Read the [Native Apps](native-apps.md) and [OTA Apps](ota-apps.md) pages for the details of each path.

## Next steps

::: grids
::: grid
::: button "Native Apps" ./native-apps.md icon:terminal
:::
::: grid
::: button "OTA Apps" ./ota-apps.md icon:package
:::
::: grid
::: button "App API Reference" ../reference/app-api.md icon:book
:::
::: grid
::: button "PocketMage Library" ../reference/pocketmage-library.md icon:box
:::
:::
