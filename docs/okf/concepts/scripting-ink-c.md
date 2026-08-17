---
type: concept
title: "Scripting Example: E-Ink Drawing"
description: "A PocketMage script demonstrating drawing shapes and text on the E-Ink display."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/scripting/ink-c/"
path: /scripting/ink-c/
updated: 2026-08-17
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-17T00:23:56.975Z"
---
---
title: "Scripting Example: E-Ink Drawing"
description: "A PocketMage script demonstrating drawing shapes and text on the E-Ink display."
---

# Scripting Example: E-Ink Drawing

The E-Ink display is 320x240 pixels. Coordinates use the top-left origin.

```c
// PocketMage E-Ink scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot ink.c
// Compile and run: brew ink.c
// Colors/clr: 1 - Black, 0 - White

delay(1000);

// Draw shapes on a white background
inkBackground(0);                              // background color
inkRect(10, 20, 50, 25, 1, 0);                 // x,y,w,h,line clr,fill clr
inkCircle(100, 50, 30, 1, 1);                  // x,y,r,line clr,fill clr
inkText(10, 100, 2, 1, "PocketMage!");         // x,y,size,color,text
updateInk();                                   // update E-Ink display

delay(3000);

// Draw shapes on a black background
inkBackground(1);                              // background color
inkRect(20, 10, 25, 50, 0, 1);                 // x,y,w,h,line clr,fill clr
inkCircle(120, 60, 35, 0, 0);                  // x,y,r,line clr,fill clr
inkText(30, 120, 3, 0, "Hello World!");        // x,y,size,color,text
updateInk();                                   // update E-Ink display

delay(3000);
```

## What it shows

- `inkBackground()` fills the whole screen.
- `inkRect()` and `inkCircle()` draw into the display buffer.
- `inkText()` renders text at a font size.
- `updateInk()` is the blocking hardware refresh; nothing appears until you call it.

## Read next

- [OLED Drawing](oled-c.md)
- [Basic Input/Output](example-c.md)
- [Full Wrench Command List](fullPotionCommandList.md)
