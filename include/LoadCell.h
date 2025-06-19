#ifndef LOADCELL_H
#define LOADCELL_H

#include <Arduino.h>
#include "HX711.h"
#include <array>

class LoadCell {
public:
	LoadCell(int doutPin, int clkPin, float calibrationFactor, float alpha = 0.3, float thresholdMultiplier = 0.5);

	void setup();
	void update();               
	void tare();
	bool isFeedStopped();        
	float getWeight();

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
	float feedRateStdDev = 0;
	float dynamicThreshold = 0.01;
	float thresholdMultiplier;

	// // Variance tracking (Welford)
	// float mean = 0, M2 = 0;
	// int count = 0;

	// Timing
	unsigned long lastRateUpdate = 0;
	unsigned long statsStart = 0;
	// const unsigned long sampleInterval = 200;
	const unsigned long sampleInterval = 500;
	// const unsigned long statsWindow = 5000;

	// Feed stop detection
	unsigned long stoppedSince = 0;
	const unsigned long stopHoldTime = 5000;

	static const int windowSize = 10;
	std::array<float, windowSize> feedRateWindow;
	std::array<float, windowSize> weightWindow;

	// unsigned long statsResetInterval = 5000; // 5 sec
	// unsigned long lastStatsReset = 0;
};

#endif