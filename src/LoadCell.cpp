#include "LoadCell.h"

LoadCell::LoadCell(int doutPin, int clkPin, float cf, float a, float tm)
	: DOUT(doutPin), CLK(clkPin), calibrationFactor(cf), alpha(a), thresholdMultiplier(tm) {}


void LoadCell::tare() {
	scale.tare(10);
}

void LoadCell::setup() {
	scale.begin(DOUT, CLK);
	scale.set_scale(calibrationFactor);
	scale.tare(10);
	scaleReading = scale.get_units();
	previousWeight = scaleReading;
	lastRateUpdate = millis();
	statsStart = millis();
}

float LoadCell::getWeight() { 
	return scale.get_units(); 
}

void LoadCell::update() {
	if (millis() - lastRateUpdate >= sampleInterval) {
		scaleReading = getWeight();
		float dw = abs(scaleReading - previousWeight);
		float dt = (millis() - lastRateUpdate) / 1000;
		currRate = dw / dt;

		// EMA smoothing
		smoothedRate = alpha * currRate + (1 - alpha) * smoothedRate;




		// Serial.print("Weight: ");
		// Serial.println(scaleReading, 1);

		// Serial.print("Smoothed Rate: ");
		// Serial.println(smoothedRate, 3); 

		// Welford stats
		// count++;
		// float delta = smoothedRate - mean;
		// mean += delta / count;
		// M2 += delta * (smoothedRate - mean);

		// if (millis() - lastStatsReset >= statsResetInterval) {
		// 	// Reset Welford stats
		// 	count = 0;
		// 	mean = 0;
		// 	M2 = 0;
		// 	lastStatsReset = millis();
		// }

		// if (count >= 10) {
		// 	feedRateStdDev = sqrt(M2 / (count - 1));
		// 	dynamicThreshold = feedRateStdDev * thresholdMultiplier;

		// 	Serial.print("Smoothed Rate: ");
		// 	Serial.println(smoothedRate, 3); 
		// 	Serial.print(" | Threshold: ");
		// 	Serial.println(dynamicThreshold, 3);
		// }
		previousWeight = scaleReading;
		lastRateUpdate = millis();
	}
}

bool LoadCell::isFeedStopped() {
	float stopRateThreshold = 0.2;
	if (smoothedRate < stopRateThreshold) {
		if (stoppedSince == 0) stoppedSince = millis();
		if (millis() - stoppedSince >= stopHoldTime) return true;
	} 
	else {
		stoppedSince = 0;
	}
	return false;
}