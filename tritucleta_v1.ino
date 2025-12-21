#include "config.h"
#include "display.h"
#include "hx711_helper.h"
#include "encoder.h"
#include "arcade.h"
#include "wifi_helper.h"

// --------- forward declarations ----------
static void initHardware();
static void initWiFi();
static void handleCalibrationButton();
static void handleEncoderTurn();
static void handleEncoderClick();
static void updateUiAndPeripherals();

void setup() {
  Serial.begin(115200);
  initHardware();
  initWiFi();
}

void loop() {
  encoderTick();
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

// Calibración con pulsación larga
static void handleCalibrationButton() {
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

static void updateUiAndPeripherals() {
  static bool prevFlashing = false;

  // 1) avanza el flash
  flashTick();
  bool flashingNow = isFlashing();

  // 2) si terminó el flash JUSTO ahora -> redibujá todo
  if (prevFlashing && !flashingNow) {
    // header
    drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED, WIFI_SSID, nullptr);

    // si tu updateWeight() redraw solo el body/peso, llamalo
    updateWeight();

    // opcional: status “Listo”
    // showStatus("Listo", COLOR_OK);
  }

  prevFlashing = flashingNow;

  // 3) Mientras flashea: NO pises el body, pero mantené WiFi + LED vivos
  if (flashingNow) {
    updateWiFiStatus(); // mantiene estado wifi interno (si lo necesitás)
    updateArcade();     // mantiene ledTick()
    return;
  }

  // 4) Normal
  updateWiFiStatus();
  updateWeight();
  updateArcade();
}

