#include <MP2722.h>
#include <esp_sleep.h>

// Registers
#define MP2722_REG02 0x02
#define MP2722_REG08 0x08
#define MP2722_REG09 0x09
#define MP2722_REG11 0x11
#define MP2722_REG13 0x13
#define MP2722_REG16 0x16


// Initialization of MP2722 Class
MP2722 PowerSystem;

MP2722::MP2722(TwoWire& wirePort) : _wire(&wirePort) {}

bool MP2722::begin() {
  _wire->begin();
  return isConnected();
}

bool MP2722::isConnected() {
  _wire->beginTransmission(MP2722_ADDR);
  return (_wire->endTransmission() == 0);
}

bool MP2722::writeReg(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(MP2722_ADDR);
  _wire->write(reg);
  _wire->write(value);
  return (_wire->endTransmission() == 0);
}

bool MP2722::readReg(uint8_t reg, uint8_t &value) {
  _wire->beginTransmission(MP2722_ADDR);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;

  if (_wire->requestFrom(static_cast<uint8_t>(MP2722_ADDR), static_cast<uint8_t>(1)) != 1) return false;
  value = _wire->read();
  return true;
}

bool MP2722::setCCMode(uint8_t cc_cfg) {
  if (cc_cfg > 0b101) return false;  // invalid per datasheet

  uint8_t reg;
  if (!readReg(MP2722_REG09, reg)) return false;

  reg &= ~(0b111 << 4);          // clear bits 6:4
  reg |= (cc_cfg & 0b111) << 4;  // set new CC_CFG value

  return writeReg(MP2722_REG09, reg);
}

bool MP2722::setBoostCurrentLimit(float amps) {
  uint8_t code;
  if      (amps == 0.5f) code = 0b00;
  else if (amps == 1.5f) code = 0b01;
  else if (amps == 2.1f) code = 0b10;
  else if (amps == 3.0f) code = 0b11;
  else return false; // invalid input

  uint8_t reg;
  if (!readReg(MP2722_REG08, reg)) return false;

  reg &= ~(0b11 << 3);          // clear bits 4:3
  reg |= (code & 0b11) << 3;    // apply new OLIM bits

  return writeReg(MP2722_REG08, reg);
}


bool MP2722::getDPDMStatus(MP2722_DPDMStatus &out) {
  uint8_t reg;
  if (!readReg(MP2722_REG11, reg)) return false;

  uint8_t code = (reg >> 4) & 0x0F;
  out.code = code;

  switch (code) {
    case 0b0000: out.currentLimitA = 0.5f; out.description = "Not started (500mA)"; break;
    case 0b0001: out.currentLimitA = 0.5f; out.description = "USB SDP (500mA)"; break;
    case 0b0010: out.currentLimitA = 2.0f; out.description = "USB DCP (2A)"; break;
    case 0b0011: out.currentLimitA = 1.5f; out.description = "USB CDP (1.5A)"; break;
    case 0b0100: out.currentLimitA = 1.0f; out.description = "Divider 1 (1A)"; break;
    case 0b0101: out.currentLimitA = 2.1f; out.description = "Divider 2 (2.1A)"; break;
    case 0b0110: out.currentLimitA = 2.4f; out.description = "Divider 3 (2.4A)"; break;
    case 0b0111: out.currentLimitA = 2.0f; out.description = "Divider 4 (2A)"; break;
    case 0b1000: out.currentLimitA = 0.5f; out.description = "Unknown (500mA)"; break;
    case 0b1001: out.currentLimitA = 2.0f; out.description = "High-voltage adapter (2A)"; break;
    case 0b1110: out.currentLimitA = 3.0f; out.description = "Divider 5 (3A)"; break;
    default:     out.currentLimitA = 0.5f; out.description = "Reserved/Undefined (500mA)"; break;
  }

  return true;
}

bool MP2722::getChargeStatus(MP2722_ChargeStatus &out) {
  uint8_t reg;
  if (!readReg(MP2722_REG13, reg)) return false;

  uint8_t code = (reg >> 5) & 0b111;
  out.code = code;

  switch (code) {
    case 0b000: out.description = "Not charging"; break;
    case 0b001: out.description = "Trickle charge"; break;
    case 0b010: out.description = "Pre-charge"; break;
    case 0b011: out.description = "Fast charge"; break;
    case 0b100: out.description = "Constant-voltage charge"; break;
    case 0b101: out.description = "Charging done"; break;
    default:    out.description = "Reserved/Unknown"; break;
  }

  return true;
}

bool MP2722::isBatteryLow(bool &low) {
  uint8_t reg;
  if (!readReg(MP2722_REG16, reg)) return false;

  low = (reg >> 4) & 0x01;
  return true;
}

bool MP2722::setBoost(bool enable) {
    uint8_t reg;
    if (!readReg(0x09, reg)) return false;

    if (enable)
        reg |= (1 << 2);   // Set EN_BOOST bit
    else
        reg &= ~(1 << 2);  // Clear EN_BOOST bit

    if (!writeReg(0x09, reg)) return false;

    delay(10); // allow settling
    return true;
}

bool MP2722::getBoostState(bool &enabled) {
    uint8_t reg;
    if (!readReg(0x09, reg)) return false;
    enabled = reg & (1 << 2);
    return true;
}

void MP2722::setUSBControlESP() {
  digitalWrite(USB_MUX_PIN, HIGH);
}


