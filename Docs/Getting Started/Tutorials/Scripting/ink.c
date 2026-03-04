// PocketMage E-Ink scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot ink.c
// Compile and run: brew ink.c
// Colors/clr: 1 - Black, 0 - White

delay(1000);

// Draw shapes on a white background
inkBackground(0); //Background clr
inkRect(10, 20, 50, 25, 1, 0); //x,y,w,h,line clr,fill clr
inkCircle(100, 50, 30, 1, 1); //x,y,r,line clr,fill clr
inkText(10, 100, 2, 1, "PocketMage!"); //x,y,font size,text
updateInk(); //Update E-Ink display

delay(3000);

// Draw shapes on a black background
inkBackground(1); //Background clr
inkRect(20, 10, 25, 50, 0, 1); //x,y,w,h,line clr,fill clr
inkCircle(120, 60, 35, 0, 0); //x,y,r,line clr,fill clr
inkText(30, 120, 3, 0, "Hello World!"); //x,y,font size,text
updateInk(); //Update E-Ink display

delay(3000);