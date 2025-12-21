#pragma once
#include <Arduino.h>

// ===== TFT ST7735 =====
#define TFT_SCLK          18
#define TFT_MOSI          23
#define TFT_CS            5
#define TFT_DC            17
#define TFT_RST           16
#define TFT_ROTATION      3

// ===== HX711 =====
#define PIN_DT            34
#define PIN_SCK           27

// ===== Botón calibración =====
#define PUL_CAL           19

// ===== Encoder =====
#define ENC_CLK           32
#define ENC_DT            33
#define ENC_SW            26

#define PUL_DEBOUNCE_MS   50
#define INVERT_DIRECTION  1
#define STEPS_PER_NOTCH   4

// ===== Filtros =====
#define MEDIAN_N          5
#define EMA_ALPHA         0.1f
#define DEADBAND_G        0.1f
#define POLL_MS           25

// ===== Calibración =====
#define CAL_STEP_GRAMS    1.0f
#define CAL_STEP_FINE     1.0f
#define CAL_MIN           (-10000.0f)
#define CAL_MAX           ( 10000.0f)

// ===== Arcade =====
#define LED_ARCD          14
#define PUL_ARCD          25

#define BLINK_INTERVAL_MS 200
#define DEBOUNCE_MS       20

// ===== Wi-Fi =====
#define WIFI_SSID     "Servicio de Interpol-2.4GHz"
#define WIFI_PASS     "tegarquevecino"
#define WIFI_CONNECT_TIMEOUT_MS 8000
#define WIFI_RECHECK_MS          2000
