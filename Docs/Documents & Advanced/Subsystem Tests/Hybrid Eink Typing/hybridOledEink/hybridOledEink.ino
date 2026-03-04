#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Fonts/FreeMonoBold9pt7b.h>

#define FULL_REFRESH_AFTER 20

//FONT AND DISP SETUP
//U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
GxEPD2_BW<GxEPD2_310_GDEQ031T10, GxEPD2_310_GDEQ031T10::HEIGHT> display(GxEPD2_310_GDEQ031T10(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); 
// GDEQ031T10 240x320, UC8253, (no inking, backside mark KEGMO 3100)
U8G2_SH1106_128X32_VISIONOX_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

//VARIABLES
String currentWord = "";
String allText = "";
String prevAllText = "";
volatile int einkRefresh = 20;
int previousMillis = 0;

//FUNCTIONS
char scanSerial() {
  char incomingChar;

  if (Serial.available() > 0) {
    // Read the incoming character
    incomingChar = Serial.read();
    
    // Echo the character back to the terminal
    Serial.print("You typed: ");
    Serial.println(incomingChar);
    return incomingChar;
  }
  return 255;
}

void oledWord(String word) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_ncenB24_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+12,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB18_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+9,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB14_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+7,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB12_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+6,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB10_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+5,word.c_str());
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (u8g2.getStrWidth(word.c_str()) < 128) {
    u8g2.drawStr((128 - u8g2.getStrWidth(word.c_str()))/2,16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  else {
    u8g2.drawStr(128 - u8g2.getStrWidth(word.c_str()),16+4,word.c_str());
    u8g2.sendBuffer();
    return;
  }
  
}

void einkText(String text) {
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(0, 10);
  display.print(text);
  display.nextPage();
  display.hibernate();
}

void einkTextPartial(String text) {
  display.setFont(&FreeMonoBold9pt7b);

  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

  int bottomY = 10+tbh;

  display.setPartialWindow(0,0,display.width(),display.height());
  display.fillScreen(GxEPD_WHITE);
  display.setCursor(0, 10);
  display.print(text);
  display.nextPage();
  display.hibernate();
  display.setFullWindow();
}

void einkHandler(void *parameter) {
  while (true) {
    if (prevAllText != allText) {
      if (allText ==  "") {
        einkText(allText);
      }

      prevAllText = allText;
      einkRefresh++;

      if (einkRefresh > FULL_REFRESH_AFTER) {
        einkText(allText);
        einkRefresh = 0;
      }
      else einkTextPartial(allText);
    }
  }
}

//SETUP AND LOOP
void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    einkHandler,            // Function name (your user-defined function)
    "einkHandlerTask",      // Task name
    10000,                  // Stack size (in bytes)
    NULL,                   // Parameters (none in this case)
    1,                      // Priority (1 is low priority)
    NULL,                   // Task handle (optional)
    1                       // Core ID (0 for core 0, 1 for core 1)
  );

  display.init(115200);
  display.setRotation(3);
  
  display.setTextColor(GxEPD_BLACK);
  
  Wire.begin(36, 35);
  u8g2.begin();
  u8g2.clearBuffer();	
  u8g2.sendBuffer();

  while (scanSerial() == 255) {
    String teststr = "";
    String exampleword = "Start Typing...";
    for (int i = 0; i<15; i++) {
      teststr = exampleword.substring(0,i);
      oledWord(teststr);
      delay(100);
    }
    delay(1000);
  }
  Serial.println("Serial Works!");
}

void loop() {
  char inchar = scanSerial();

  if (inchar == 255);                           //No char recieved
  else if (inchar == 32) {                      //Space Recieved
    allText += (currentWord + " ");
    currentWord = "";
  }
  else if (inchar == 13) {                      //CR Recieved
    allText += (currentWord + "\n");
    currentWord = "";
  }
  else if (inchar == 27) {                      //ESC Recieved
    allText ="";
    currentWord = "";
    oledWord("Clearing...");
    delay(500);
  }
  else if (inchar == 127 || inchar == 8) {      //BKSP recieved
    if (currentWord.length() > 0) {
      currentWord = currentWord.substring(0, currentWord.length() - 1);
    }
  }
  else currentWord += inchar;
  
  int currentMillis = millis();
  if (currentMillis - previousMillis >= 16) { //Make sure oled only updates at 60fps
    previousMillis = currentMillis;
    oledWord(currentWord);
  }
}
