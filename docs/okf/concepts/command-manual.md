---
type: concept
title: "PocketMage Command Manual"
description: "Keystrokes and command reference for the PocketMage PDA."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/command-manual/"
path: /command-manual/
updated: 2026-08-04
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-04T02:13:37.647Z"
---
---
title: "PocketMage Command Manual"
description: "Keystrokes and command reference for the PocketMage PDA."
---

# PocketMage Command Manual

This page is the fast reference for keys and built-in commands. For setup or app-building help, start with the [Getting Started](../getting-started/index.md) and [Guides](../guides/index.md) pages.

## Keyboard layers

PocketMage uses four keyboard layers:

- normal
- shift
- function (FN)
- function plus shift

Use the PCB legends for the full key map. This page focuses on what each app does with those keys.

## General keys

| Key                        | Action                                   |
|----------------------------|------------------------------------------|
| **( < )**                  | Cursor left                              |
| **( > )**                  | Cursor right                             |
| **(SHIFT) + ( < )**        | Cursor to start of line                  |
| **(SHIFT) + ( > )**        | Cursor to end of line                    |
| **(FN) + ( o )**           | Clear line                               |
| **(FN) + ( < )**           | Exit or back button                      |
| **(FN) + ( > )**           | Save document                            |
| **(SHIFT) + (key)**        | Capital letter                           |
| **(FN) + (Key)**           | FN layer keymapping (legends on the PCB) |
| **(FN) + (SHIFT) + (Key)** | FN + SHIFT layer keymapping              |
| **( o )** or **(ENTER)**   | Select button                            |

## Apps

| App      | Purpose                                    |
|----------|--------------------------------------------|
| Home     | Launch apps and run home commands          |
| TXT      | Text editing                               |
| FileWiz  | File browsing and file actions             |
| USB      | USB mass storage                           |
| COMM     | Peer chat over the mesh radio              |
| Settings | System configuration                       |
| Tasks    | Task list management                       |
| Calendar | Date and event management                  |
| Journal  | Daily notes                                |
| Lexicon  | Dictionary lookup                          |
| Loader   | OTA app management                         |
| Terminal | Wrench scripting and shell-style workflows |

Terminal hosts a sub-mode for **SSH** (remote shell over WiFi); enter it with the `ssh` command from the terminal prompt.

## Keymap

### Default layer

```txt
[q] [w] [e] [r] [t] [y] [u] [i] [o] [p]
[a] [s] [d] [f] [g] [h] [j] [k] [l] [BKSP]
[TAB] [z] [x] [c] [v] [b] [n] [m] [.] [ENTER]
[SHIFT] [FN] [SPACE] [<] [o] [>]
```

### Shift layer

```txt
[Q] [W] [E] [R] [T] [Y] [U] [I] [O] [P]
[A] [S] [D] [F] [G] [H] [J] [K] [L] [BKSP]
[TAB] [Z] [X] [C] [V] [B] [N] [M] ['] [ENTER]
[SHIFT] [FN] [SPACE] [SHIFT+<] [SHIFT+o] [SHIFT+>]
```

### Function layer

```txt
[1] [2] [3] [4] [5] [6] [7] [8] [9] [0]
[#] [!] [$] [:] [;] [(] [)] [&] ["] [BKSP]
[FN+TAB] [%] [_] [+] [-] [*] [/] [?] [,] [ENTER]
[SHIFT] [FN] [SPACE] [FN+<] [FN+o] [FN+>]
```

### Function + shift layer

