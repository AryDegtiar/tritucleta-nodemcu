#include "config.h"
#include "display.h"
#include "hx711_helper.h"
#include "encoder.h"
#include "arcade.h"
#include "wifi_helper.h"
#include "url_helper.h"

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
  urlStoreBegin();   // ✅ carga idx guardado (NVS)
  initWiFi();
}

void loop() {
  encoderTick();

  // ✅ prioridad: combo ENC + CAL -> menú endpoints (bloqueante)
  handleUrlMenuCombo();

  handleCalibrationButton();
  handleEncoderTurn();
  handleEncoderClick();
  updateUiAndPeripherals();
}

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

// ==========================
// Combo: ENC_SW + PUL_CAL
// ==========================
static void handleUrlMenuCombo() {
  static unsigned long bothStart = 0;
  static bool tracking = false;

  bool encPressed = encoderPressed();                 // ya viene debounceado
  bool calPressed = (digitalRead(PUL_CAL) == LOW);    // pullup

  if (encPressed && calPressed) {
    if (!tracking) {
      tracking = true;
      bothStart = millis();
      return;
    }

    if (millis() - bothStart >= URL_MENU_HOLD_MS) {
      tracking = false;
      bothStart = 0;

      // Entramos al menú (bloquea todo hasta confirmar)
      enterUrlMenuBlocking();

      // al volver, evitamos clicks “fantasma”
      while (encoderClick()) {}
    }
  } else {
    tracking = false;
    bothStart = 0;
  }
}

// Calibración con pulsación larga (si NO estás apretando el encoder)
static void handleCalibrationButton() {
  // ✅ si el encoder está apretado, no queremos que dispare calibración (porque podría ser combo)
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
    wasPressed = false; // evita múltiples entradas
    enterCalibrationMode();
    Serial.println("[MAIN] Calibración: enter (long press)");
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
    case ENC_NONE:
      break;
  }
}

static void handleEncoderClick() {
  if (!encoderClick()) return;

  if (isCalibrating()) {
    if (calibrationConfirm()) {
      Serial.println("[MAIN] Calibración OK");
    } else {
      Serial.println("[MAIN] Calibración FAIL");
    }
  } else {
    doTare();
  }
}

// ==========================
// Menú endpoints (bloqueante)
// ==========================
static void enterUrlMenuBlocking() {
  // armamos items para UI
  const uint8_t n = urlCount();
  const char* items[3] = { nullptr, nullptr, nullptr };
  for (uint8_t i = 0; i < n && i < 3; i++) items[i] = urlGetLabel(i);

  int selected = (int)urlGetIndex();

  // Limpiamos eventos pendientes del encoder
  while (encoderClick()) {}

  drawUrlMenu("endpoints", items, n, selected);

  // Bloquea el “programa normal” hasta confirmar
  while (true) {
    encoderTick();

    // Podés mantener “vivos” wifi + LED mientras elegís
    updateWiFiStatus();
    updateArcade();
    flashTick();

    // girar
    EncoderTurn t = encoderTurn();
    if (t == ENC_RIGHT) {
      selected++;
      if (selected >= (int)n) selected = 0;
      drawUrlMenu("endpoints", items, n, selected);
    } else if (t == ENC_LEFT) {
      selected--;
      if (selected < 0) selected = (int)n - 1;
      drawUrlMenu("endpoints", items, n, selected);
    }

    // confirmar con click
    if (encoderClick()) {
      urlSetIndex((uint8_t)selected);

      // feedback opcional
      flashStart(displayOkColor(), "OK", 700);

      // salir
      break;
    }

    delay(5);
  }

  // Al salir, redibujamos UI normal
  drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED, WIFI_SSID, nullptr);
  updateWeight();
}

static void updateUiAndPeripherals() {
  static bool prevFlashing = false;

  // 1) avanza el flash
  flashTick();
  bool flashingNow = isFlashing();

  // 2) si terminó el flash JUSTO ahora -> redibujá todo
  if (prevFlashing && !flashingNow) {
    drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED, WIFI_SSID, nullptr);
    updateWeight();
  }

  prevFlashing = flashingNow;

  // 3) Mientras flashea: NO pises el body, pero mantené WiFi + LED vivos
  if (flashingNow) {
    updateWiFiStatus();
    updateArcade();
    return;
  }

  // 4) Normal
  updateWiFiStatus();
  updateWeight();
  updateArcade();
}
