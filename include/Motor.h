#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor {
public:
	Motor(int in1Pin, int in2Pin);

    void setup();
    void makeNoise(int frequency, int duration);
    float getVoltage() const;
    float getMinVoltage() const;
    float getMaxVoltage() const;
    float getVoltageStep() const;
    // unsigned long getMotorStartTime() const;
    unsigned long getMotorMaxTime() const;

    void setVoltage(int driverPin, float newVoltage);
    // void setMotorStartTime(unsigned long time);

private:
    int in1Pin;
    int in2Pin;

	// unsigned long motorStartTime = 0;
	const unsigned long motorMaxTime = 60000; //milliseconds to run motor

	unsigned long lastMotorUpdate = 0;
	const unsigned long motorUpdateInterval = 500; //milliseconds, controls how frequently the motor voltage gets updated

    float motorVoltage;
	float motorMinVoltage = 2.5; //voltage the system will default to when clicking or holding up. Use 1.5V for 1000Hz analog freq, 2.7 for 20000Hz.
	float motorMaxVoltage = 3.3; //PWM Logic Level
	float motorVoltageStep = 0.1; //voltage the motor will step up per MotorUpdateTime interval when a button is held  Use 0.2 for 1000Hz, 0.1 for 20000Hz
	int motorPWMFrequency = 20000; //motor PWM frequency, 20000 you can't hear, 1000 has more granular range.
};

#endif