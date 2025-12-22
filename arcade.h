#pragma once
#include <Arduino.h>

void setupArcade();
void updateArcade();

// Hook llamado desde hx711_helper.cpp (si lo estás usando)
void hx711Yield();
