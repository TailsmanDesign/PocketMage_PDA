#define   KB_IRQ           8
//#define   PWR_BTN          0
#define   BAT_SENS         4
#define   CHRG_SENS       39
#define   RTC_INT          1

void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);

  //set the resolution to 12 bits (0-4095)
  //analogReadResolution(12);

  pinMode(41, OUTPUT);
  //pinMode(PWR_BTN, INPUT);
  pinMode(KB_IRQ, INPUT);
}

void loop() {
  
  //Serial.print("PWR: "); Serial.print(digitalRead(PWR_BTN));
  Serial.print(", KB: "); Serial.print(analogRead(KB_IRQ));
  Serial.print(", CHRG: "); Serial.print(analogRead(CHRG_SENS));
  Serial.print(", RTC: "); Serial.print(analogRead(RTC_INT));

  // Read and convert battery voltage
  float batteryVoltage = analogRead(BAT_SENS) * (3.3 / 4095.0) * 2;
  Serial.print(", BAT: "); Serial.print(batteryVoltage, 2); // Print with 2 decimal places

  Serial.println();
  delay(100);
  /*
  digitalWrite(41, true);
  Serial.print("ON\n");
  delay(1000);
  digitalWrite(41, false);
  Serial.print("OFF\n");
  delay(1000);
  */

}
