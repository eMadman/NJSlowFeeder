#include "Battery.h"

Battery::Battery(int pin) : batteryPin(pin) {}

float Battery::readBatteryVoltage() const {
    float raw = analogRead(batteryPin);
    float vAdc = raw * (adcReferenceVoltage / adcResolution); // For 10-bit ADC and 3.3V reference
    return vAdc * voltageDividerRatio; // Adjust for 22k/10k divider
}

int Battery::getBatteryPercentage() const {
    float voltage = constrain(readBatteryVoltage(), emptyVoltage, fullVoltage);
    int percent = (int)(((voltage - emptyVoltage) / (fullVoltage - emptyVoltage)) * 100.0);
    return percent;
}

int Battery::getBatteryWarningThreshold() const {
	return batteryWarningThreshold;
}

int Battery::getBatteryCriticalThreshold() const {
	return batteryCriticalThreshold;
}

