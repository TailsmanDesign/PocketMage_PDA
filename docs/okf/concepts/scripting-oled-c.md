---
type: concept
title: "Scripting Example: OLED Drawing"
description: "A PocketMage script demonstrating drawing shapes and text on the OLED display."
source: "https://tailsmandesign.github.io/PocketMage_PDA/docs/scripting/oled-c/"
path: /scripting/oled-c/
updated: 2026-08-16
okf:
  generated_by: "@docmd/plugin-okf"
  generated_at: "2026-08-16T21:40:21.057Z"
---
---
title: "Scripting Example: OLED Drawing"
description: "A PocketMage script demonstrating drawing shapes and text on the OLED display."
---

# Scripting Example: OLED Drawing

The OLED is 256x32 pixels. Coordinates use the top-left origin, so the horizontal center is x = 128.

```c
// PocketMage OLED scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot oled-c.c
// Compile and run: brew oled-c.c
// Colors/clr: 1 - Black, 0 - White

delay(1000);

// Draw shapes on a white background
oledBackground(0);                               // background color
oledRect(10, 10, 25, 5, 1, 0);                   // x,y,w,h,line clr,fill clr
oledCircle(128, 10, 8, 1);                       // x,y,r,line clr
oledText(10, 20, 2, 1, "PocketMage!");           // x,y,size,color,text
updateOled();                                    // update OLED display

delay(3000);

// Draw shapes on a black background
oledBackground(1);                               // background color
oledRect(10, 10, 25, 5, 0, 1);                   // x,y,w,h,line clr,fill clr
oledCircle(128, 10, 8, 0);                       // x,y,r,line clr
oledText(10, 20, 2, 0, "Hello World!");          // x,y,size,color,text
updateOled();                                    // update OLED display

delay(3000);
```

## What it shows

- `oledBackground()` fills the OLED buffer.
- `oledRect()` and `oledCircle()` draw into the buffer.
- `oledText()` renders text at a font size.
- `updateOled()` pushes the buffer to the screen and clears it.

## Read next

- [E-Ink Drawing](ink-c.md)
- [Basic Input/Output](example-c.md)
- [Full Wrench Command List](fullPotionCommandList.md)
