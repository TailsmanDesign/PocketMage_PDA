#include <pocketmage.h>

void PocketMage_INIT(){
  // Serial, I2C, SPI
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(SPI_SCK, -1, SPI_MOSI, -1);

  // OLED SETUP
  setupOled();

  // STARTUP JINGLE
  setupBZ();
  
  // WAKE INTERRUPT SETUP
  pinMode(KB_IRQ, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_8, 0);

  // KEYBOARD SETUP
  setupKB();

  // EINK HANDLER SETUP
  setupEink();
  
  // SD CARD SETUP
  setupSD();

  // POWER SETUP
  pinMode(PWR_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PWR_BTN), pocketmage::power::PWR_BTN_irq, FALLING);
  pinMode(CHRG_SENS, INPUT);
  pinMode(BAT_SENS, INPUT);
  //WiFi.mode(WIFI_OFF);
  //btStop();

  // SET CPU CLOCK FOR POWER SAVE MODE
  if (SAVE_POWER) setCpuFrequencyMhz(40 );
  else            setCpuFrequencyMhz(240);
  
  // CAPACATIVE TOUCH SETUP
  setupTouch();

  // RTC SETUP
  setupClock();

  // Set "random" seed
  randomSeed(analogRead(BAT_SENS));
}