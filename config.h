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
#define CAL_HOLD_MS       1000

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

#define DEBOUNCE_MS       20   // solo para "soltar estable" (tap queda instantáneo)
#define LED_BLINK_POST_MS 150  // titilar mientras lee/postea
#define LED_BLINK_ERR_MS  40   // (ya no se usa) antes era blink error
#define LED_ERR_TOTAL_MS  5000 // 5 segundos

// ===== Wi-Fi =====
#define WIFI_SSID     "tritucleta_balanza"
#define WIFI_PASS     "energiarenovable"
#define WIFI_CONNECT_TIMEOUT_MS 8000
#define WIFI_RECHECK_MS          2000

// ===== Backend =====
#define POST_URL "http://192.168.0.22:8080/v1/weight"
#define HTTP_TIMEOUT_MS 5000

// ===== POST samples HX711 =====
#define POST_SAMPLES 10   // recomendado 8–15 (30 tarda MUCHO en 10Hz)
