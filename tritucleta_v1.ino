#include "config.h"
#include "display.h"
#include "hx711_helper.h"
#include "encoder.h"
#include "arcade.h"
#include "wifi_helper.h"

// --------- forward declarations (helpers locales) ----------
static void initHardware();
static void initWiFi();
static void handleCalibrationButton();
static void handleEncoderTurn();
static void handleEncoderClick();
static void updateUiAndPeripherals();

// ==========================================================
void setup() {
  Serial.begin(115200);
  initHardware();   // TFT, HX711, encoder, arcade, botón CAL
  initWiFi();       // UI "Conectando..." y conexión (no bloqueante)
}

void loop() {
  encoderTick();              // API pasiva: lee y acumula eventos del encoder
  handleCalibrationButton();  // botón físico de modo calibración
  handleEncoderTurn();        // giro: ajusta CF si está calibrando
  handleEncoderClick();       // click: confirma CF o hace TARE

  updateUiAndPeripherals();   // WiFi header, peso en pantalla, LED/botón
}

// ================== helpers privados ======================

static void initHardware() {
  displayInit();                 // UI
  setupHX711();                  // balanza
  setupEncoder();                // encoder
  setupArcade();                 // LED/botón arcade (opcional)
  pinMode(PUL_CAL, INPUT_PULLUP);// botón calibración
}

static void initWiFi() {
  // Header inicial: “Conectando…”
  drawHeaderWiFi(WIFI_CONNECTING, WIFI_SSID, nullptr);
  setupWiFi(WIFI_SSID, WIFI_PASS);  // si falla, header queda en rojo; seguimos igual
}

static void handleCalibrationButton() {
  static unsigned long lastCalPressMs = 0;
  const unsigned long now = millis();
  if (digitalRead(PUL_CAL) == LOW && (now - lastCalPressMs) >= PUL_DEBOUNCE_MS) {
    lastCalPressMs = now;
    enterCalibrationMode();
    Serial.println("[MAIN] Calibración: enter");
  }
}

static void handleEncoderTurn() {
  switch (encoderTurn()) {       // ENC_RIGHT / ENC_LEFT / ENC_NONE
    case ENC_RIGHT:
      if (isCalibrating()) calibrationAdjust(+10);
      break;
    case ENC_LEFT:
      if (isCalibrating()) calibrationAdjust(-10);
      break;
    case ENC_NONE:
      break;
  }
}

static void handleEncoderClick() {
  if (!encoderClick()) return;

  if (isCalibrating()) {
    // Nota: mantener la firma bool calibrationConfirm() para evitar choques
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
  updateWiFiStatus();  // solo refresca header si cambió el estado; no bloquea
  updateWeight();      // muestra el peso actual (online/offline)
  updateArcade();      // LED/botón arcade (si lo usás)
}
