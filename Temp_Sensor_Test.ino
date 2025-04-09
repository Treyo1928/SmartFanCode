// Temp_Sensor_Test.ino
// Standalone test for TMP36 temperature sensor 


#include "temperature_sensor.h"

void setup() {
  Serial.begin(9600);
  Serial.println("TMP36 Sensor Test Initialized");
  delay(1000);
}

void loop() {
  float temp = getTemperature();
  Serial.print("Current Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");
  delay(1000);
}