void MP2722::setUSBControlBMS() {
  digitalWrite(USB_MUX_PIN, LOW);
}

bool MP2722::getDPDMStatus(DPDMResult &out) {
    uint8_t reg;
    if (!readReg(0x11, reg)) return false; // I2C fail

    out.code = (reg >> 4) & 0x0F;

    switch (out.code) {
        case 0x0: out.description = "Not started (500mA)"; out.currentLimitA = 0.5; break;
        case 0x1: out.description = "USB SDP (500mA)"; out.currentLimitA = 0.5; break;
        case 0x2: out.description = "USB DCP (2A)"; out.currentLimitA = 2.0; break;
        case 0x3: out.description = "USB CDP (1.5A)"; out.currentLimitA = 1.5; break;
        case 0x4: out.description = "Divider 1 (1A)"; out.currentLimitA = 1.0; break;
        case 0x5: out.description = "Divider 2 (2.1A)"; out.currentLimitA = 2.1; break;
        case 0x6: out.description = "Divider 3 (2.4A)"; out.currentLimitA = 2.4; break;
        case 0x7: out.description = "Divider 4 (2A)"; out.currentLimitA = 2.0; break;
        case 0x8: out.description = "Unknown (500mA)"; out.currentLimitA = 0.5; break;
        case 0x9: out.description = "High-voltage adapter (2A)"; out.currentLimitA = 2.0; break;
        case 0xE: out.description = "Divider 5 (3A)"; out.currentLimitA = 3.0; break;
        default:  out.description = "Reserved/Invalid"; out.currentLimitA = 0.0; break;
    }

    return true;
}

bool MP2722::getOTGNeed(bool &boostNeeded) {
    uint8_t reg;
    if (!readReg(0x16, reg)) return false; // I2C fail

    boostNeeded = (reg & (1 << 3)) != 0;
    return true;
}

bool MP2722::setFastChargeCurrent(uint16_t mA) {
  if (mA > 5040) return false; // Exceeds maximum possible 6-bit value (63 * 80mA)

  uint8_t code = mA / 80;

  uint8_t reg;
  if (!readReg(MP2722_REG02, reg)) return false;

  reg &= 0xC0;          // Clear bits 5:0 (ICC), preserve bits 7:6 (VPRE)
  reg |= (code & 0x3F); // Apply the new 6-bit current limit

  return writeReg(MP2722_REG02, reg);
}

void MP2722::printDiagnostics() {
    Serial.println(F("=== MP2722 Diagnostics ==="));

    // Connection check
    if (isConnected()) Serial.println(F("MP2722: Connected"));
    else { Serial.println(F("MP2722: Not detected")); return; }

    // CC mode
    uint8_t reg09;
    if (readReg(0x09, reg09)) {
        Serial.printf("CC_CFG[2:0]: 0b%03u\r\n", (reg09 >> 4) & 0b111);
    }

    // Boost current limit
    uint8_t reg08;
    if (readReg(0x08, reg08)) {
        uint8_t code = (reg08 >> 3) & 0b11;
        float amps;
        switch (code) {
            case 0b00: amps = 0.5f; break;
            case 0b01: amps = 1.5f; break;
            case 0b10: amps = 2.1f; break;
            case 0b11: amps = 3.0f; break;
        }
        Serial.printf("Boost current limit: %.1f A\r\n", amps);
    }

    // DPDM status
    MP2722_DPDMStatus dpdm;
    if (getDPDMStatus(dpdm)) {
        Serial.printf("DPDM: 0x%X - %s (%.1fA)\r\n", dpdm.code, dpdm.description, dpdm.currentLimitA);
    }

    // Charge status
    MP2722_ChargeStatus chg;
    if (getChargeStatus(chg)) {
        Serial.printf("Charge status: 0x%X - %s\r\n", chg.code, chg.description);
    }

    // Boost status
    bool boostOn;
    if (PowerSystem.getBoostState(boostOn)) {
        Serial.printf("Boost state: %s\n", boostOn ? "ENABLED" : "DISABLED");
    }

    // Battery low
    bool low;
    if (isBatteryLow(low)) {
        Serial.print(F("Battery low: "));
        Serial.println(low ? "YES" : "NO");
    }

    Serial.println(F("============================"));
}

// ----- INIT ----- //
bool MP2722::init(uint8_t sda, uint8_t scl) {
  // Start I2C
  Wire.begin(sda, scl);
  if (!begin()) return false;

  // Designate Multiplexer pin as output
  pinMode(USB_MUX_PIN, OUTPUT);

  // Give USB control to BMS
  setUSBControlBMS();

  // Set fast charge current limit to 1200mA
  if (!setFastChargeCurrent(1200)) return false;

  // Set CC mode: 011 = Dual Role Power, try SNK
  // Set CC mode: 100 = Dual Role Power, try SRC
  // Set CC mode: 010 = Dual Role Power
  if (!setCCMode(0b011)) return false;

  // Disable reverse boost
  if (!setBoost(false)) return false;

  // Set boost current limit to 0.5A
  if (!setBoostCurrentLimit(0.5f)) return false;

  // Check battery low status
  bool low;
  if (!isBatteryLow(low)) return false;
  if (low) {
    #pragma message "TODO: Add OLED battery low warning"
    //esp_deep_sleep_start();  // Enter deep sleep immediately
    return false;            // Not reached
  }

  return true;  // All steps succeeded
}