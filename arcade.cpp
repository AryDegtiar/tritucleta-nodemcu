#include "arcade.h"

// ===== Config =====
static const uint16_t BLINK_TOTAL_MS    = 1000; // dura 1 segundo total

// ===== Estado interno =====
static bool blinking = false;          // false: LED fijo, true: parpadeando
static bool ledState = true;           // estado actual del LED (HIGH/LOW)
static unsigned long blinkStart = 0;   // inicio del período de parpadeo
static unsigned long lastBlink = 0;    // última vez que alternó el LED

// Debounce de botón
static bool lastStable = HIGH;         // estado estable anterior (INPUT_PULLUP: HIGH = suelto)
static bool lastRead   = HIGH;         // última lectura cruda
static unsigned long lastChange = 0;   // timestamp del último cambio crudo

void setupArcade() {
  pinMode(LED_ARCD, OUTPUT);
  pinMode(PUL_ARCD, INPUT_PULLUP);     // botón a GND

  blinking = false;
  ledState = true;
  digitalWrite(LED_ARCD, HIGH);        // LED encendido fijo al inicio

  lastStable = HIGH;
  lastRead   = HIGH;
  lastChange = millis();

  Serial.println("[Arcade] Inicializado");
}

static bool readButtonEdgeFalling() {
  // Lee crudo (LOW = presionado, HIGH = suelto)
  bool raw = (digitalRead(PUL_ARCD) == LOW) ? LOW : HIGH;
  unsigned long now = millis();

  if (raw != lastRead) {
    lastRead = raw;
    lastChange = now; // posible cambio, empezar a contar debounce
  }

  // Si se mantuvo estable más que DEBOUNCE_MS, confirmamos el cambio
  if ((now - lastChange) >= DEBOUNCE_MS && raw != lastStable) {
    lastStable = raw;
    // Edge FALLING: pasó de HIGH (suelto) a LOW (presionado)
    if (lastStable == LOW) {
      return true; // detectamos un clic (borde de bajada)
    }
  }
  return false;
}

void updateArcade() {
  unsigned long now = millis();

  // 1) Detectar clic (una sola vez) con debounce
  if (readButtonEdgeFalling()) {
    // Evento: botón presionado (no hace falta mantener)
    Serial.println("[Arcade] Pulsador presionado");
    blinking   = true;
    blinkStart = now;
    lastBlink  = now;
    ledState   = false;                 // arrancamos apagando para que se note el parpadeo
    digitalWrite(LED_ARCD, LOW);
  }

  // 2) Lógica de LED
  if (!blinking) {
    // LED fijo encendido
    if (!ledState) {
      ledState = true;
      digitalWrite(LED_ARCD, HIGH);
    } else {
      // aseguramos estado HIGH
      digitalWrite(LED_ARCD, HIGH);
    }
    return;
  }

  // 3) Si está parpadeando, alternar cada BLINK_INTERVAL_MS
  if (now - lastBlink >= BLINK_INTERVAL_MS) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_ARCD, ledState ? HIGH : LOW);
  }

  // 4) Terminar parpadeo al cumplir BLINK_TOTAL_MS y volver a fijo
  if (now - blinkStart >= BLINK_TOTAL_MS) {
    blinking = false;
    ledState = true;
    digitalWrite(LED_ARCD, HIGH);
  }
}
