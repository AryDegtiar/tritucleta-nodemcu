#pragma once
#include <Arduino.h>

extern volatile float CAL_FACTOR;

void setupHX711();
void updateWeight();

void doTare();
void applyCalFactor(float cf);

// Calibración por gramos (guiada)
bool isCalibrating();
void enterCalibrationMode();
void calibrationAdjust(int16_t steps);
bool calibrationConfirm();

float readWeightForPost(uint8_t samples = 20);
