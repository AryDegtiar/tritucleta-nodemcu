#include "hx711_helper.h"
#include "config.h"
#include "display.h"
#include <HX711.h>
#include <math.h>

// -----------------------------------------------------------------
// Hook opcional para "ceder" CPU mientras esperamos al HX711.
// Si otro módulo define hx711Yield(), se llamará dentro de waits
// (por ejemplo, para mantener titileo del LED durante un POST).
// -----------------------------------------------------------------
void __attribute__((weak)) hx711Yield() {}

#ifndef POLL_MS
#define POLL_MS 200UL
#endif

// =========================
// Estado interno del HX711
// =========================

HX711 scale;

// Factor de calibración global (declarado en hx711_helper.h)
volatile float CAL_FACTOR = 1.0f;

static bool   g_initialized       = false;
static bool   g_calibrating       = false;
static float  g_lastWeightGrams   = 0.0f;
static float  g_calTargetGrams    = 0.0f;
static unsigned long g_lastPollMs = 0;

// =========================
// Helpers internos
// =========================

static float sanitizeAndRoundUp(float gramsRaw) {
  // Nunca negativos
  if (gramsRaw < 0.0f) gramsRaw = 0.0f;

  const float RES = 0.5f;

  // Convertimos al espacio de 0.5 (por ejemplo, 100.2603 / 0.5 = 200.5206)
  float scaled = gramsRaw / RES;

  // Redondeo al más cercano
  long nearest = lroundf(scaled);

  // Volvemos a gramos
  float quantized = (float)nearest * RES;

  return quantized;
}

static void applyScaleToDevice() {
  scale.set_scale(CAL_FACTOR);
  Serial.print("[HX711] CAL_FACTOR = ");
  Serial.println(CAL_FACTOR, 6);
}

static void pollWeightAndDisplay() {
  if (g_calibrating) {
    // En modo calibración mostramos solo el “peso objetivo”
    g_lastWeightGrams = g_calTargetGrams;
    showWeight(g_calTargetGrams);
    return;
  }

  // Modo normal: leemos usando el factor de escala actual
  float gramsRaw = scale.get_units(5);   // promedio de 5 lecturas
  float grams    = sanitizeAndRoundUp(gramsRaw);

  g_lastWeightGrams = grams;

  showWeight(grams);
  showStatus("Listo", COLOR_OK);

  // Debug opcional
  Serial.print("[HX711] raw(units) = ");
  Serial.print(gramsRaw, 4);
  Serial.print(" -> ");
  Serial.print(grams, 0);
  Serial.println(" g");
}

// =========================
// API expuesta
// =========================

void setupHX711() {
  Serial.println("[HX711] Init...");

  scale.begin(PIN_DT, PIN_SCK);

  // ✅ Arranca con calibración por default (115000 = 279g)
  // Esto NO rompe tu calibración manual: si calibrás, se recalcula y se pisa.
  CAL_FACTOR = (float)DEFAULT_CAL_FACTOR;
  applyScaleToDevice();

  // Tare inicial con la balanza vacía
  scale.tare();
  g_initialized = true;

  Serial.println("[HX711] Tare OK, balanza lista");
  Serial.print("[HX711] Default CAL_FACTOR (boot) = ");
  Serial.println(CAL_FACTOR, 6);
}

void updateWeight() {
  if (!g_initialized) return;

  unsigned long now = millis();
  if (now - g_lastPollMs < POLL_MS) {
    return;  // todavía no toca leer
  }
  g_lastPollMs = now;

  pollWeightAndDisplay();
}

void doTare() {
  if (!g_initialized) return;

  scale.tare();
  g_lastWeightGrams = 0.0f;
  showWeight(0.0f);
  showStatus("TARE", COLOR_OK);

  Serial.println("[HX711] Tare ejecutado");
}

void applyCalFactor(float cf) {
  CAL_FACTOR = cf;
  applyScaleToDevice();
}

