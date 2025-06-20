#include <Arduino.h>
#include <Button.h> //uh, buttons
#include "driver/rtc_io.h" //deepsleep
#include "LoadCell.h" //loadcell setting
#include "Motor.h" //Motor setting

//deepsleep/wake defines, using EXT0 cause internal pullup power domain is needed.
#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO)  // 2 ^ GPIO_NUMBER in hex
#define WAKEUP_GPIO              GPIO_NUM_5     // Only RTC IO are allowed

// //GPIOs for R&D Supermini, don't use.
// const int IN1MotorPin = 13; //change to 12 if motor runs in reverse
// const int IN2MotorPin = 12; //change to 13 if motor runs in reverse
// const int buttonUPpin = 5;
// const int buttonDOWNpin = 4; 
// const int HX_DOUT = 10; //hx711
// const int HX_CLK = 11; //hx711
// #define HX711CLK GPIO_NUM_11 //define HX_CLK this way as well for deepsleep pullup (hx711 in sleep)

//GPIOs (note these don't match up with the silkscreen digital callouts on the XIAO board, see schematic)
const int IN1MotorPin = 44; //change to 44 if motor runs in reverse
const int IN2MotorPin = 7; //change to 7 if motor runs in reverse
const int buttonUPpin = 5; //labeled as D4 on the silkscreen for xiao
const int buttonDOWNpin = 6; //labeled as D5 on the slikscreen for xiao
const int HX_DOUT = 9; //hx711, labeled as D10 on the silkscreen for xiao
const int HX_CLK = 8; //hx711, labeled as D9 on the silkscreen for xiao
#define HX711CLK GPIO_NUM_8 //define HX_CLK this way as well for deepsleep pullup (hx711 in sleep)

//Loop/Sleep Variables
u_long PrevUpdateTime = 0; 
u_long SleepTimeoutTracker = 0;
//user config
int SleepTimeoutTime = 60000; //milliseconds, controls how long before device deep sleep when there's no weight, button presses, etc.
int SleepErrorTimeoutTime = 300000;  //sleep regardless of weight after 5 minutes of no button pushes. Used to cover for any weird scale behavior.
float WeightTimeoutMinimumBound = 2.6; //minimum weight to stay awake
float WeightTimeoutMaxiumumBound = 30.0; //maximum bean weight expected in slowfeeder, keeps awake.

//HX711 Scale
// HX711 scale;
// float calibration_factor = -2650; //put in your calibration factor from calibration code
// float scale_reading = 0;

Motor motor(IN1MotorPin, IN2MotorPin);

// const unsigned long maxMotorRunTime = 10000; //milliseconds to run motor
float calibrationFactor = 300;
LoadCell loadCell(HX_DOUT, HX_CLK, calibrationFactor);

//LastButtonPress
u_long LastButtonPress = 0;
int ButtonRunTimeout = 5000; //milliseconds to ignore motor shutoff conditions due to button press

//Button configs
Button buttonUP(buttonUPpin, BUTTON_PULLDOWN, true, 50);
Button buttonDOWN(buttonDOWNpin, BUTTON_PULLDOWN, true, 50);

unsigned long tareDelayStartTime = 0;
bool waitingAfterClick = false;
//Button Callbacks
// void onPress(Button& b){
// 	Serial.print("onPress: ");
// 	Serial.println(b.pin);
// 	//digitalWrite(13,HIGH);
// }

// void onClick(Button& b){
// 	Serial.print("onClick: ");
// 	Serial.println(b.pin);
// 	//digitalWrite(13,HIGH);
// }

void onRelease(Button& b) {
    Serial.print("onRelease: ");
    Serial.println(b.pin);
    if(b.buttonstatus == 0){
        Serial.print("Click: ");
        Serial.println(b.pin);
        b.buttonstatus = 2;
    }
    else if(b.buttonstatus == 1){
        Serial.print("HoldRelease: ");
        Serial.println(b.pin);
        b.buttonstatus = 0;
    }
    else {
        Serial.print("Unhandled pin triggered - release");
        Serial.println(b.buttonstatus);
    }
}

void onHold(Button& b){
    Serial.print("Held: ");
    Serial.println(b.pin);
    b.buttonstatus = 1;
}

//deepsleep related
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
        default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
        }
}

