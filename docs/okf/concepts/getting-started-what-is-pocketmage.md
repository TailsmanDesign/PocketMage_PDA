---
type: concept
title: "What is PocketMage?"
description: "A short explanation of the PocketMage device and firmware stack."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/getting-started/what-is-pocketmage/"
path: /getting-started/what-is-pocketmage/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T22:45:57.801Z"
---
---
title: "What is PocketMage?"
description: "A short explanation of the PocketMage device and firmware stack."
---

# What is PocketMage?

PocketMage is a clamshell PDA built around an ESP32-S3. It is open-source hardware and software, designed to be built at home, hacked, and extended. This page explains the device and the two software layers that run on it.

::: grids
::: grid
::: card "Dual displays" icon:monitor
A 320x240 E-Ink panel for the main workspace and a 256x32 OLED for status and fast feedback.
:::
:::
::: grid
::: card "Full keyboard" icon:keyboard
A tactile thumb keyboard with four layers: normal, shift, function, and function plus shift.
:::
:::
::: grid
::: card "Two software layers" icon:layers
PocketMageOS handles apps and state; the PocketMage Library handles the hardware.
:::
:::
::: grid
::: card "Open and extendable" icon:wrench
Build apps with C++ or run Wrench scripts from the terminal. Load third-party apps over OTA.
:::
:::
:::

## The hardware

The main display is a 3.1" 1-bit E-Ink panel (320x240). E-Ink is sharp and power efficient, but slow to update, so a small OLED sits below it for status text, prompts, and feedback that needs to appear instantly. The keyboard is a matrix driven by a TCA8418 IC, which is why it can support four separate key layers.

The device runs on an ESP32-S3 with 16 MB flash and WiFi and Bluetooth radios. Storage lives on a microSD card. Everything else - RTC, buzzer, battery management, USB, and an expansion port with I2C/SPI/UART breakouts - is exposed to software through the shared library.

## The two software layers

PocketMage firmware is split into two layers so that apps stay small.

- **PocketMageOS** is the operating system. It owns boot, app switching, state, keyboard routing, and the screen dispatch loop. If you are changing how the device behaves, you are working in PocketMageOS.
- **PocketMage Library** is the reusable hardware and UI support layer in `Code/PocketMageOS/lib/PocketMage/`. It wraps the display, storage, keyboard, audio, clock, and system helpers so apps never talk to hardware directly.

The split matters because both native apps and third-party OTA apps build on the same library. An app does three jobs - initialize, handle input, draw - and uses the library for everything else.

## What ships on the device

The stock firmware includes these apps:

| App | Purpose |
| --- | --- |
| Home | Launch apps and run commands |
| TXT | Text and markdown-style editing |
| FileWiz | File browsing and file actions |
| USB | Expose storage over USB |
| COMM | Peer chat over the mesh radio |
| Settings | System configuration |
| Tasks | Task list management |
| Calendar | Date and event management |
| Journal | Daily notes |
| Lexicon | Dictionary lookup from the SD card |
| Loader | OTA app management |
| Terminal | Wrench scripting and shell workflows |
| SSH | Remote shell over WiFi (terminal sub-mode) |

## Why the split matters for you

- Building an app? Read [Making Apps](../guides/making-apps.md) and use the [PocketMage Library](../reference/pocketmage-library.md).
- Changing the OS itself? Start with [PocketMageOS overview](../development/index.md).
- Automating from the device? Use the [Wrench scripting](../scripting/index.md) surface.
- Just using the device? Keep the [Command Manual](../command-manual/index.md) nearby.

## Next steps

::: grids
::: grid
::: button "PocketMageOS overview" ../development/index.md icon:cpu
:::
::: grid
::: button "PocketMage Library" ../reference/pocketmage-library.md icon:box
:::
::: grid
::: button "Making Apps" ../guides/making-apps.md icon:code
:::
::: grid
::: button "Command Manual" ../command-manual/index.md icon:keyboard
:::
:::
