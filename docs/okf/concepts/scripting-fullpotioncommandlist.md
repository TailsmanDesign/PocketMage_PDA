---
type: concept
title: "PocketMage Scripting Reference"
description: "Complete API reference for Wrench script bindings on PocketMage."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/scripting/fullPotionCommandList/"
path: /scripting/fullPotionCommandList/
updated: 2026-08-18
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-18T01:16:52.298Z"
---
---
title: "PocketMage Scripting Reference"
description: "Complete API reference for Wrench script bindings on PocketMage."
---

# PocketMage Scripting Reference

This document details the Wrench script bindings available in the PocketMage OS. Functions are grouped by their operational category.

---

## 1. Input / Output & Terminal

### `print(text)`

Appends a string to the terminal output buffer. Does not automatically refresh the screen.

* **Parameters:** `text` (String) - The string to print.
* **Returns:** None.
* **Example:**

  ```c
  print("Hello, terminal!");
  ```

### `updateTerm()`

Refreshes the terminal display to show the latest buffer contents.

* **Parameters:** None.
* **Returns:** None.
* **Example:**

  ```c
  print("Loading...");
  updateTerm();
  ```

### `prompt(promptText)`

Opens a text input prompt interface and pauses script execution until the user submits or cancels.

* **Parameters:** `promptText` (String) - The message displayed to the user.
* **Returns:** (String) The entered text, or an empty string `""` if the user cancels (`_EXIT_`).
* **Example:**

  ```c
  name = prompt("Enter your name:");
  print("Hello " + name);
  ```

### `boolPrompt(promptText)`

Opens a yes/no boolean prompt interface.

* **Parameters:** `promptText` (String) - The question displayed to the user.
* **Returns:** (Int) `1` for yes/true, `0` for no/false.
* **Example:**

  ```c
  if (boolPrompt("Save file?")) {
      writeFile("/data.txt", "saved");
  }
  ```

### `timePrompt(defaultTime)`

Opens a time selection interface.

* **Parameters:** `defaultTime` (Int) - The initial time value to display.
* **Returns:** (Int) The selected time.
* **Example:**

  ```c
  alarm = timePrompt(1200);
  ```

### `datePrompt(defaultDate)`

Opens a date selection interface.

* **Parameters:** `defaultDate` (String) - The initial date to display (format: max 15 chars).
* **Returns:** (String) The selected date.
* **Example:**

  ```c
  dueDate = datePrompt("2026-05-01");
  ```

### `waitForKeypress(message)`

Halts script execution and waits for the user to press a key.

* **Parameters:** `message` (String) - Text to display while waiting.
* **Returns:** None.
* **Example:**

  ```c
  waitForKeypress("Press any key to continue...");
  ```

---

## 2. Main E-Paper Display

### `updateInk()`

Pushes the current E-Ink drawing buffer to the physical e-paper display. This is a slow, blocking hardware update.

* **Parameters:** None.
* **Returns:** None.
* **Example:**

  ```c
  inkText(10, 20, 2, 1, "Hello E-Ink");
  updateInk();
  ```

### `inkBackground(color)`

Fills the entire E-Ink screen with a solid color.

* **Parameters:** `color` (Int) - `0` for white, `1` (or non-zero) for black.
* **Returns:** None.
* **Example:**

  ```c
  inkBackground(1);
  ```

### `inkRect(x, y, width, height, borderColor, fillColor)`

Draws a rectangle on the E-Ink display buffer.

* **Parameters:**
  * `x`, `y` (Int) - Origin coordinates.
  * `width`, `height` (Int) - Dimensions.
  * `borderColor` (Int) - `0` to draw border, `1` for no border.
  * `fillColor` (Int) - `0` to fill rectangle, `1` for transparent/no fill.
* **Returns:** None.
* **Example:**

  ```c
  inkRect(10, 10, 100, 50, 0, 1);
  ```

### `inkCircle(x, y, radius, borderColor, fillColor)`

Draws a circle on the E-Ink display buffer.

* **Parameters:**
  * `x`, `y` (Int) - Center coordinates.
  * `radius` (Int) - Radius in pixels.
  * `borderColor` (Int) - `0` to draw border, `1` for no border.
  * `fillColor` (Int) - `0` to fill circle, `1` for transparent/no fill.
* **Returns:** None.
* **Example:**

  ```c
  inkCircle(100, 100, 25, 0, 0);
  ```

### `inkText(x, y, size, color, text)`

Renders text to the E-Ink display buffer.

* **Parameters:**
  * `x`, `y` (Int) - Cursor start coordinates.
  * `size` (Int) - Font size mapping: `1` = 5x7 Fixed, `2` = 9pt Bold, `3` = 12pt Bold.
  * `color` (Int) - `0` for white text, `1` (or non-zero) for black text.
  * `text` (String) - Text to display.
* **Returns:** None.
* **Example:**

  ```c
  inkText(5, 50, 3, 1, "PocketMage");
  ```

---

## 3. Secondary OLED Display

### `updateOled()`

Pushes the current OLED drawing buffer to the physical OLED screen and clears the internal buffer.

* **Parameters:** None.
* **Returns:** None.
* **Example:**

  ```c
  oledText(0, 10, 2, 0, "OLED Active");
  updateOled();
  ```

### `oledWord(text)`

Abstracted wrapper to quickly print a formatted string directly to the OLED display.

* **Parameters:** `text` (String) - Text to display.
* **Returns:** None.
* **Example:**

  ```c
  oledWord("Processing...");
  ```

