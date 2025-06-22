#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

class Battery {
public:
	Battery(int batteryPin);
    int getBatteryPercentage() const;

private:
	int batteryPin;
    // Battery controls
    const float adcReferenceVoltage = 3.3;   // ADC reference on most ESP32 boards
    const float R1 = 22.0;                   // kohm
    const float R2 = 10.0;                   // kohm
    const float voltageDividerRatio = (R1 + R2) / R2; 
    const float adcResolution = 4095.0; 
    const float fullVoltage = 4.2;
    const float emptyVoltage = 3.0;
    const int batteryWarningThreshold = 20;     // percentage, below this, play warning chime
    const int batteryCriticalThreshold = 7;     // percentage, below this, shut down to protect battery

    float readBatteryVoltage() const;
};

#endif // BATTERY_H