void setup() {
    //start serial for ButtonDebug
    Serial.begin(115200);
    delay(10);
    Serial.println("Serial Start");

    // motor setup
    motor.setup();
    Serial.println("Setting up motor");

    // laod cell set up 
    loadCell.setup();
    Serial.println("Setting up loadCell");
    print_wakeup_reason();

    // Assign callback functions
    //buttonUP.pressHandler(onPress);
    //buttonUP.clickHandler(onClick);
    buttonUP.releaseHandler(onRelease);
    buttonUP.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

    //buttonDOWN.pressHandler(onPress);
    //buttonDOWN.clickHandler(onClick);
    buttonDOWN.releaseHandler(onRelease);
    buttonDOWN.holdHandler(onHold, 1000); // must be held for at least 1000 ms to trigger

    //Methods to prevent scale taring before button release. 
    // Might be worth pairing with zero factor to create robust detection of unit resting on a counter.
    Serial.println(digitalRead(buttonUPpin));
    Serial.println(digitalRead(buttonDOWNpin));
    while(digitalRead(buttonUPpin) != LOW || digitalRead(buttonDOWNpin) != LOW){ 
        delay(500);
        Serial.println("Button pressed, delay startup tare");
    } 

    motor.makeNoise(1000, 150);
    motor.makeNoise(1300, 150);
    motor.makeNoise(1600, 150);
    motor.makeNoise(2000, 200); 

    //reset status, button lib has startup bugs - will always trigger a release and click mode per button when process is called the first time. 
    // Also cover button pressed reset from scale method
    buttonUP.process();
    buttonDOWN.process();
    buttonUP.buttonstatus = 0;
    buttonDOWN.buttonstatus = 0;
    Serial.println(buttonUP.buttonstatus);
    Serial.println(buttonDOWN.buttonstatus);

    //set time trackers to current time
    PrevUpdateTime = millis();
    SleepTimeoutTracker = PrevUpdateTime;
}

unsigned long buttonHighStartTime = 0;
unsigned long buttonHighDuration = 0;
bool buttonWasHigh = false;

void loop() {
    // int toUpdate = ((millis() - PrevUpdateTime) > MotorUpdateTime);
    if(((millis() - SleepTimeoutTracker) > SleepTimeoutTime) || ((millis() - LastButtonPress) > SleepErrorTimeoutTime)){
        //put HX711 to sleep
        motor.makeNoise(1800, 150);
        motor.makeNoise(1400, 150);
        motor.makeNoise(1000, 150);
        motor.makeNoise(600, 300);
        rtc_gpio_pulldown_dis(HX711CLK);
        rtc_gpio_pullup_en(HX711CLK);
        //begin going to sleep :)
        esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);  //1 = High, 0 = Low
        // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level during deepsleep.
        rtc_gpio_pullup_dis(WAKEUP_GPIO);
        rtc_gpio_pulldown_en(WAKEUP_GPIO);
        Serial.println("Going to sleep now");
        esp_deep_sleep_start();
        Serial.println("If you see this, no you didn't");
    }

    //check button states
    buttonUP.process();
    buttonDOWN.process();

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
    switch (buttonUP.buttonstatus) {
        case 1:  // Hold
            if (motor.getVoltage() == 0) {
                motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            }
            // Serial.print("Holding Up; Current Voltage: ");
            // Serial.print("Holding Up; New Voltage: ");
            // Serial.println(constrain(currVoltage + motor.getVoltageStep(), 0, motor.getBusVoltage()), 2);
            motor.setVoltage(IN1MotorPin, motor.getVoltage() + motor.getVoltageStep());
            break;
        

        case 2:  // Click
            // loadCell.tare();
            // motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            // buttonUP.buttonstatus = 0;
            // break;
            motor.setVoltage(IN1MotorPin, motor.getMinVoltage());
            tareDelayStartTime = millis();
            waitingAfterClick = true;
            buttonUP.buttonstatus = 0;
            break;

        default:
            // No action needed
            break;
    }

    switch (buttonDOWN.buttonstatus) {
        case 1:  // Hold
            if (motor.getVoltage() > 0) {
                // Serial.print("Holding down; MotorVoltage: ");
                // Serial.println(constrain(currVoltage - MotorVoltageStep, 0, BusVoltage), 2);
                
                // ensure motor doesn't stop when holding down button
                float newVoltage = motor.getVoltage() - motor.getVoltageStep();
                newVoltage = max(newVoltage, motor.getMinVoltage());
                motor.setVoltage(IN1MotorPin, newVoltage);
            }
            break;

        case 2:  // Click
            motor.setVoltage(IN1MotorPin, 0);
            buttonDOWN.buttonstatus = 0;
            loadCell.tare();
            break;

        default:
            // No action needed
            break;
    }

    if (waitingAfterClick) {
        if(millis() - tareDelayStartTime >= 1000) {
            // reset to wait afte click
            waitingAfterClick = false;
            tareDelayStartTime = millis();
        }
    }
    else {
        if (motor.getVoltage() > 0) {
            loadCell.update();
            if (loadCell.shouldStopMotor()) {
                motor.setVoltage(IN1MotorPin, 0);
                loadCell.tare();
            }
        }   
    }
    delay(10);
}