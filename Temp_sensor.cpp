
#include "temperature_sensor.h"

const int TMP36_PIN = A1;

float getTemperature() {
	int analogValue = analogRead(TMP36_PIN);
	float voltage = analogValue * 5.0 / 1024.0;
	float temperatureC = (voltage - 0.5) * 100.0;
	return temperatureC;
}