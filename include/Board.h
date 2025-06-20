#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <Button.h>
#include "LoadCell.h"
#include "Motor.h"

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
    void processFeedingCycle();

private:
    // GPIO Pins
    static const int IN1MotorPin = 44; //change to 44 if motor runs in reverse
    static const int IN2MotorPin = 7; //change to 7 if motor runs in reverse
    static const int buttonUPpin = 5; //labeled as D4 on the silkscreen for xiao
    static const int buttonDOWNpin = 6; //labeled as D5 on the slikscreen for xiao
    static const int HX_DOUT = 9; //hx711, labeled as D10 on the silkscreen for xiao
    static const int HX_CLK = 8; //hx711, labeled as D9 on the silkscreen for xiao
    static const gpio_num_t HX711CLK = GPIO_NUM_8;
    static const gpio_num_t WAKEUP_GPIO = GPIO_NUM_5;

    // Components
    Motor motor;
    LoadCell loadCell;
    Button buttonUP;
    Button buttonDOWN;

    // Timing tracking
    unsigned long lastMotorActiveTime;
    unsigned long tareDelayStartTime;
    bool waitingAfterClick;

    // Config
    const int sleepTimeoutTime = 30000;

    // Internal callbacks
    static void onRelease(Button& b);
    static void onHold(Button& b);

    void playStartupChime();
    void playDeepSleepChime();
    void printWakeupReason();
};

#endif // BOARD_H
