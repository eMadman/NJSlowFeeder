#include "Board.h"
#include <Arduino.h>
#include "driver/rtc_io.h"

Board::Board(float calibrationFactor)
    : motor(IN1MotorPin, IN2MotorPin),
    loadCell(HX_DOUT, HX_CLK, calibrationFactor),
    battery(batteryPin),
    buttonUp(buttonUpPin, BUTTON_PULLDOWN, true, 50),
	buttonDown(buttonDownPin, BUTTON_PULLDOWN, true, 50),
	lastClickTimeUp(0),
	lastClickTimeDown(0),
	lastMotorActiveTime(0),
    delayStartTime(0),
    waitingAfterClick(false) {}

void Board::setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println("Starting up...");

    motor.setup();
    Serial.println("Setting up motor");

    if (isHX711Connected(loadCellDetectTimeout)) {
        loadCellPresent = true;
        loadCell.setup();
        Serial.println("Setting up loadCell");
    }
    else {
        loadCellPresent = false;
        Serial.println("Load Cell not detected");
    }

    if (isBatteryMonitorConnected()) {
        batteryMonitorPresent = true;
        Serial.println("Battery monitor detected");
    }
    else {
        batteryMonitorPresent = false;
        Serial.println("Battery monitor not detected");
    }

    printWakeupReason();

    buttonUp.releaseHandler(onRelease);
    buttonUp.holdHandler(onHold, 1000);

    buttonDown.releaseHandler(onRelease);
    buttonDown.holdHandler(onHold, 1000);

    delay(1000);
    playStartupChime();

	updateButtons();
    buttonUp.buttonstatus = BUTTON_IDLE;
    buttonDown.buttonstatus = BUTTON_IDLE;

    Serial.println(buttonUp.buttonstatus);
    Serial.println(buttonDown.buttonstatus);
}

void Board::updateButtons() {

    buttonUp.process();
    buttonDown.process();

    // Double-click detection for UP button
    if (buttonUp.buttonstatus == BUTTON_CLICK) {
        unsigned long now = millis();
        if (now - lastClickTimeUp <= doubleClickInterval) {
            buttonUp.buttonstatus = BUTTON_DOUBLE_CLICK;
            lastClickTimeUp = 0; // reset
        } 
		else {
            lastClickTimeUp = now;
        }
    }

    // Double-click detection for DOWN button (optional)
    if (buttonDown.buttonstatus == BUTTON_CLICK) {
        unsigned long now = millis();
        if (now - lastClickTimeDown <= doubleClickInterval) {
			Serial.println("Down button double click");
            buttonDown.buttonstatus = BUTTON_DOUBLE_CLICK;
            lastClickTimeDown = 0;
        } 
		else {
            lastClickTimeDown = now;
        }
    }
}

void Board::playLowBatteryChime() {
    motor.makeNoise(1000, 150);
    delay(100);
    motor.makeNoise(800, 150);
    delay(100);
    motor.makeNoise(600, 200);
}

void Board::playStartupChime() {
    motor.makeNoise(1000, 150);
    motor.makeNoise(1300, 150);
    motor.makeNoise(1600, 150);
    motor.makeNoise(2000, 200);
}

void Board::playDeepSleepChime() {
	motor.makeNoise(1800, 150);
    motor.makeNoise(1400, 150);
    motor.makeNoise(1000, 150);
    motor.makeNoise(600, 300);
}
bool Board::isBatteryMonitorConnected(int minValidAdc) {
    int raw = analogRead(batteryPin);
    return raw > minValidAdc;
}

bool Board::isHX711Connected(unsigned long timeout) {
    pinMode(HX_DOUT, INPUT);
    unsigned long start = millis();
    while (digitalRead(HX_DOUT) == HIGH) {
        if (millis() - start > timeout) {
            return false; // Not ready in time — possibly disconnected
        }
    }
    return true;
}

bool Board::shouldSleep() {
	return (millis() - lastMotorActiveTime > sleepTimeoutTime) || buttonDown.buttonstatus == BUTTON_DOUBLE_CLICK;
}

void Board::enterDeepSleep() {
    delay(500);
    playDeepSleepChime();

    if (loadCellPresent){
        // Turn off hx711 pins
        rtc_gpio_pulldown_dis(HX711CLK);
        rtc_gpio_pullup_en(HX711CLK);

        pinMode(HX_CLK, OUTPUT);
        digitalWrite(HX_CLK, HIGH);
        pinMode(HX_DOUT, INPUT);
    }

    // Turn off motor pins
    pinMode(IN1MotorPin, OUTPUT);
    digitalWrite(IN1MotorPin, LOW);
    pinMode(IN2MotorPin, OUTPUT);
    digitalWrite(IN2MotorPin, LOW);

    // Put buttonDownPin to input with pullup to avoid floating
    pinMode(buttonDownPin, INPUT_PULLUP);

    // Turn batteryPin to input
    if (batteryMonitorPresent){
        pinMode(batteryPin, INPUT);
    }

    // Wakeup config
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, HIGH);
    rtc_gpio_pullup_dis(WAKEUP_GPIO);   
    rtc_gpio_pulldown_en(WAKEUP_GPIO);

      // Report
    Serial.print("Motor idle for (s): ");
    Serial.println((millis() - lastMotorActiveTime) / 1000);

    Serial.println("Going to deep sleep");
    Serial.flush();
    delay(50);
    esp_deep_sleep_start();
}

