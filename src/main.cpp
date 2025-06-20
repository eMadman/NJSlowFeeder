#include <Arduino.h>
#include <Button.h> //uh, buttons
#include "driver/rtc_io.h" //deepsleep
#include "LoadCell.h" //loadcell setting
#include "Motor.h" //Motor setting
#include "Board.h" //Board setting

//deepsleep/wake defines, using EXT0 cause internal pullup power domain is needed.
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_5     // Only RTC IO are allowed

float calibrationFactor = 300;
Board board(calibrationFactor);

void setup() {
    board.setup();
}

unsigned long buttonHighStartTime = 0;
unsigned long buttonHighDuration = 0;
// bool buttonWasHigh = false;

void loop() {
    if (board.shouldSleep()) {
        board.enterDeepSleep();
    }
    //check button states
    board.updateButtons();
    // // Track how long buttonUPpin stays LOW
    // if (digitalRead(buttonUPpin) == HIGH) {
    //     if (!buttonWasHigh) {
    //         buttonHighStartTime = millis();  // just went HIGH
    //         buttonWasHigh = true;
    //     }
    //     // else: do nothing, keep waiting for LOW
    // } else {
    //     if (buttonWasHigh) {
    //         buttonHighDuration = millis() - buttonHighStartTime;
    //         Serial.print("Button was HIGH for (ms): ");
    //         Serial.println(buttonHighDuration);
    //         buttonWasHigh = false;
    //     }
    // }
    //time state evaluations
    // if(buttonUP.buttonstatus != 0 || buttonDOWN.buttonstatus != 0){ LastButtonPress = millis();} //track last button active time
    // if(buttonUP.buttonstatus != 0 || buttonDOWN.buttonstatus != 0 || (scale_reading > WeightTimeoutMinimumBound && scale_reading < WeightTimeoutMaxiumumBound)){ SleepTimeoutTracker = millis();} //delay sleep

    //button status evaluations
    board.handleButtonAction();
    board.processFeedingCycle();

    delay(10);
}