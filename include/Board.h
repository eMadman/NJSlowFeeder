#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>
#include <Button.h>
#include "LoadCell.h"
#include "Motor.h"

class Board {
public:
    Board();
    void setup();
    void loop();

private:
    void handleButtons();
    void checkSleep();
    void goToSleep();
    void playStartupSound();
    void playSleepSound();
    void setupButtons();
    static void onRelease(Button& b);
    static void onHold(Button& b);
    void handleButtonLogic();

    // Pins
    static const int IN1MotorPin = 44;
    static const int IN2MotorPin = 7;
    static const int buttonUPpin = 5;
    static const int buttonDOWNpin = 6;
    static const int HX_DOUT = 9;
    static const int HX_CLK = 8;

    // Sleep
    static const int WAKEUP_GPIO = GPIO_NUM_5;
    static constexpr int SleepTimeoutTime = 60000;
    static constexpr int SleepErrorTimeoutTime = 300000;
    float WeightTimeoutMinimumBound = 2.6;
    float WeightTimeoutMaximumBound = 30.0;

    // Objects
    Motor motor;
    LoadCell loadCell;
    Button buttonUP;
    Button buttonDOWN;

    // State
    unsigned long PrevUpdateTime = 0;
    unsigned long SleepTimeoutTracker = 0;
    unsigned long LastButtonPress = 0;
};

#endif