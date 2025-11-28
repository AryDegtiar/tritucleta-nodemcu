#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define COLOR_OK     ST77XX_GREEN
#define COLOR_WARN   ST77XX_YELLOW
#define COLOR_ERROR  ST77XX_RED

// ===== API de pantalla =====
void displayInit();

// ===== Header Wi-Fi (nuevo) =====
enum WifiUiState : uint8_t {
  WIFI_CONNECTING = 0,
  WIFI_CONNECTED  = 1,
  WIFI_DISCONNECTED = 2
};

// Dibuja el header con el estado Wi-Fi.
// - state: WIFI_CONNECTING / WIFI_CONNECTED / WIFI_DISCONNECTED
// - ssid : puede ser nullptr si no querés mostrarlo
// - ip   : string con IP o nullptr (solo se usa en CONNECTED)
void drawHeaderWiFi(WifiUiState state, const char* ssid, const char* ip);

// ===== Zona de peso / estado inferior =====
void showWeight(float grams);

// Mensaje de estado (franja inferior)
void showStatus(const char* msg, uint16_t color);