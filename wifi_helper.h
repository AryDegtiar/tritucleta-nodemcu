#pragma once
#include <Arduino.h>

void setupWiFi(const char* ssid, const char* pass);
void updateWiFiStatus();
bool wifiIsConnected();
