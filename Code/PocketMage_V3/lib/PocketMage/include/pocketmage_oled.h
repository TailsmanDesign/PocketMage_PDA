//    .oooooo.   ooooo        oooooooooooo oooooooooo.    //
//   d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888          888          888      888  //
//  888      888  888          888oooo8     888      888  //
//  888      888  888          888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //     

#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <vector>
#pragma region fonts
#pragma endregion

// OLED 
extern U8G2_SSD1326_ER_256X32_F_4W_HW_SPI u8g2;

// ===================== OLED CLASS =====================
class PocketmageOled {
public:
  explicit PocketmageOled(U8G2 &u8) : u8g2_(u8) {}

  
  // Main methods
  void oledWord(String word, bool allowLarge = false, bool showInfo = true);
  void oledLine(String line, int input_pos, bool doProgressBar = true, String bottomMsg = "");
  void oledScroll();
  void infoBar();
  void setPowerSave(bool enable);
  bool getPowerSave() const                                   { return OLEDPowerSave_; }

private:
  U8G2                  &u8g2_;        // class reference to hardware oled object
  volatile bool OLEDPowerSave_;
 
  // helpers
  uint16_t strWidth(const String& s) const;
};

void setupOled();
PocketmageOled& OLED();
