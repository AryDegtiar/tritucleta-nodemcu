#include "url_helper.h"
#include "config.h"
#include <Preferences.h>

static const char* const URLS[] = {
  URL_LOCAL,
  URL_DEV,
  URL_PROD
};

static const char* const LABELS[] = {
  "LOCAL",
  "DEV",
  "PROD"
};

static Preferences prefs;
static uint8_t currentIdx = 0;

static uint8_t clamp(uint8_t v) {
  if (v > 2) return 0;
  return v;
}

void urlStoreBegin() {
  prefs.begin("tritucleta", false);
  currentIdx = clamp(prefs.getUChar("url_idx", 0));
}

uint8_t urlGetIndex() {
  return currentIdx;
}

const char* urlGet() {
  return URLS[currentIdx];
}

const char* urlGetLabel(uint8_t idx) {
  return LABELS[clamp(idx)];
}

void urlSetIndex(uint8_t idx) {
  currentIdx = clamp(idx);
  prefs.putUChar("url_idx", currentIdx);
}

uint8_t urlCount() {
  return 3;
}
