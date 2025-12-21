#pragma once
#include <Arduino.h>
#include <stdint.h>

// Estado Wifi para header (lo tenés ya en tu proyecto)
enum WifiUiState {
  WIFI_CONNECTING = 0,
  WIFI_CONNECTED,
  WIFI_DISCONNECTED
};

void displayInit();
void drawHeaderWiFi(WifiUiState state, const char* ssid, const char* ip);
void showWeight(float grams);
void showStatus(const char* msg, uint16_t color);

// Flash NO bloqueante (pantalla verde/roja 5s)
void flashStart(uint16_t color, const char* msg, uint16_t durationMs);
void flashTick();
bool isFlashing();

// Colores “seguros” (evitan BGR/RGB raros)
uint16_t displayOkColor();
uint16_t displayErrColor();
