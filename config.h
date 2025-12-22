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

// ===== Calibración por defecto =====
// 115000 raw = 279 g
#define DEFAULT_CAL_RAW     488513.0f
#define DEFAULT_CAL_GRAMS   1186.0f
#define DEFAULT_CAL_FACTOR  (DEFAULT_CAL_RAW / DEFAULT_CAL_GRAMS)

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

// ===== Backend (3 endpoints) =====
// OJO: el POST actual soporta http:// (no https://)
#define URL_LOCAL "http://192.168.1.62:5000/api/weight/live"   
#define URL_DEV   "https://boceto-tritucleta-alealcontador.replit.app/api/weight/live"    
#define URL_PROD  "https://www.circoreciclado.com/api/weight/live"  

// Combo para entrar al menú de endpoints (ENC_SW + PUL_CAL)
#define URL_MENU_HOLD_MS  400

#define HTTP_TIMEOUT_MS 5000

// ===== POST samples HX711 =====
#define POST_SAMPLES 10   // recomendado 8–15 (30 tarda MUCHO en 10Hz)

// =======================================================
// ===================== COLORES =========================
// =======================================================
// ST7735 usa RGB565 (16-bit). Convertimos desde RGB(0-255).
#ifndef RGB565
  #define RGB565(r,g,b) ( (uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)) )
#endif

// Semánticos
#ifndef COLOR_OK
  #define COLOR_OK    RGB565(0, 255, 0)      // verde
#endif

#ifndef COLOR_WARN
  #define COLOR_WARN  RGB565(255, 200, 0)    // amarillo/anaranjado visible
#endif

#ifndef COLOR_ERROR
  #define COLOR_ERROR RGB565(255, 0, 0)      // rojo
#endif

#ifndef COLOR_INFO
  #define COLOR_INFO  RGB565(0, 200, 255)    // celeste
#endif

#ifndef COLOR_TEXT
  #define COLOR_TEXT  RGB565(255, 255, 255)  // blanco
#endif

#ifndef COLOR_BG
  #define COLOR_BG    RGB565(0, 0, 0)        // negro
#endif