### `sysMessage(msg, delayMs)`

Flashes a high-priority system message on the OLED and blocks execution for the specified time.

* **Parameters:**
  * `msg` (String) - System message text.
  * `delayMs` (Int) - Time in milliseconds to show the message before continuing.
* **Returns:** None.
* **Example:**

  ```c
  sysMessage("Script Error", 2000);
  ```

### `oledBackground(color)`

Fills the OLED screen buffer.

* **Parameters:** `color` (Int) - `0` to turn pixels ON, `1` to turn pixels OFF.
* **Returns:** None.
* **Example:**

  ```c
  oledBackground(1);
  ```

### `oledRect(x, y, width, height, borderColor, fillColor)`

Draws a rectangle on the OLED buffer.

* **Parameters:**
  * `x`, `y` (Int) - Origin coordinates.
  * `width`, `height` (Int) - Dimensions.
  * `borderColor` (Int) - `0` to draw border, `1` for no border.
  * `fillColor` (Int) - `0` to fill rectangle, `1` for transparent/no fill.
* **Returns:** None.
* **Example:**

  ```c
  oledRect(0, 0, 256, 32, 0, 1);
  ```

### `oledCircle(x, y, radius, borderColor)`

Draws a circle outline on the OLED buffer. (No fill option supported).

* **Parameters:**
  * `x`, `y` (Int) - Center coordinates.
  * `radius` (Int) - Radius in pixels.
  * `borderColor` (Int) - `0` to draw border, `1` for no border.
* **Returns:** None.
* **Example:**

  ```c
  oledCircle(128, 16, 10, 0);
  ```

### `oledText(x, y, size, color, text)`

Renders text to the OLED buffer.

* **Parameters:**
  * `x`, `y` (Int) - Cursor start coordinates.
  * `size` (Int) - Font size mapping: `1` = 5x7, `2` = 7x13 Bold, `3` = HelvB14, `default` = LubR18.
  * `color` (Int) - `0` to turn text pixels ON, `1` to turn text pixels OFF (inverted).
  * `text` (String) - Text to display.
* **Returns:** None.
* **Example:**

  ```c
  oledText(0, 15, 1, 0, "Status: OK");
  ```

---

## 4. Hardware & System

### `getKey()`

Pulls the most recent character from the keyboard input buffer.

* **Parameters:** None.
* **Returns:** (Int) The ASCII keycode value of the pressed key. Returns `0` if no key is pressed.
* **Example:**

  ```c
  k = getKey();
  if (k == 13) { print("Enter pressed"); }
  ```

### `getTouch()`

Retrieves the scroll vector direction from the hardware touch sensor.

* **Parameters:** None.
* **Returns:** (Int) The directional scroll vector.
* **Example:**

  ```c
  scrollDir = getTouch();
  ```

### `getBattery()`

Retrieves the raw integer state of the battery monitor.

* **Parameters:** None.
* **Returns:** (Int) Battery state enum/integer.
* **Example:**

  ```c
  state = getBattery();
  ```

### `getBatteryVoltage()`

Retrieves the live battery voltage reading.

* **Parameters:** None.
* **Returns:** (Float) Battery voltage.
* **Example:**

  ```c
  volts = getBatteryVoltage();
  ```

### `setCpuSpeed(freq)`

Dynamically scales the CPU frequency.

* **Parameters:** `freq` (Int) - Target frequency in MHz.
* **Returns:** None.
* **Example:**

  ```c
  setCpuSpeed(80);
  ```

### `sleep()`

Triggers the device to enter deep sleep immediately.

* **Parameters:** None.
* **Returns:** None.
* **Example:**

  ```c
  sleep();
  ```

### `getTime()`

Gets the current system RTC time.

* **Parameters:** None.
* **Returns:** (String) Formatted time string `HH:MM:SS`.
* **Example:**

  ```c
  t = getTime();
  ```

### `getDate()`

Gets the current system RTC date.

* **Parameters:** None.
* **Returns:** (String) Formatted date string `YYYY-MM-DD`.
* **Example:**

  ```c
  d = getDate();
  ```

---

## 5. Filesystem (SD Card)

### `readFile(path)`

Reads the entire contents of a file from the SD card into a string.

* **Parameters:** `path` (String) - Absolute path to the file.
* **Returns:** (String) File contents.
* **Example:**

  ```c
  data = readFile("/data/config.txt");
  ```

### `writeFile(path, content)`

Writes a string to a file on the SD card. Overwrites existing files.

* **Parameters:**
  * `path` (String) - Absolute path to the target file.
  * `content` (String) - Data to write.
* **Returns:** None.
* **Example:**

  ```c
  writeFile("/data/log.txt", "Event recorded");
  ```

---

## 6. General Helpers

### `delay(ms)`

Halts script execution for a specified number of milliseconds.

* **Parameters:** `ms` (Int) - Milliseconds to wait.
* **Returns:** None.
* **Example:**

  ```c
  delay(500);
  ```

### `toInt(string)`

Converts a string representation of a number into an integer.

* **Parameters:** `string` (String) - The numeric string.
* **Returns:** (Int) The parsed integer value.
* **Example:**

  ```c
  val = toInt("1024");
  ```

### `random(min, max)`

Generates a pseudo-random integer within a specific range.

* **Parameters:**
  * `min` (Int) - Minimum bounds.
  * `max` (Int) - Maximum bounds.
* **Returns:** (Int) Random number between min and max (inclusive).
* **Example:**

  ```c
  diceRoll = random(1, 6);
  ```
