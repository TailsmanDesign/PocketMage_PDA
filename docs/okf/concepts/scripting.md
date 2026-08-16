---
type: concept
title: Scripting
description: "Scripting reference and examples for the PocketMage TERMINAL app (Wrench)."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/scripting/"
path: /scripting/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T22:45:57.808Z"
---
---
title: "Scripting"
description: "Scripting reference and examples for the PocketMage TERMINAL app (Wrench)."
---

# Scripting

The PocketMage TERMINAL app runs scripts written in Wrench, a lightweight C-like scripting language. Scripts can draw to the E-Ink and OLED displays, handle input, and talk to the rest of the system through the PocketMage library surface.

Wrench is a small interpreted language with a familiar `if` / `while` / `var` syntax. On PocketMage it is embedded inside the TERMINAL app, so every script is a `.c` file on the SD card that the terminal compiles and runs on demand.

## What Wrench is for

Wrench sits one level above a native app. It trades raw performance for convenience:

- **Automation** - script repeated terminal workflows instead of typing them each time
- **Custom screens** - draw to the E-Ink or OLED with a few function calls
- **Prototyping** - test an idea in a script before writing it as a native app
- **Device control** - read the clock, battery, keyboard, and touch sensors from a script

Because scripts run in the TERMINAL app, they already have access to everything the terminal can reach: the displays, the SD card, prompts, and the shared system helpers.

## Script lifecycle

1. **Edit** - create or open a `.c` script in TXT, or use the terminal's editor. Save it to the SD card.
2. **Compile** - run `brew <script>.c` in the TERMINAL app to compile and execute it.
3. **Run** - the terminal interprets the script. `print()` sends output to the terminal buffer, `updateTerm()` paints it to the OLED.

The example scripts at the top of the page double as runnable tests: [Basic Input/Output](example-c.md) walks through the whole edit-to-run flow.

## Bindings overview

Wrench bindings fall into five groups. The [full command list](fullPotionCommandList.md) documents every one with parameters, return values, and examples.

- **Input/output and terminal** - `print()`, `updateTerm()`, `prompt()`, `boolPrompt()`, `timePrompt()`, `datePrompt()`, `waitForKeypress()`
- **E-Ink display** - `updateInk()`, `inkBackground()`, `inkRect()`, `inkCircle()`, `inkText()`
- **OLED display** - `updateOled()`, `oledWord()`, `sysMessage()`, `oledBackground()`, `oledRect()`, `oledCircle()`, `oledText()`
- **Hardware and system** - `getKey()`, `getTouch()`, `getBattery()`, `getBatteryVoltage()`, `setCpuSpeed()`, `sleep()`, `getTime()`, `getDate()`
- **Filesystem** - `readFile()`, `writeFile()`
- **General helpers** - `delay()`, `toInt()`, `random()`

## Language notes

- Variables are declared with `var` and are dynamically typed. `var x = ""` creates a string; `var i = 0` creates an integer.
- Strings convert with `toInt()` where an integer is required.
- Control flow uses C-style braces and parentheses.
- `random(min, max)` returns a value in the inclusive range.
- `delay()` is a blocking wait, in milliseconds.

## Read this if

- you want to automate something from the terminal app
- you want to draw custom screens
- you want to understand how Wrench fits into PocketMageOS

## Related

- [App API Reference](../reference/app-api.md) - app entry points and helpers
- [PocketMage Library](../reference/pocketmage-library.md) - the shared hardware and UI layer

## Next steps

::: grids
::: grid
::: button "Basic Input/Output" ./example-c.md icon:code
:::
::: grid
::: button "E-Ink Drawing" ./ink-c.md icon:monitor
:::
::: grid
::: button "OLED Drawing" ./oled-c.md icon:monitor
:::
::: grid
::: button "Full Command List" ./fullPotionCommandList.md icon:book
:::
:::