// =========================
// Calibración por gramos
// =========================

bool isCalibrating() {
  return g_calibrating;
}

void enterCalibrationMode() {
  if (!g_initialized) return;

  g_calibrating    = true;
  g_calTargetGrams = 0.0f;

  // NO hacemos tare aquí ni tocamos escala.
  // Asumimos que el zero-offset ya fue fijado con la balanza vacía.

  g_lastWeightGrams = 0.0f;
  showWeight(0.0f);
  showStatus("CAL", COLOR_WARN);

  Serial.println("[HX711] >>> MODO CALIBRACION ON <<<");
  Serial.println("[HX711] Flujo: tare vacío -> poné peso -> CAL -> girá hasta gramos reales -> OK.");
}

void calibrationAdjust(int16_t steps) {
  if (!g_calibrating) return;
  if (steps == 0) return;

  g_calTargetGrams += (float)steps;

  if (g_calTargetGrams < 0.0f)     g_calTargetGrams = 0.0f;
  if (g_calTargetGrams > 50000.0f) g_calTargetGrams = 50000.0f;

  g_calTargetGrams = sanitizeAndRoundUp(g_calTargetGrams);

  showWeight(g_calTargetGrams);

  Serial.print("[HX711][CAL] target = ");
  Serial.print(g_calTargetGrams, 0);
  Serial.println(" g");
}

bool calibrationConfirm() {
  if (!g_calibrating) {
    return false;
  }

  if (g_calTargetGrams <= 0.0f) {
    showStatus("CAL FAIL (0g)", COLOR_ERROR);
    Serial.println("[HX711][CAL] FAIL: target <= 0g");
    g_calibrating = false;
    return false;
  }

  // get_value() devuelve (read_average - offset) en “counts crudos”
  long raw = scale.get_value(15);  // promedio de 15 lecturas

  Serial.print("[HX711][CAL] raw promedio (get_value) = ");
  Serial.println(raw);

  if (raw == 0) {
    showStatus("CAL FAIL (raw=0)", COLOR_ERROR);
    Serial.println("[HX711][CAL] FAIL: raw == 0");
    g_calibrating = false;
    return false;
  }

  // CAL_FACTOR puede ser negativo; está bien (depende de la orientación de la celda)
  CAL_FACTOR = (float)raw / g_calTargetGrams;
  applyScaleToDevice();

  g_calibrating = false;

  showStatus("CAL OK", COLOR_OK);

  Serial.println("[HX711] >>> MODO CALIBRACION OFF <<<");
  Serial.print("[HX711] Peso objetivo (g) = ");
  Serial.println(g_calTargetGrams, 0);
  Serial.print("[HX711] CAL_FACTOR final = ");
  Serial.println(CAL_FACTOR, 6);

  return true;
}

float readWeightForPost(uint8_t samples) {
  if (!g_initialized) return 0.0f;
  if (samples == 0) samples = 1;

  // Promedio de N lecturas, esperando is_ready
  double acc = 0.0;
  uint8_t got = 0;

  for (uint8_t i = 0; i < samples; i++) {
    // esperar conversión HX711
    unsigned long start = millis();
    while (!scale.is_ready()) {
      // no bloquear fuerte + permitir que otros módulos hagan "tick" (LED, UI, etc.)
      hx711Yield();
      delay(1);
      if (millis() - start > 300) { // si un sample se cuelga, lo salteamos
        break;
      }
    }
    if (!scale.is_ready()) continue;

    // get_value(1) = (read - offset) en counts
    long raw = scale.get_value(1);
    float gramsRaw = (CAL_FACTOR != 0.0f) ? ((float)raw / CAL_FACTOR) : 0.0f;
    float grams = sanitizeAndRoundUp(gramsRaw);

    acc += grams;
    got++;
  }

  if (got == 0) return 0.0f;
  return (float)(acc / (double)got);
}
