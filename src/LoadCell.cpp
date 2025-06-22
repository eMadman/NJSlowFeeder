#include "LoadCell.h"

LoadCell::LoadCell(int doutPin, int clkPin, float cf, float a)
	: DOUT(doutPin), CLK(clkPin), calibrationFactor(cf), alpha(a) {
		minMotorRunTime = max(minMotorRunTime, sampleInterval * windowSize);
	}


void LoadCell::reset() {
	// Serial.println("Taring load cell");
	weightInd = 0;
	fill(weightWindow.begin(), weightWindow.end(), 0.0f);
	startTime = 0;
	lastRateUpdateTime = 0;
	rateStoppedSince = 0;
	weightStoppedSince = 0;
	previousWeight = 0;
	smoothedRate = 0;
	minWeight = 0; maxWeight = 0;
	weightFlag = false, rateFlag = false;
}

float LoadCell::getScaleWeight() {
	return scale.get_units();
}

void LoadCell::setup() {
	scale.begin(DOUT, CLK);
	scale.set_scale(calibrationFactor);
}

bool LoadCell::started() const {
	return startTime != 0;
}

void LoadCell::startUp() {
	scale.tare(10);
	startTime = millis();
	lastRateUpdateTime = millis();
	previousWeight = getScaleWeight();
}

void LoadCell::update() {
	if (!started()){
		startUp();
		return;
	}
	else if (millis() - lastRateUpdateTime < sampleInterval) { 
		return; 
	}

	float scaleReading = getScaleWeight();
	float dw = abs(scaleReading - previousWeight);
	float dt = (millis() - lastRateUpdateTime) / 1000.0;

	float currRate = dw / dt;

	// EMA smoothing
	smoothedRate = alpha * currRate + (1 - alpha) * smoothedRate;

	// Rolling window to track weights
	weightWindow[weightInd++] = scaleReading;
	weightInd = weightInd % windowSize;

	auto minmax = minmax_element(weightWindow.begin(), weightWindow.end());
	minWeight = *minmax.first;
	maxWeight = *minmax.second;

	for (size_t i = 0; i < weightWindow.size(); ++i) {
		Serial.print(weightWindow[i], 3); 
		if (i < weightWindow.size() - 1) Serial.print(" | ");
	}
	Serial.println();
	Serial.print("Weight diff: ");
	Serial.println(maxWeight - minWeight, 3);
	Serial.print("Smoothed Rate: "); 
	Serial.println(smoothedRate, 3); 
	Serial.println();

	previousWeight = scaleReading;
	lastRateUpdateTime = millis();
}

bool LoadCell::shouldStopMotor() {
	unsigned long elapsed = millis() - startTime;
	if (elapsed < minMotorRunTime){
		return false;
	}
	else if (elapsed > maxMotorRunTime) { 
		Serial.println("Motor stop: max time reached");
		return true; 
	}
	else if (isFeedStopped()) {
		Serial.println("Motor stop: stats condition met");
		return true;
	}
	return false;
}

bool LoadCell::isFeedStopped() {
    if (weightWindow.size() < windowSize) {
        return false;
    }

    unsigned long now = millis();

    // --- Weight check ---
    bool weightCond = (maxWeight - minWeight < weightChangeThreshold);
    if (weightCond) {
        if (weightStoppedSince == 0) weightStoppedSince = now;
    } 
	else {
        weightStoppedSince = 0;
    }

    // --- Rate check ---
    bool rateCond = (smoothedRate < feedRateThreshold);
    if (rateCond) {
        if (rateStoppedSince == 0) rateStoppedSince = now;
    } 
	else {
        rateStoppedSince = 0;
    }

    // Flags for persistent condition 
    bool weightStuck = weightCond && (now - weightStoppedSince >= stopHoldTime);
    bool rateStuck   = rateCond   && (now - rateStoppedSince >= stopHoldTime);

    // Final stop condition
	if (weightCond) { Serial.println("Weight condition met"); }
	if (rateCond) { Serial.println("Rate condition met"); }
	if (weightStuck) { Serial.println("Weight condition stuck"); }
	if (rateStuck) { Serial.println("Rate condition stuck"); }

	return (weightCond && rateCond) || weightStuck || rateStuck;
}