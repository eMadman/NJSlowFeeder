#include "Board.h"
#include <Arduino.h>
#include "driver/rtc_io.h"

Board::Board(float calibrationFactor)
    : motor(IN1MotorPin, IN2MotorPin),
    loadCell(HX_DOUT, HX_CLK, calibrationFactor),
    battery(batteryPin),
    buttonUp(buttonUpPin, BUTTON_PULLDOWN, true, 50),
	buttonDown(buttonDownPin, BUTTON_PULLDOWN, true, 50) {}

void Board::setup() {
    Serial.begin(115200);
    delay(2500);
    Serial.println("Starting up...");

    motor.setup();
    Serial.println("Setting up motor");

    if (isHX711Connected()) {
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
        if (battery.isBatteryCritical()) {
            batteryCritical = true;
        }
        else if (battery.isBatteryWarning()) {
            batteryWarning = true;
        }
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
    if (batteryCritical || batteryWarning) {
        playLowBatteryChime();
    }
    else {
        playStartupChime();
    }

	// updateButtons();
    buttonUp.process();
    buttonDown.process();
    buttonUp.buttonstatus = BUTTON_IDLE;
    buttonDown.buttonstatus = BUTTON_IDLE;
}

void Board::handleDoubleClick(Button& button, bool& pendingClick, unsigned long& clickStartTime) {
    unsigned long now = millis();

    if (button.buttonstatus == BUTTON_CLICK) {
        if (pendingClick && (now - clickStartTime <= doubleClickInterval)) {
            button.buttonstatus = BUTTON_DOUBLE_CLICK;
            pendingClick = false;
        } 
        else {
            // Temporarily assign to idle to wait for second click decision
            button.buttonstatus = BUTTON_IDLE;
            pendingClick = true;
            clickStartTime = now;
        }
    }

    if (pendingClick && (now - clickStartTime > doubleClickInterval)) {
        button.buttonstatus = BUTTON_CLICK;
        pendingClick = false;
    }
}

void Board::updateButtons() {
    // Update button response
    buttonUp.process();
    buttonDown.process();

    // Double-click detection 
    handleDoubleClick(buttonUp, pendingClickUp, clickUpStartTime);
    handleDoubleClick(buttonDown, pendingClickDown, clickDownStartTime);
}

void Board::playBatteryLevelChime() {
    int percentage = battery.getBatteryPercentage();

    int chimeCount = 0;
    if (percentage < 30) {
        chimeCount = 1;
    } 
    else if (percentage <= 65) {
        chimeCount = 2;
    } 
    else {
        chimeCount = 3;
    }
    Serial.print("Battery level (%): ");
    Serial.println(percentage);
    for (int i = 0; i < chimeCount; ++i) {
        motor.makeNoise(800, 300);
        delay(150);
    }
}

void Board::playLowBatteryChime() {
    motor.makeNoise(1000, 150);
    motor.makeNoise(800, 150);
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
    unsigned long now = millis();
    bool timeout = (now - lastMotorActiveTime > sleepTimeoutTime) && (now - lastButtonActiveTime > sleepTimeoutTime);
	return timeout || buttonDown.buttonstatus == BUTTON_DOUBLE_CLICK;
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
    delay(10);
    pinMode(IN1MotorPin, INPUT);

    pinMode(IN2MotorPin, OUTPUT);
    digitalWrite(IN2MotorPin, LOW);
    delay(10);
    pinMode(IN2MotorPin, INPUT);

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
    Serial.print("System idle for (s): ");
    Serial.println(min((millis() - lastMotorActiveTime), (millis() - lastButtonActiveTime)) / 1000);

    Serial.flush();
    delay(50);
    Serial.println("Going to deep sleep");
    esp_deep_sleep_start();
}

void Board::printWakeupReason() const {
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

void Board::onRelease(Button& button) {
    // Serial.print("onRelease: ");
    // Serial.println(button.pin);
	switch (button.buttonstatus)
	{
		case BUTTON_IDLE:
			// Serial.print("Click: ");
			// Serial.println(b.pin);
			button.buttonstatus = BUTTON_CLICK;
			break;
		case BUTTON_HOLD:
			Serial.print("HoldRelease: ");
			// Serial.println(b.pin);
			button.buttonstatus = BUTTON_IDLE;
		default:
			break;
    } 	
}

void Board::onHold(Button& button) {
    // Serial.print("onHold: ");
    // Serial.println(button.pin);
    button.buttonstatus = BUTTON_HOLD;
}

void Board::handleButtonAction() {
	switch (buttonUp.buttonstatus) {
        case BUTTON_HOLD:  
            // Serial.print("Holding Up; Current Voltage: ");
            // Serial.print("Holding Up; New Voltage: ");
            // Serial.println(constrain(currVoltage + motor.getVoltageStep(), 0, motor.getBusVoltage()), 2);
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                motor.setVoltage(IN1MotorPin, motor.getVoltage() + motor.getVoltageStep());
            }
            break;

        case BUTTON_CLICK:  // Click
            // loadCell.tare();
            // motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            // buttonUp.buttonstatus = 0;
            // break;
            lastButtonActiveTime = millis();
            if (!motor.started()) {
                motor.setMotorStartTime();
            }
            motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            delayStartTime = millis();
            waitingAfterClick = true;
			buttonUp.buttonstatus = BUTTON_IDLE;
            break;

		case BUTTON_DOUBLE_CLICK:
            lastButtonActiveTime = millis();
            if (!motor.started()) {
                motor.setMotorStartTime();
            }
			motor.setVoltage(IN1MotorPin, motor.getMaxVoltage(), true);
            delayStartTime = millis();
            waitingAfterClick = true;
			buttonUp.buttonstatus = BUTTON_IDLE;
			break;

        default:
            // No action needed
            break;
    }

    switch (buttonDown.buttonstatus) {
        case BUTTON_HOLD:  
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                // ensure motor doesn't stop when holding down button
                float newVoltage = motor.getVoltage() - motor.getVoltageStep();
                newVoltage = max(newVoltage, motor.getMinVoltage());
                motor.setVoltage(IN1MotorPin, newVoltage);
            }
            break;

        case BUTTON_CLICK:
            lastButtonActiveTime = millis();
            if (motor.getVoltage() > 0) {
                motor.reset();
                motor.setVoltage(IN1MotorPin, 0);
                if (loadCellPresent) {
                    loadCell.reset();
                }
            }
            else if (batteryMonitorPresent) {
                playBatteryLevelChime();
            }
            buttonDown.buttonstatus = BUTTON_IDLE;
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
            Serial.println("Voltage > 0");
			lastMotorActiveTime = millis();
            if (loadCellPresent){
                loadCell.update();
            }
			if (shouldStopMotor()) {
				motor.setVoltage(IN1MotorPin, 0);
                motor.reset();
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