void Board::printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
        default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
}

void Board::onRelease(Button& b) {
    Serial.print("onRelease: ");
    Serial.println(b.pin);
	switch (b.buttonstatus)
	{
		case BUTTON_IDLE:
			Serial.print("Click: ");
			Serial.println(b.pin);
			b.buttonstatus = BUTTON_CLICK;
			break;
		case BUTTON_HOLD:
			Serial.print("HoldRelease: ");
			Serial.println(b.pin);
			b.buttonstatus = BUTTON_IDLE;
		default:
			break;
    } 	
}

void Board::onHold(Button& b) {
    Serial.print("onHold: ");
    Serial.println(b.pin);
    b.buttonstatus = BUTTON_HOLD;
}

void Board::handleButtonAction() {
	switch (buttonUp.buttonstatus) {
        case BUTTON_HOLD:  
            // Serial.print("Holding Up; Current Voltage: ");
            // Serial.print("Holding Up; New Voltage: ");
            // Serial.println(constrain(currVoltage + motor.getVoltageStep(), 0, motor.getBusVoltage()), 2);
            if (motor.getVoltage() > 0) {
                motor.setVoltage(IN1MotorPin, motor.getVoltage() + motor.getVoltageStep());
            }
            break;

        case BUTTON_CLICK:  // Click
            // loadCell.tare();
            // motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            // buttonUp.buttonstatus = 0;
            // break;
            if (!motor.started()) {
                motor.setMotorStartTime();
            }

            motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            delayStartTime = millis();
            waitingAfterClick = true;
			buttonUp.buttonstatus = BUTTON_IDLE;
            break;

		case BUTTON_DOUBLE_CLICK:
            if (!motor.started()) {
                motor.setMotorStartTime();
            }
			motor.setVoltage(IN1MotorPin, motor.getMaxVoltage(), true);
			buttonUp.buttonstatus = BUTTON_IDLE;
			break;

        default:
            // No action needed
            break;
    }

    switch (buttonDown.buttonstatus) {
        case BUTTON_HOLD:  
            if (motor.getVoltage() > 0) {
                // Serial.print("Holding down; MotorVoltage: ");
                // Serial.println(constrain(currVoltage - MotorVoltageStep, 0, BusVoltage), 2);
                
                // ensure motor doesn't stop when holding down button
                float newVoltage = motor.getVoltage() - motor.getVoltageStep();
                newVoltage = max(newVoltage, motor.getMinVoltage());
                motor.setVoltage(IN1MotorPin, newVoltage);
            }
            break;

        case BUTTON_CLICK:
            motor.resetMotorStartTime();
            motor.setVoltage(IN1MotorPin, 0);
			buttonDown.buttonstatus = BUTTON_IDLE;
            if (loadCellPresent) {
                loadCell.reset();
            }
            break;

        default:
            // No action needed
            break;
    }
}

bool Board::shouldStopMotor() {
    if (loadCellPresent) {
        return motor.shouldStop() || loadCell.shouldStop();
    }
    return motor.shouldStop();
}

void Board::checkAndProtectBattery() {
    if (!batteryMonitorPresent){ return; }

    int batteryPercent = battery.getBatteryPercentage();
    // Serial.print("Battery level: ");
    // Serial.println(batteryPercent);

    if (batteryPercent <= battery.getBatteryCriticalThreshold()) {
        Serial.println("Critical battery! Shutting down to protect battery.");
        motor.setVoltage(IN1MotorPin, 0, true);
        // playLowBatteryChime();
        enterDeepSleep();  // Force sleep
    }
    else if (batteryPercent <= battery.getBatteryWarningThreshold()) {
        Serial.print("Low battery warning: ");
        Serial.print(batteryPercent);
        Serial.println("% remaining.");
        // playLowBatteryChime();  // Only a warning
    }
}

void Board::processFeedingCycle() {
	if (loadCellPresent && waitingAfterClick) {
		// delay for 1s after clicking button
		// to avoid flucuations in readings
		if(millis() - delayStartTime >= delayAfterClick) {
			// reset to wait afte click
			waitingAfterClick = false;
			delayStartTime = millis();
		}
	}
	else {
		if (motor.getVoltage() > 0) {
			lastMotorActiveTime = millis();
            if (loadCellPresent){
                loadCell.update();
            }
			if (shouldStopMotor()) {
				motor.setVoltage(IN1MotorPin, 0);
                motor.resetMotorStartTime();
                if (loadCellPresent){
                    loadCell.reset();
                }
			}
		}   
	}
}

Motor& Board::getMotor() { return motor; }
LoadCell& Board::getLoadCell() { return loadCell; }
Button& Board::getButtonUp() { return buttonUp; }
Button& Board::getButtonDown() { return buttonDown; }