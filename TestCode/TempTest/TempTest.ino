#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMP_PIN 2  // Use Pin 2 (or another digital pin)

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  while (!Serial);  // Wait for Serial Monitor to open (important for Nano 33 BLE)
  
  Serial.println("Initializing DS18B20...");

  sensors.begin();

  if (sensors.getDeviceCount() == 0) {
    Serial.println("No DS18B20 detected! Check wiring.");
  } else {
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount());
    Serial.println(" DS18B20 sensor(s).");
  }
}

void loop() {
  Serial.print("Requesting temperature... ");
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == -127.00) {
    Serial.println("ERROR: Sensor not responding!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");
  }

  delay(1000);
}