```txt
[!] [@] [#] [$] [%] [^] [&] [*] [(] [)]
[~] [`] [|] [[] []] [{] [}] [<] [>] [BKSP]
[FN+TAB] [%] [=] [&] [+] [-] [\] [?] [,] [ENTER]
[SHIFT] [FN] [SPACE] [FN+SHIFT+<] [FN+SHIFT+o] [FN+SHIFT+>]
```

## While sleeping

You can bypass the home menu and enter an app directly: pressing the key below while sleeping wakes the device and boots into the corresponding app.

| Key         | App                                                  |
|-------------|------------------------------------------------------|
| **(SPACE)** | Return to previous app (saved state from last sleep) |
| **(H)**     | Home                                                 |
| **(U)**     | USB                                                  |
| **(F)**     | FileWiz                                              |
| **(T)**     | Tasks                                                |
| **(N)**     | TXT                                                  |
| **(S)**     | Settings                                             |
| **(C)**     | Calendar                                             |
| **(J)**     | Journal                                              |
| **(D)**     | Lexicon                                              |
| **(X)**     | Terminal                                             |
| **(L)**     | Loader                                               |

## Home app

### Enter an OS app

Type an app's name to enter that app. For example, type `calendar` to enter the calendar. Shortcuts also work: `cal` enters the calendar.

### Enter a third-party app

Type the letter of the slot the app is installed in. For example, if the Calc app is in the first slot, type `a` to enter it.

### Commands

Home commands are typed in the home command bar. Examples:

| Command                         | Action                  |
|---------------------------------|-------------------------|
| `roll dN`                       | Roll a die with N sides |
| `reset`                         | Reset the device        |
| `sdreset`                       | Re-mount the SD card    |
| `sleep`                         | Enter sleep             |
| `note`, `txt`, `write`          | Open TXT                |
| `filewiz`, `file`               | Open FileWiz            |
| `usb`, `transfer`               | Open USB                |
| `app`, `loader`                 | Open the app loader     |
| `tasks`                         | Open Tasks              |
| `term`, `terminal`, `cmd`       | Open the Terminal       |
| `bluetooth`, `bt`               | Bluetooth setup         |
| `settings`, `preferences`       | Open Settings           |
| `cal`                           | Open Calendar           |
| `journal`, `daily`              | Open Journal            |
| `lexicon`, `dict`, `dictionary` | Open Lexicon            |
| `chat`, `msg`                   | Open COMM               |
| `version`, `ver`                | Show the OS version     |

Numeric shortcuts also map to apps: `0` is Terminal, `1` TXT, `2` FileWiz, `3` USB, `4` COMM, `5` Settings, `6` Tasks, `7` Calendar, `8` Journal, `9` Lexicon.

See the [system state reference](../reference/system-state.md) and the [PocketMage Library](../reference/pocketmage-library.md) to understand how those commands map to PocketMageOS code.

## TXT app

| Key                        | Action                                          |
|----------------------------|-------------------------------------------------|
| **(FN) + ( < )**           | Exit app                                        |
| **(FN) + ( > )**           | Save document                                   |
| **(FN) + ( o )**           | Enter filesystem (loading files)                |
| **(SHIFT) + ( o )**        | New blank text document                         |
| **(FN) + (TAB)**           | Cycle font family (SERIF / SANS / MONO)         |
| **(FN) + (SHIFT) + ( o )** | Jump to a specific line number                  |
| **(ENTER)**                | Create a new line                               |
| **(SHIFT) + ( < )**        | Change text style (body, heading, etc.)         |
| **(SHIFT) + ( > )**        | Change formatting (bold, italics, etc.)         |
| **Scroll bar**             | Swipe up or down to scroll through the document |

## FileWiz

| Key                      | Action                     |
|--------------------------|----------------------------|
| **(FN) + ( < )**         | Exit app                   |
| **( < )** and **( > )**  | Scroll left and right      |
| **( o )** or **(ENTER)** | Select file or folder      |
| **( 0 )** to **( 9 )**   | Select recent file         |
| **(BKSP)**               | Go back a filesystem level |

## USB

Plug the PocketMage into your PC to view the files. Eject and exit the app when you are finished.

| Key              | Action   |
|------------------|----------|
| **(FN) + ( < )** | Exit app |

## COMM

Peer chat over the mesh radio. See the mesh radio add-on module for setup.

## Settings

Type the setting as it appears on the screen to change it. All settings are case-insensitive and are also available from the home command bar. Examples:

| Command                                  | Action                                                 |
|------------------------------------------|--------------------------------------------------------|
| `timeset 15:46` or `settime 15:46`       | Set time (HH:MM)                                       |
| `dateset 20251230` or `setdate 20251230` | Set date (YYYYMMDD)                                    |
| `lock`                                   | Report lock state; `lock on` / `lock off` to change it |
| `lockpin <pin>`                          | Set the lock PIN                                       |
| `lumina <0-255>`                         | OLED brightness                                        |
| `timeout <sec>`                          | Auto sleep timeout                                     |
| `oledfps <fps>`                          | OLED max FPS                                           |
| `clock <on/off>`                         | Show the clock on screen                               |
| `showyear <on/off>`                      | Show the year in the clock                             |
| `savepower <on/off>`                     | Power saving mode                                      |
| `fastrefresh <on/off>`                   | Fast partial refresh mode                              |
| `debug <on/off>`                         | Verbose debug output                                   |
| `boottohome <on/off>`                    | Start home on boot                                     |
| `allownosd <on/off>`                     | Run without an SD card                                 |
| `lang <code>`                            | Set the interface language                             |

| Key              | Action   |
|------------------|----------|
| **(FN) + ( < )** | Exit app |

## Tasks

| Key                    | Action                                      |
|------------------------|---------------------------------------------|
| **(N)**                | Create a new task, follow on-screen prompts |
| **(ENTER)**            | Enter information into a prompt             |
| **( 0 )** to **( 9 )** | Select task for editing                     |
| **(FN) + ( < )**       | Exit app                                    |

## Calendar

Type commands to navigate dates or create events. Commands are case-insensitive.

### Month view

| Input                          | Action                             |
|--------------------------------|------------------------------------|
| `jan 2025` / `feb 2030` / etc. | Jump to month and year             |
| `20251225`                     | Jump to exact date (YYYYMMDD)      |
| `14`                           | Jump to a day in the current month |
| **(N)**                        | New event                          |
| **(FN) + ( < )**               | Exit app                           |

### Week view

| Input                                           | Action                             |
|-------------------------------------------------|------------------------------------|
| `sun`, `mon`, `tue`, `wed`, `thu`, `fri`, `sat` | Jump to weekday in the viewed week |
| **(N)**                                         | New event                          |
| **(FN) + ( < )**                                | Exit app                           |

### Day view

| Input              | Action                         |
|--------------------|--------------------------------|
| **(N)**            | New event for the selected day |
| `1`, `2`, `3`, ... | Open event by index            |
| **(FN) + ( < )**   | Exit app                       |

### Repeating events

| Input        | Action                                                                                                                        |
|--------------|-------------------------------------------------------------------------------------------------------------------------------|
| `no`         | No repeat                                                                                                                     |
| `daily`      | Repeat every day                                                                                                              |
| `weekly xx`  | Repeat weekly; `xx` is one or more of `mo`, `tu`, `we`, `th`, `fr`, `sa`, `su`                                                |
| `monthly xx` | Repeat monthly; `xx` is the day of the month (1-31) or ordinal weekday (for example `2tu` = the second Tuesday of each month) |
| `yearly xx`  | Repeat yearly; `xx` is month and day (for example `apr22`)                                                                    |

## Journal

Type a date to open or create a journal entry. Commands are case-insensitive.

| Input                               | Action                                                     |
|-------------------------------------|------------------------------------------------------------|
| **(T)**                             | Open today's journal entry                                 |
| `YYYYMMDD` (for example `20250314`) | Open or create an entry for an exact date                  |
| `jan 1` / `feb 12` / etc.           | Open or create an entry for a month and day (current year) |
| **(FN) + ( < )**                    | Exit app                                                   |

## Lexicon

Type a word to search the dictionary. Matches load from the SD card. Commands are case-insensitive.

| Input                                 | Action                     |
|---------------------------------------|----------------------------|
| Type any word (for example `abandon`) | Search for definitions     |
| **(ENTER)**                           | Execute search             |
| **( < )** or **( > )**                | Previous / next definition |
| **(FN) + ( < )**                      | Exit app                   |

## App loader

Manage and install `.tar` apps into OTA slots. Commands are case-insensitive.

| Input             | Action                                                     |
|-------------------|------------------------------------------------------------|
| **A / B / C / D** | Select OTA slot to edit                                    |
| **(S)**           | Swap app in the selected slot (choose a `.tar` file)       |
| **(D)**           | Delete app in the selected slot                            |
| **(FN) + ( < )**  | Exit app / return to menu                                  |
| **Progress bar**  | Shows extraction (0-50%) and installation (50-100%) status |

## Sleep modes

On battery, the device saves power and shows a random screensaver. When charging, it shows a clock, upcoming tasks, and weather (work in progress).

### Sleep (battery)

- **sleep button** to enter sleep
- **any key on keyboard** to wake

### Now-later (USB plugged in)

- **sleep button** to enter now-later
- **sleep button** to wake
