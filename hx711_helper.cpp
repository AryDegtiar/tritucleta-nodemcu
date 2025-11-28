#include "config.h"
#include "hx711_helper.h"
#include "display.h"
#include <HX711.h>

static HX711 scale;
volatile float CAL_FACTOR = 360.0f;

enum Mode : uint8_t { MODE_NORMAL=0, MODE_CALIBRATING=1 };
static Mode mode = MODE_NORMAL;

static float emaWeight = NAN;
static float lastShown = NAN;
static float calibTarget = 0.0f;

// ==========================================================
// ---------------------- FILTROS ---------------------------
// ==========================================================

static float readMedianUnits(uint8_t n) {
  if (n < 1) n = 1; 
  if (n > 7) n = 7;

  float v[7];
  for (uint8_t i = 0; i < n; ++i)
    v[i] = scale.get_units(1);

  // ordenamiento por inserción (pequeño y rápido)
  for (uint8_t i = 1; i < n; ++i) {
    float k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = k;
  }
  return v[n/2];
}

static bool filteredRead(float &outGrams) {
  float m = readMedianUnits(MEDIAN_N);

  // Filtro EMA
  if (isnan(emaWeight)) emaWeight = m;
  else                  emaWeight = EMA_ALPHA * m + (1.0f - EMA_ALPHA) * emaWeight;

  float rounded = roundf(emaWeight * 10.0f) / 10.0f;

  // Deadband
  if (isnan(lastShown) || fabsf(rounded - lastShown) >= DEADBAND_G) {
    lastShown = rounded;
    outGrams = rounded;
    return true;
  }
  return false;
}

// ==========================================================
// ---------------- CONFIGURACIÓN / TARE ---------------------
// ==========================================================

void applyCalFactor(float cf) {
  if (cf < CAL_MIN) cf = CAL_MIN;
  if (cf > CAL_MAX) cf = CAL_MAX;

  CAL_FACTOR = cf;
  scale.set_scale(CAL_FACTOR);

  char msg[32];
  snprintf(msg, sizeof(msg), "CF=%.2f", CAL_FACTOR);
  showStatus(msg, COLOR_WARN);
}

void doTare() {
  showStatus("TARE...", COLOR_WARN);
  scale.set_scale(1.0f);
  scale.tare(20);
  scale.set_scale(CAL_FACTOR);

  emaWeight = NAN; 
  lastShown = NAN;

  showStatus("OK (0 g)", COLOR_OK);
}

void setupHX711() {
  scale.begin(PIN_DT, PIN_SCK);

  CAL_FACTOR = 1.0f;

  scale.set_scale(1.0f);
  scale.tare(20);
  scale.set_scale(CAL_FACTOR);

  emaWeight = NAN; 
  lastShown = NAN;

  showStatus("HX711 listo", COLOR_OK);
}

// ==========================================================
// ---------------------- UPDATE PESO ------------------------
// ==========================================================

void updateWeight() {
  static unsigned long last = 0;
  if (millis() - last < POLL_MS) return;
  last = millis();

  if (!scale.is_ready()) return;

  float g;
  if (filteredRead(g)) {
    showWeight(g);

    if (mode == MODE_CALIBRATING) {
      char msg[32];
      snprintf(msg, sizeof(msg), "Objetivo: %.1f g", calibTarget);
      showStatus(msg, COLOR_WARN);
    }
  }
}

bool isCalibrating() { 
  return mode == MODE_CALIBRATING; 
}

// ==========================================================
// ---------------------- CALIBRACIÓN ------------------------
// ==========================================================

void enterCalibrationMode() {

  // Pre-lecturas para estabilizar el filtro
  float g;
  for (int i = 0; i < 4; ++i) { 
    filteredRead(g); 
    delay(10); 
  }

  // *** ATENCIÓN: Como pediste ***
  // El objetivo arranca SIEMPRE en 0 gramos.
  calibTarget = 0.0f;

  mode = MODE_CALIBRATING;

  showStatus("Objetivo: 0 g", COLOR_WARN);
}

void calibrationAdjust(int16_t steps) {
  if (mode != MODE_CALIBRATING || steps == 0) return;

  calibTarget += steps * CAL_STEP_GRAMS;
  if (calibTarget < 0.0f) calibTarget = 0.0f;

  char msg[32];
  snprintf(msg, sizeof(msg), "Objetivo: %.1f g", calibTarget);
  showStatus(msg, COLOR_WARN);
}

bool calibrationConfirm() {
  if (mode != MODE_CALIBRATING) return false;
  if (calibTarget <= 0.0f) {
    showStatus("Error: gramos=0", COLOR_ERROR);
    return false;
  }

  // Tomamos lecturas crudas
  const int samples = 25;
  long rawSum = 0;

  for (int i = 0; i < samples; ++i) {
    rawSum += scale.read_average(1);
    delay(8);
  }

  long rawAvg = rawSum / samples;
  long net = rawAvg - scale.get_offset();

  if (labs(net) < 50) {
    showStatus("Coloque peso real", COLOR_ERROR);
    return false;
  }

  float newCF = (float)net / calibTarget;

  applyCalFactor(newCF);

  mode = MODE_NORMAL;

  emaWeight = NAN;
  lastShown = NAN;

  showStatus("CAL OK", COLOR_OK);
  return true;
}
