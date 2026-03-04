// PocketMage OLED scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot ink.c
// Compile and run: brew ink.c
// Colors/clr: 1 - Black, 0 - White

delay(1000);

// Draw shapes on a white background
oledBackground(0); //Background clr
oledRect(10, 20, 25, 5, 1, 0); //x,y,w,h,line clr,fill clr
oledCircle(100, 10, 10, 1); //x,y,r,line clr
oledText(180, 20, 2, 1, "PocketMage!"); //x,y,font size,text
updateOled(); //Update OLED display

delay(3000);

// Draw shapes on a black background
oledBackground(1); //Background clr
oledRect(10, 20, 25, 5, 0, 1); //x,y,w,h,line clr,fill clr
oledCircle(100, 10, 10, 0); //x,y,r,line clr
oledText(120, 20, 3, 0, "Hello World!"); //x,y,font size,text
updateOled(); //Update OLED display

delay(3000);