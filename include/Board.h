#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <Button.h>
#include "LoadCell.h"
#include "Motor.h"
#include "Battery.h"

enum ButtonStatus {
    BUTTON_IDLE = 0,
    BUTTON_HOLD = 1,
    BUTTON_CLICK = 2,
    BUTTON_DOUBLE_CLICK = 3,
};

class Board {
public:
    Board(float calibrationFactor);
    void setup();
    void updateButtons();
    bool shouldSleep();  
    void enterDeepSleep();
    
    Motor& getMotor();
    LoadCell& getLoadCell();
    Button& getButtonUp();
    Button& getButtonDown();

    void handleButtonAction();
    bool shouldStopMotor();
    void processFeedingCycle();

private:
    // GPIO Pins
    static const int IN1MotorPin = 44; //change to 44 if motor runs in reverse
    static const int IN2MotorPin = 7; //change to 7 if motor runs in reverse
    static const int buttonUpPin = 5; //labeled as D4 on the silkscreen for xiao
    static const int buttonDownPin = 6; //labeled as D5 on the slikscreen for xiao
    static const int HX_DOUT = 9; //hx711, labeled as D10 on the silkscreen for xiao
    static const int HX_CLK = 8; //hx711, labeled as D9 on the silkscreen for xiao
    static const int batteryPin = A2; // ADC input from voltage divider
    static const gpio_num_t HX711CLK = GPIO_NUM_8;
    static const gpio_num_t WAKEUP_GPIO = GPIO_NUM_5;

    // Components
    Motor motor;
    LoadCell loadCell;
    Battery battery;
    Button buttonUp;
    Button buttonDown;

    // Timing tracking
    unsigned long lastMotorActiveTime = 0;
    unsigned long lastButtonActiveTime = 0;
    
    // loadCell
    bool loadCellPresent;
    unsigned long delayStartTime = 0;
    bool waitingAfterClick = false;
    const unsigned long delayAfterClick = 1000;

    // batteryMonitor
    bool batteryMonitorPresent;
    bool batteryWarning = false;
    bool batteryCritical = false;

    // Double click tracking
    bool pendingClickUp = false;
    bool pendingClickDown = false;
	unsigned long clickUpStartTime = 0;
    unsigned long clickDownStartTime = 0; 
    const unsigned long doubleClickInterval = 400;

    // Config
    const int sleepTimeoutTime = 30000; // inactive time before system goes to sleep

    // Internal callbacks
    static void onRelease(Button& button);
    static void onHold(Button& button);

    // Detect components
    bool isBatteryMonitorConnected(int minValidAdc = 600);
    bool isHX711Connected(unsigned long timeout = 1000);

    // Double click
    void handleDoubleClick(Button& button, bool& pendingClick, unsigned long& clickStartTime);

    // Chimes
    void playBatteryLevelChime();
    void playLowBatteryChime();
    void playStartupChime();
    void playDeepSleepChime();
    void printWakeupReason() const;
};

#endif // BOARD_H
