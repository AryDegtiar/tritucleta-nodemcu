#pragma once
#include <Arduino.h>

// ===== TFT ST7735 80x160 =====
#define TFT_SCLK          18
#define TFT_MOSI          23
#define TFT_CS            5
#define TFT_DC            2
#define TFT_RST           16
#define TFT_ROTATION      3

// ===== HX711 =====
#define PIN_DT            34
#define PIN_SCK           27

#define PUL_CAL           19

// ---- Encoder ----
#define ENC_CLK           32
#define ENC_DT            33
#define ENC_SW            26

#define PUL_DEBOUNCE_MS   50

// --- Comportamiento del encoder ---
#define INVERT_DIRECTION   1    // 0 o 1
#define STEPS_PER_NOTCH    4    // probá 4 (EC11), si aún “salta”, probá 2

// ===== Filtros / muestreo =====
#define MEDIAN_N   5       // ventana un poco mayor
#define EMA_ALPHA  0.1   // alisa más sin volverse “lento”
#define DEADBAND_G 0.1f   // muestra microcambios útiles de 50 mg (si hay ruido bajalo a 0.1)
#define POLL_MS    25      // un poquito más frecuente

// ===== Calibración =====
#define CAL_STEP_GRAMS  1.0f   // pasos de gramos al girar en calibración
#define CAL_STEP_FINE     1.0f   // si ajustás CF directamente con el encoder
#define CAL_MIN           (-10000.0f)
#define CAL_MAX           ( 10000.0f)

// ===== Pulsador Arcade =====
#define LED_ARCD          12
#define PUL_ARCD          21 

#define BLINK_INTERVAL_MS 200
#define DEBOUNCE_MS       20

// ============================================================
// ===============   CONFIG Wi-Fi (ajustá a tu red)  ==========
// ============================================================
// Podés sobreescribirlos antes de compilar, o guardarlos en EEPROM
#define WIFI_SSID     "Servicio de Interpol-2.4GHz"
#define WIFI_PASS     "tegarquevecino"
#define WIFI_CONNECT_TIMEOUT_MS 8000   // tiempo máximo al conectar
#define WIFI_RECHECK_MS          2000  // cada cuánto revalidar conexión
