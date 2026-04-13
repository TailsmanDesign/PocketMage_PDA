#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <config.h>

class MP2722 {
public:
  explicit MP2722(TwoWire& wirePort = Wire);

  bool begin();
  bool isConnected();

  // Structs
  struct MP2722_DPDMStatus {
    uint8_t code;
    float currentLimitA;
    const char* description;
  };

  struct MP2722_ChargeStatus {
    uint8_t code;
    const char* description;
  };

  struct DPDMResult {
      uint8_t code;
      const char* description;
      float currentLimitA;
  };

  // Register-level access
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &value);

  // Helper functions
  bool init(uint8_t sda, uint8_t scl);
  void printDiagnostics();
  bool setCCMode(uint8_t cc_cfg);
  bool setBoostCurrentLimit(float amps);
  bool getDPDMStatus(MP2722_DPDMStatus &out);
  bool getChargeStatus(MP2722_ChargeStatus &out);
  bool isBatteryLow(bool &low);
  bool setBoost(bool enable);
  bool getBoostState(bool &enabled);
  bool getDPDMStatus(DPDMResult &out);
  bool getOTGNeed(bool &boostNeeded);
  bool setFastChargeCurrent(uint16_t mA);

  void setUSBControlESP();
  void setUSBControlBMS();

private:
  TwoWire* _wire;
};

// Initialization of MP2722 Class
extern MP2722 PowerSystem;