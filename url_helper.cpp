#include "url_helper.h"
#include "config.h"

#include <Preferences.h>

// 0=local, 1=dev, 2=prod
static const char* const URLS[] = {
  URL_LOCAL,
  URL_DEV,
  URL_PROD
};

static const char* const LABELS[] = {
  "localhost",
  "develop",
  "production"
};

static Preferences g_pref;
static bool g_inited = false;
static uint8_t g_idx = 0;

static uint8_t clampIdx(int v) {
  if (v < 0) return 0;
  uint8_t max = (uint8_t)(sizeof(URLS) / sizeof(URLS[0]) - 1);
  if ((uint8_t)v > max) return max;
  return (uint8_t)v;
}

void urlStoreBegin() {
  if (g_inited) return;

  g_pref.begin("tritucleta", false);

  uint32_t stored = g_pref.getUInt("url_idx", 0);
  g_idx = clampIdx((int)stored);

  g_inited = true;

  Serial.print("[URL] idx boot = ");
  Serial.print(g_idx);
  Serial.print(" (");
  Serial.print(urlGetLabel(g_idx));
  Serial.print(") -> ");
  Serial.println(urlGet());
}

uint8_t urlGetIndex() {
  return g_idx;
}

const char* urlGet() {
  return URLS[g_idx];
}

const char* urlGetLabel(uint8_t idx) {
  idx = clampIdx(idx);
  return LABELS[idx];
}

void urlSetIndex(uint8_t idx) {
  idx = clampIdx(idx);
  if (idx == g_idx) return;

  g_idx = idx;
  if (g_inited) {
    g_pref.putUInt("url_idx", (uint32_t)g_idx);
  }

  Serial.print("[URL] set idx = ");
  Serial.print(g_idx);
  Serial.print(" (");
  Serial.print(urlGetLabel(g_idx));
  Serial.print(") -> ");
  Serial.println(urlGet());
}

uint8_t urlCount() {
  return (uint8_t)(sizeof(URLS) / sizeof(URLS[0]));
}
