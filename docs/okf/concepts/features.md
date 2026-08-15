---
type: concept
title: Features
description: "What PocketMage can do today."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/features/"
path: /features/
updated: 2026-08-15
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-15T18:08:50.949Z"
---
---
title: "Features"
description: "What PocketMage can do today."
---

# Features

PocketMage is a productivity PDA first. The device focuses on reading, editing, quick navigation, and simple workflows that fit E-Ink. This page covers what the current firmware can do and what is planned.

::: tag "Productivity"
::: tag "E-Ink"
::: tag "Apps"
::: tag "Hardware"

## The design philosophy

The device pairs a slow, sharp E-Ink panel with a fast, small OLED. Anything that benefits from instant feedback - prompts, status, timers - lives on the OLED. Anything that benefits from a large readable page - notes, files, calendars - lives on the E-Ink. The result is a note-taking device that feels responsive without sacrificing E-Ink's power efficiency.

## Current apps

::: grids
::: grid
::: card "Home" icon:home
Launch apps by name or shortcut and run commands like `roll d6` or `sleep`.
:::
:::
::: grid
::: card "TXT" icon:file-text
Edit plain text and markdown-style notes. Supports fonts, styles, and formatting.
:::
:::
::: grid
::: card "FileWiz" icon:folder
Browse files and folders, open recent files, and move through the filesystem.
:::
:::
::: grid
::: card "USB" icon:usb
Mount the microSD card as a mass-storage device on a PC.
:::
:::
::: grid
::: card "COMM" icon:radio
Peer chat over the mesh radio. See the mesh add-on module.
:::
:::
::: grid
::: card "Settings" icon:settings
Change system options: time, date, brightness, timeout, language, and more.
:::
:::
::: grid
::: card "Tasks" icon:list-check
Manage task lists with due dates, editing, and sorting.
:::
:::
::: grid
::: card "Calendar" icon:calendar
Month, week, and day views with events and repeating schedules.
:::
:::
::: grid
::: card "Journal" icon:notebook
Open a daily entry by date and keep a running log.
:::
:::
::: grid
::: card "Lexicon" icon:book
Search the dictionary loaded on the SD card.
:::
:::
::: grid
::: card "Loader" icon:package
Install, swap, and delete OTA apps in the four app slots.
:::
:::
::: grid
::: card "Terminal" icon:terminal
Run Wrench scripts for custom screens and automation.
:::
:::
::: grid
::: card "SSH" icon:radio
Connect to a remote host over WiFi from the terminal app (`ssh` command).
:::
:::
:::

## Hardware behavior

::: callout tip "The screen split matters"
E-Ink handles the main workspace. OLED handles status and fast feedback. The keyboard supports normal, shift, function, and function plus shift layers.
:::

## Sleep and power

On battery, the device enters sleep and shows a random screensaver. When plugged into USB it uses a now-later screen with a clock, upcoming tasks, and weather (work in progress). Waking can go straight into a specific app via the boot shortcut keys, so common flows start with a single keypress.

## What is next

| Area | Status |
| --- | --- |
| Timers and alarms | Planned |
| E-book reader | Planned |
| WiFi notes/calendar sync | Planned |
| LoRa/Meshtastic add-on module | Planned |
| Battery expansion module | Planned |
| Custom abbreviations | Planned |
| Bluetooth keyboard support | Planned |
| Weather on now-later screen | In progress |

## Next steps

::: grids
::: grid
::: button "What is PocketMage" ../getting-started/what-is-pocketmage.md icon:info
:::
::: grid
::: button "PocketMageOS" ../development/index.md icon:cpu
:::
::: grid
::: button "Command Manual" ../command-manual/index.md icon:keyboard
:::
::: grid
::: button "Guides" ../guides/index.md icon:book-open
:::
:::
