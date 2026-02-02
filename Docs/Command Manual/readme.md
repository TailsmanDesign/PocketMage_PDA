# PocketMage Keystrokes Guide
This is a guide on common key combinations and commands on the PocketMage PDA device. The guide is split up into sections based on application.

---
## General Keystrokes (work in almost any app)
- **( < )** | Cursor left
- **( > )** | Cursor right
- **(SHIFT) + ( < )** | Cursor to start of line
- **(SHIFT) + ( > )** | Cursor to end of line
- **(FN) + ( o )** | Clear line
- **(FN) + ( < )** | Exit or back button
- **(FN) + ( > )** | Save document
- **(SHIFT) + (key)** | Capital letter
- **(FN) + (Key)** | FN layer keymapping (legends on the PCB)
- **(FN) + (SHIFT) + (Key)** | FN+SHIFT layer keymapping
- **( o ) OR (ENTER)** | Select button

---
## Keymap
### Default Layer
```
[q] [w] [e] [r] [t] [y] [u] [i] [o] [p]
[a] [s] [d] [f] [g] [h] [j] [k] [l] [BKSP]
[TAB] [z] [x] [c] [v] [b] [n] [m] [.] [ENTER]
[SHIFT] [FN] [SPACE] [<] [o] [>]
```
### Shift Layer
```
[Q] [W] [E] [R] [T] [Y] [U] [I] [O] [P]
[A] [S] [D] [F] [G] [H] [J] [K] [L] [BKSP]
[TAB] [Z] [X] [C] [V] [B] [N] [M] ['] [ENTER]
[SHIFT] [FN] [SPACE] [SHIFT+<] [SHIFT+o] [SHIFT+>]
```
### Function Layer
```
[1] [2] [3] [4] [5] [6] [7] [8] [9] [0]
[#] [!] [$] [:] [;] [(] [)] [&] ["] [BKSP]
[FN+TAB] [%] [_] [+] [-] [*] [/] [?] [,] [ENTER]
[SHIFT] [FN] [SPACE] [FN+<] [FN+o] [FN+>]
```
### Function + Shift Layer
```
[!] [@] [#] [$] [%] [^] [&] [*] [(] [)]
[~] [`] [|] [[] []] [{] [}] [<] [>] [BKSP]
[FN+TAB] [%] [_] [&] [+] [-] [\] [?] [,] [ENTER]
[SHIFT] [FN] [SPACE] [FN+SHIFT+<] [FN+SHIFT+o] [FN+SHIFT+>]
```
---
## While Sleeping
### Bypass home and directly enter an app
You can bypass the home menu and enter directly into an app and wake up with one keystroke. Pressing the buttons below while PocketMage is sleeping will wake the device and boot into the corresponding app.

- **( SPACE )** - Return to previous app (saved state from last sleep)
- **( H )** - Home
- **( U )** - USB
- **( F )** - Filewiz
- **( T )** - Tasks
- **( N )** - TXT
- **( S )** - Settings
- **( C )** - Calendar
- **( J )** - Journal
- **( D )** - Dictionary (lexicon)
- **( L )** - Loader


---
## Home App
### Entering an OS app
Type an app's name to enter that app. For example, to enter calendar, type "calendar". You can type the name as it appears on the screen or use a shortcut. For example, typing "cal" also enters the calendar.

### Entering a 3rd party app
For 3rd party apps, type the letter of the slot that app is installed in. For example if you have the Calc app installed in the first app slot, type "a" to enter the app.

### Other commands
Many other commands can be done from the homescreen, including all of the settings commands and some other fun ones for you to discover!

---
## TXT App
- **(FN) + ( < )** | Exit app
- **(FN) + ( > )** | Save document
- **(FN) + ( o )** | Enter filesystem (loading files)
- **(SHFT) + ( o )** | New blank text document
- **(FN) + (Key)** | FN layer keymapping (legends on the PCB)
- **(SHFT) + (key)** | Capital letter
- **(ENTER)** | Create a new line
- **(SHFT) + ( < )** | Change text style (body, heading, etc.)
- **(SHFT) + ( > )** | Change formatting (bold, italics, etc.)
- **Scroll Bar** | Swipe up or down to scroll through the document

---
## FILEWIZ
- **(FN) + ( < )** | Exit app
- **( < ) AND ( > )** | Scroll left and right
- **( o ) OR (ENTER)** | Select file or folder
- **( 0 ) TO ( 9 )** | Select recent file
- **( BKSP )** | Go back a filesystem level

---
## USB
Plug in the PocketMage to your PC to view the files. Eject and exit the app when you're finished.
- **(FN) + ( < )** | Exit app

---
## Settings
Type the setting as it appears on the screen to change it. Some examples are given below. Note: all settings are case-insensitive, meaning that you can type in all lowercase. All of these settings are also available from the home menu command bar if you memorize them.
- TimeSet [HH]:[MM] -> TimeSet 15:46
- DateSet YYYYMMDD -> DateSet 20251230
- ShowYear [bool] -> ShowYear t
- Timeout [int] -> Timeout 300
- **(FN) + ( < )** | Exit app

---
## Tasks
- **( N )** | Create a new task, follow on-screen prompts
- **(ENTER)** | Enter information into prompt
- **( 0 ) TO ( 9 )** | Select task for editing
- **(FN) + ( < )** | Exit app

---
## Calendar
Type commands to navigate dates or create events. All commands are case-insensitive.

### Month View
- **jan 2025 / feb 2030 / etc.** | Jump to month and year
- **20251225** | Jump to exact date (YYYYMMDD)
- **14** | Jump to a day in the current month
- **( N )** | New event
- **(FN) + ( < )** | Exit app

### Week View
- **sun, mon, tue, wed, thu, fri, sat** | Jump to weekday in the viewed week
- **( N )** | New event
- **(FN) + ( < )** | Exit app

### Day View
- **( N )** | New event for selected day
- **1, 2, 3, ...** | Open event by index
- **(FN) + ( < )** | Exit app

### Repeating Events
- **no** | No repeat
- **daily** | Repeat every day
- **weekly xx** | Repeat every week, **xx** is one or more of **mo**, **tu**, **we**, **th**, **fr**, **sa**, **su**
- **monthly xx** | Repeat monthly, **xx** is the day of the month (1-31) or ordinal weekday (ex. **2tu** = the second Tuesday of each month)
- **yearly xx** | Repeat every year, **xx** is month and day of the month (ex. **apr22**)

---
## Journal
Type a date to open or create a journal entry. Commands are case-insensitive.

- **( T )** | Open today’s journal entry
- **YYYYMMDD - Example: 20250314** | Open/create entry for exact date
- **jan 1 / feb 12 / etc.** | Open/create entry for given month and day (uses current year)
- **(FN) + ( < )** | Exit app

---
## Lexicon
Type a word to search the dictionary. Matches are loaded from the SD card. Commands are case-insensitive.

- **Type any word** | Search for definitions (example: abandon)
- **(ENTER)** | Execute search
- **( < ) OR ( > )** | Previous / next definition
- **(FN) + ( < )** | Exit app

---
## App loader
Manage and install .tar apps to OTA slots. Commands are case-insensitive.
- **A / B / C / D** | Select OTA slot to edit
- **( S )** | Swap app in selected slot (choose a .tar file)
- **( D )** | Delete app in selected slot
- **(FN) + ( < )** | Exit app / return to menu
- **Progress Bar** | Shows extraction (0–50%) and installation (50–100%) status
---
## Sleep Modes
When on battery, save power and look at a random screensaver. When charging, view a clock, upcoming tasks, and weather (work in progress)
### Sleep (when not plugged into usb)
- **sleep button** to enter sleep
- **any key on keyboard** to wake
### Now-Later (when usb is plugged in)
- **sleep button** to enter now-later
- **sleep button** to wake
