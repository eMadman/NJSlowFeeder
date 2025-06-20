#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include "HX711.h"
#include <array>

using namespace std;

class LoadCell {
public:
	LoadCell(int doutPin, int clkPin, float calibrationFactor, float alpha = 0.3);

	void setup();
	void update();               
	void tare();
	float getScaleWeight();
	void startMotorTimer();
	bool isFeedStopped();        
	bool shouldStopMotor();

private:
	HX711 scale;
	float calibrationFactor;

	int DOUT, CLK;

	// Weight tracking
	float previousWeight = 0;
	float scaleReading = 0;

	// Feed rate
	float currRate = 0;
	float smoothedRate = 0;
	float alpha;

	// Timing
	unsigned long lastRateUpdateTime = 0;
	const unsigned long sampleInterval = 500;
	const unsigned long minMotorRunTime = 5000; 
	// const unsigned long minMotorRunTime = 0; 
	const unsigned long maxMotorRunTime = 50000;
	unsigned long motorStartTime = 0;

	// Feed stop detection
	unsigned long stoppedSince = 0;
	// No hold after stop criteria met
	const unsigned long stopHoldTime = 0;

	// Stopping thresholds
	const float weightChangeThreshold = 3.5;
	const float feedRateThreshold = 1.0;

	// Params used to stop
	float minWeight;
	float maxWeight;
	bool weightFlag = false;
	bool rateFlag = false;
	static const int windowSize = 10;
	int weightInd = 0;
	array<float, windowSize> weightWindow;
};

#endif