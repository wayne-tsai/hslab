#include <Arduino.h>

const int pinD1 = 3; // GPIO 3 for D1
const int pinD2 = 4; // GPIO 4 for D2

void setup() {
  Serial.begin(115200);
  pinMode(pinD1, INPUT);
  pinMode(pinD2, INPUT);
}

void loop() {
  int adcValueD1 = analogRead(pinD1);
  int adcValueD2 = analogRead(pinD2);

  float voltageD1 = (adcValueD1 / 4095.0) * 3.3;
  float voltageD2 = (adcValueD2 / 4095.0) * 3.3;

  Serial.print("Voltage at D1: ");
  Serial.print(voltageD1);
  Serial.println(" V");

  Serial.print("Voltage at D2: ");
  Serial.print(voltageD2);
  Serial.println(" V");

  delay(1000);
}