#include "config.h"
#include "display.h"
#include "hx711_helper.h"
#include "encoder.h"
#include "arcade.h"
#include "wifi_helper.h"
#include "url_helper.h"

// ================= helpers =================
static const char* endpointLabel() {
  // Fuente única de verdad
  return urlGetLabel(urlGetIndex());
}

static void showReadyStatus() {
  char st[24];
  snprintf(st, sizeof(st), "Listo %s", endpointLabel());
  showStatus(st, COLOR_OK);
}

// --------- forward declarations ----------
static void initHardware();
static void initWiFi();
static void handleUrlMenuCombo();
static void enterUrlMenuBlocking();
static void handleCalibrationButton();
static void handleEncoderTurn();
static void handleEncoderClick();
static void updateUiAndPeripherals();

void setup() {
  Serial.begin(115200);
  initHardware();
  urlStoreBegin();   // carga endpoint REAL
  initWiFi();
}

void loop() {
  encoderTick();

  handleUrlMenuCombo();
  handleCalibrationButton();
  handleEncoderTurn();
  handleEncoderClick();
  updateUiAndPeripherals();
}

// ================= init =================
static void initHardware() {
  displayInit();
  setupHX711();
  setupEncoder();
  setupArcade();
  pinMode(PUL_CAL, INPUT_PULLUP);
}

static void initWiFi() {
  drawHeaderWiFi(WIFI_CONNECTING, WIFI_SSID, nullptr);
  setupWiFi(WIFI_SSID, WIFI_PASS);
}

// ================= combo ENC + CAL =================
static void handleUrlMenuCombo() {
  static unsigned long startMs = 0;
  static bool tracking = false;

  bool enc = encoderPressed();
  bool cal = (digitalRead(PUL_CAL) == LOW);

  if (enc && cal) {
    if (!tracking) {
      tracking = true;
      startMs = millis();
    } else if (millis() - startMs >= URL_MENU_HOLD_MS) {
      tracking = false;
      enterUrlMenuBlocking();
      while (encoderClick()) {}
    }
  } else {
    tracking = false;
  }
}

// ================= calibración =================
static void handleCalibrationButton() {
  if (encoderPressed()) return;

  static unsigned long pressStart = 0;
  static bool wasPressed = false;

  bool pressed = (digitalRead(PUL_CAL) == LOW);
  unsigned long now = millis();

  if (pressed && !wasPressed) {
    pressStart = now;
    wasPressed = true;
  }

  if (!pressed && wasPressed) {
    wasPressed = false;
  }

  if (pressed && wasPressed && (now - pressStart >= CAL_HOLD_MS)) {
    wasPressed = false;
    enterCalibrationMode();
  }
}

static void handleEncoderTurn() {
  switch (encoderTurn()) {
    case ENC_RIGHT:
      if (isCalibrating()) calibrationAdjust(+1);
      break;
    case ENC_LEFT:
      if (isCalibrating()) calibrationAdjust(-1);
      break;
    default:
      break;
  }
}

static void handleEncoderClick() {
  if (!encoderClick()) return;

  if (isCalibrating()) {
    calibrationConfirm();
  } else {
    doTare();
  }
}

// ================= menú endpoints =================
static void enterUrlMenuBlocking() {
  const uint8_t n = urlCount();
  const char* items[3];

  for (uint8_t i = 0; i < n; i++) {
    items[i] = urlGetLabel(i);
  }

  int selected = urlGetIndex();

  drawUrlMenu("ENDPOINT", items, n, selected);

  while (true) {
    encoderTick();
    updateWiFiStatus();
    updateArcade();
    flashTick();

    EncoderTurn t = encoderTurn();
    if (t == ENC_RIGHT) {
      selected = (selected + 1) % n;
      drawUrlMenu("ENDPOINT", items, n, selected);
    } else if (t == ENC_LEFT) {
      selected = (selected - 1 + n) % n;
      drawUrlMenu("ENDPOINT", items, n, selected);
    }

    if (encoderClick()) {
      urlSetIndex(selected);   // SET REAL
      flashStart(displayOkColor(), "OK", 600);
      break;
    }

    delay(5);
  }

  drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED,
                 WIFI_SSID, nullptr);
  updateWeight();
  showReadyStatus();   // ← ahora queda fijo
}

// ================= UI NORMAL =================
static void updateUiAndPeripherals() {
  static bool prevFlashing = false;

  flashTick();
  bool flashingNow = isFlashing();

  // terminó flash → redraw completo
  if (prevFlashing && !flashingNow) {
    drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED,
                   WIFI_SSID, nullptr);
    updateWeight();
    showReadyStatus();     // ← SIEMPRE
  }

  prevFlashing = flashingNow;

  // mientras flashea
  if (flashingNow) {
    updateWiFiStatus();
    updateArcade();
    return;
  }

  // loop normal
  updateWiFiStatus();
  updateWeight();
  showReadyStatus();       // ← ESTA ES LA CLAVE
  updateArcade();
}
