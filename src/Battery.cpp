#include "Battery.h"

#define DEFAULT_VREF 1100  // in mV, used for calibration

Battery::Battery(adc1_channel_t channel, adc_unit_t adcUnit) 
            : adcChannel(channel), adcUnit(adcUnit) {}

void Battery::setup() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_12);  // up to 3.1V
    esp_adc_cal_characterize(adcUnit, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adcChars);
}

float Battery::readBatteryVoltage() const {
    uint32_t raw = adc1_get_raw(adcChannel);
    uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adcChars);
    return (millivolts / 1000.0f) * voltageDividerRatio;
}

int Battery::getBatteryPercentage() const {
    float voltage = constrain(readBatteryVoltage(), emptyVoltage, fullVoltage);
    int percent = (int)(((voltage - emptyVoltage) / (fullVoltage - emptyVoltage)) * 100.0f);
    Serial.print("Battery Voltage: ");
    Serial.println(voltage);
    Serial.print("Battery Percent: ");
    Serial.println(percent);
    return percent;
}

int Battery::getBatteryLevel() const {
    int percent = getBatteryPercentage();
    if (percent < batteryCriticalThreshold) { return BATTERY_CRITICAL; }
    else if (percent < batteryWarningThreshold) { return BATTERY_LOW; }
    else if (percent < batteryModerateThreshold) { return BATTERY_MEDIUM; }
    else { return BATTERY_HIGH; }
}