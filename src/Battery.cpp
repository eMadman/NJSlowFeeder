#include "Battery.h"

#define DEFAULT_VREF 1100  // in mV, used for calibration

RTC_DATA_ATTR float rtcFullVoltage = 4.2f;
RTC_DATA_ATTR float rtcEmptyVoltage = 3.0f;

Battery::Battery(gpio_num_t battery_gpio, adc1_channel_t channel, adc_unit_t adcUnit) 
            : battery_gpio(battery_gpio), adcChannel(channel), adcUnit(adcUnit) {}

void Battery::setup() {
    rtc_gpio_hold_dis(battery_gpio);  
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_12);  // up to 3.1V
    esp_adc_cal_characterize(adcUnit, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, DEFAULT_VREF, &adcChars);

    // calibrate actual full or empty battery voltage limits
    calibrateVoltageLimits();
}

float Battery::readRawVoltage() const {
    uint32_t raw = adc1_get_raw(adcChannel);
    uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adcChars);
    return (millivolts / 1000.0f) * voltageDividerRatio;
}

float Battery::getStableVoltage() const {
    float sum = 0.0f;
    for (int i = 0; i < numReading; ++i) {
        sum += constrain(readRawVoltage(), rtcEmptyVoltage, rtcFullVoltage);
        delay(10);
    }
    float voltage = sum / numReading;
    float rounded = roundf(voltage * 100.0f) / 100.0f; // return voltage rounded to 2 decimal digits
    return rounded;
}

float Battery::voltageToPercentage(float voltage) const {
    float percent = ((voltage - rtcEmptyVoltage) / (rtcFullVoltage - rtcEmptyVoltage)) * 100.0f;
    return constrain(percent, 0.0f, 100.0f);
}

int Battery::getBatteryPercentage() const {
    float voltage = getStableVoltage();
    // Serial.print("RTC Full Voltage: ");
    // Serial.println(rtcFullVoltage);
    // Serial.print("Battery Voltage: ");
    // Serial.println(voltage, 2);
    return (int)floorf(voltageToPercentage(voltage));
}

int Battery::getBatteryLevel() const {
    int percent = getBatteryPercentage();
    Serial.print("Battery Percent: ");
    Serial.println(percent);
    if (percent < batteryCriticalThreshold) { return BATTERY_CRITICAL; }
    else if (percent < batteryWarningThreshold) { return BATTERY_LOW; }
    else if (percent < batteryModerateThreshold) { return BATTERY_MEDIUM; }
    else { return BATTERY_HIGH; }
}

void Battery::maybeUpdateFullVoltage(float voltage) {
    if (voltage >= fullVoltage - voltageCalibOffset) {
        rtcFullVoltage = (1.0f - learningRate) * rtcFullVoltage + learningRate * voltage;
        Serial.print("Updated Full V: "); Serial.println(rtcFullVoltage);
    }
}

void Battery::maybeUpdateEmptyVoltage(float voltage) {
    if (voltage <= emptyVoltage + voltageCalibOffset) {
        rtcEmptyVoltage = (1.0f - learningRate) * rtcEmptyVoltage + learningRate * voltage;
        Serial.print("Updated Empty V: "); Serial.println(rtcEmptyVoltage);
    }
}

void Battery::calibrateVoltageLimits() {
    for (int i = 0; i < numCalibrations; ++i) {
        float voltage = getStableVoltage();
        int percent = (int)floorf(voltageToPercentage(voltage));
        // Serial.print("RTC Full Voltage: ");
        // Serial.println(rtcFullVoltage);
        // Serial.print("Battery Voltage: ");
        // Serial.println(voltage, 2);
        if (percent > batteryModerateThreshold && fabsf(voltage - rtcFullVoltage) > voltageCalibEpsilon) {
            maybeUpdateFullVoltage(voltage);
        } 
        else if (percent < batteryWarningThreshold && fabsf(voltage - rtcEmptyVoltage) > voltageCalibEpsilon) {
            maybeUpdateEmptyVoltage(voltage);
        } 
        else { return; } // Exit early if voltage is within expected range
    }
}