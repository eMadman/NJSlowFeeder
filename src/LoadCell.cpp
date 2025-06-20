#include "LoadCell.h"

LoadCell::LoadCell(int doutPin, int clkPin, float cf, float a)
	: DOUT(doutPin), CLK(clkPin), calibrationFactor(cf), alpha(a) {}


void LoadCell::tare() {
	// Serial.println("Taring load cell");
	weightInd = 0;
	fill(weightWindow.begin(), weightWindow.end(), 0.0f);
	motorStartTime = 0;
	lastRateUpdateTime = 0;
	stoppedSince = 0;
	previousWeight = 0;
	scaleReading = 0;
	currRate = 0;
	smoothedRate = 0;
	minWeight = 0; maxWeight = 0;
	weightFlag = false, rateFlag = false;
	scale.tare(10);
}

float LoadCell::getScaleWeight() {
	return scale.get_units();
}

void LoadCell::setup() {
	scale.begin(DOUT, CLK);
	scale.set_scale(calibrationFactor);
	tare();
}

void LoadCell::update() {
	if (motorStartTime == 0){
		startMotorTimer();
		lastRateUpdateTime = millis();
		previousWeight = getScaleWeight();
		return;
	}
	else if (millis() - lastRateUpdateTime < sampleInterval) { 
		return; 
	}

	scaleReading = getScaleWeight();
	float dw = abs(scaleReading - previousWeight);
	float dt = (millis() - lastRateUpdateTime) / 1000.0;

	currRate = dw / dt;

	// EMA smoothing
	smoothedRate = alpha * currRate + (1 - alpha) * smoothedRate;

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

void LoadCell::startMotorTimer() {
	motorStartTime = millis();
}

bool LoadCell::shouldStopMotor() {
	unsigned long elapsed = millis() - motorStartTime;
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
	
	if (maxWeight - minWeight < weightChangeThreshold) 
	{
		Serial.println("weightFlag true");
		weightFlag = true;
	}

	if (smoothedRate < feedRateThreshold) {
		if (stoppedSince == 0) stoppedSince = millis();
		if (millis() - stoppedSince >= stopHoldTime) 
		{
			Serial.println("rateFlag true");
			rateFlag = true;
		}
	}
	else {
		stoppedSince = 0;
	}
	return weightFlag && rateFlag;
}