#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>

// OLED SPI Pin Definitions
#define OLED_MOSI    14
#define OLED_SCK     15
#define OLED_CS      47
#define OLED_DC      46
#define OLED_RST     45

// SSD1326 256x32 OLED via 4-wire Software SPI (full buffer)
U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/OLED_CS, /* dc=*/OLED_DC, /* reset=*/OLED_RST);

void setup() {
  SPI.begin(OLED_SCK, -1, OLED_MOSI, OLED_CS);
  u8g2.begin();
  u8g2.setBusClock(10000000);  // 10 MHz for faster updates

  Serial.begin(9600);
}

void loop() {
  Serial.println("HELLO");

  for (int i = 0; i < u8g2.getDisplayWidth(); i+=5) {
    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, i, u8g2.getDisplayHeight());
    u8g2.sendBuffer();
  }

  delay(100);

  for (int i = u8g2.getDisplayWidth(); i > 0; i-=5) {
    u8g2.clearBuffer();
    u8g2.drawBox(0, 0, i, u8g2.getDisplayHeight());
    u8g2.sendBuffer();
  }

  delay(100);

  String message = "Very Long OLED!!!";
  u8g2.setFont(u8g2_font_ncenB18_tr);

  for (int i = 0; message[i] != '\0'; i++) {
    u8g2.clearBuffer();
    u8g2.drawStr(u8g2.getDisplayWidth() - 8 - u8g2.getStrWidth(message.c_str()), 19, message.substring(0, i + 1).c_str());
    u8g2.sendBuffer();
  }

  delay(1000);

  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  delay(250);
  u8g2.sendBuffer();

  u8g2.clearBuffer();
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  delay(250);
  u8g2.sendBuffer();

  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  delay(250);
  u8g2.sendBuffer();

  u8g2.clearBuffer();
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  delay(500);
  u8g2.setDrawColor(1);
}
