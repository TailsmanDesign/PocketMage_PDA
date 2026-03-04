#include <SPIFFS.h>
//#include <TinyUSB.h>

void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  delay(1000);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Create a test file if it doesn’t already exist
  File file = SPIFFS.open("/testfile.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
  } else {
    file.println("Hello from ESP32-S3!");
    file.close();
  }

  // Initialize TinyUSB for Mass Storage
  TinyUSBDevice.begin();
  TinyUSBMSC.begin();

  // Define callbacks for TinyUSB mass storage events
  TinyUSBMSC.setReadCallback([](uint32_t lba, void* buffer, uint32_t bufsize) -> int {
    // Read from SPIFFS to fill the buffer for USB transfer
    File file = SPIFFS.open("/testfile.txt", FILE_READ);
    if (!file) return -1;
    
    file.readBytes((char*)buffer, bufsize);
    file.close();
    return bufsize;
  });

  TinyUSBMSC.setWriteCallback([](uint32_t lba, const void* buffer, uint32_t bufsize) -> int {
    // Write from the buffer to SPIFFS for USB transfer
    File file = SPIFFS.open("/testfile.txt", FILE_WRITE);
    if (!file) return -1;
    
    file.write((const uint8_t*)buffer, bufsize);
    file.close();
    return bufsize;
  });

  TinyUSBMSC.setCapacity(2048, 512); // Set capacity: 1MB (2048 blocks of 512 bytes)
  TinyUSBMSC.setUnitReady(true);     // Mark mass storage as ready
  Serial.println("USB Mass Storage initialized.");
}

void loop() {
  // Nothing needed here for USB mass storage
}