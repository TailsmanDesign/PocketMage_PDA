---
type: concept
title: "Scripting Example: Basic Input/Output"
description: "A simple PocketMage script demonstrating user input, OLED output, and terminal printing."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/scripting/example-c/"
path: /scripting/example-c/
updated: 2026-08-17
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-17T00:23:56.974Z"
---
---
title: "Scripting Example: Basic Input/Output"
description: "A simple PocketMage script demonstrating user input, OLED output, and terminal printing."
---

# Scripting Example: Basic Input/Output

```c
// PocketMage scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot example.c
// Compile and run: brew example.c

// Declare a variable
var x = ""; // "" indicates str

// Prompt user to enter number
// Set x to entered number
x = prompt("Enter a number");

// Display x on OLED
oledWord(x);

// Print x to terminal
print(x);

// Update the terminal
// (refresh the OLED)
updateTerm();

// Initialize an iteration var i
var i = 0;

// Loop while i < x
// Use toInt to convert str to int
while (i < toInt(x)) {
  // Iterate
  i = i + 1;

  // Display i on OLED
  oledWord(i);

  // Wait 500ms
  delay(500);
}

// Print the final i to the term
print(i);

// Generate a random number
var randomInt = random(0, 100);
print("Random Number: " + randomInt);

// Update the terminal
// (refresh the OLED)
updateTerm();
```

## What it shows

- `prompt()` reads a string from the user.
- `toInt()` converts that string to an integer for the loop condition.
- `oledWord()` prints to the OLED.
- `print()` appends to the terminal buffer; `updateTerm()` repaints it.
- `delay()` and `random()` are available as general helpers.

## Read next

- [E-Ink Drawing](ink-c.md)
- [OLED Drawing](oled-c.md)
- [Full Wrench Command List](fullPotionCommandList.md)
