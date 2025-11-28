#include "encoder.h"
#include "config.h"  

// ===================================================================================
// DECODIFICADOR (Ben Buxton) + ISR en ambos canales A/B (ESP32)
// ===================================================================================

// Acumula micro-pasos (±1 por transición válida). Lo llena la ISR.
static volatile int16_t g_encoderMicroSteps = 0;

// Estado previo A/B (2 bits) y banderas de init
static volatile uint8_t g_prevABState = 0;
static volatile bool    g_decoderInited = false;

// Tabla de transición prev->curr (índice = (prev<<2) | curr)
static const int8_t QDEC_TABLE[16] = {
  // prev=00 -> 00,01,10,11
     0,  +1,  -1,   0,
  // prev=01 -> 00,01,10,11
    -1,   0,   0,  +1,
  // prev=10 -> 00,01,10,11
    +1,   0,   0,  -1,
  // prev=11 -> 00,01,10,11
     0,  -1,  +1,   0
};

static inline uint8_t readAB_inline() {
  uint8_t a = digitalRead(ENC_CLK);
  uint8_t b = digitalRead(ENC_DT);
  return (a << 1) | b;  // 2 bits: A B
}

void IRAM_ATTR encoder_isr() {
  uint8_t curr = readAB_inline();

  if (!g_decoderInited) {
    g_prevABState = curr;
    g_decoderInited = true;
    return;
  }
  if (curr == g_prevABState) return;

  int8_t delta = QDEC_TABLE[(g_prevABState << 2) | curr];
  g_prevABState = curr;

#if INVERT_DIRECTION
  delta = -delta;
#endif

  if (delta != 0) {
    // En Xtensa (ESP32) escribir int16_t es atómico.
    g_encoderMicroSteps += delta;
  }
}

void setupEncoder() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT,  INPUT_PULLUP);
  pinMode(ENC_SW,  INPUT_PULLUP);

  // Inicializar estado antes de habilitar interrupciones
  g_prevABState   = readAB_inline();
  g_decoderInited = true;

  // Capturar TODAS las transiciones
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoder_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_DT),  encoder_isr, CHANGE);
}

// ===================================================================================
// BOTÓN + BUFFERS  (API pasiva, sin efectos laterales)
// ===================================================================================

// Acumula "muescas" refinadas para la API
static int16_t g_notchBuffer = 0;

// Debounce + eventos botón
static unsigned long g_btnLastChangeMs = 0;
static bool          g_btnStableLevel  = HIGH;   // INPUT_PULLUP: HIGH = suelto
static bool          g_btnLastRawLevel = HIGH;
static bool          g_btnArmedClick   = false;  // armado mientras está presionado
static bool          g_evtClickReady   = false;  // one-shot click
static bool          g_evtHoldReady    = false;  // one-shot hold
static unsigned long g_btnPressStartMs = 0;

void encoderTick() {
  // 1) Convertir micro-steps → muescas completas y acumular en buffer
  int16_t localMicro;
  noInterrupts();
  localMicro = g_encoderMicroSteps;
  interrupts();

  if (localMicro >= STEPS_PER_NOTCH) {
    int16_t n = localMicro / STEPS_PER_NOTCH;
    noInterrupts();
    g_encoderMicroSteps -= n * STEPS_PER_NOTCH;
    interrupts();
    g_notchBuffer += n;
  } else if (localMicro <= -STEPS_PER_NOTCH) {
    int16_t n = (-localMicro) / STEPS_PER_NOTCH;
    noInterrupts();
    g_encoderMicroSteps += n * STEPS_PER_NOTCH;
    interrupts();
    g_notchBuffer -= n;
  }

  // 2) Debounce del botón del encoder + armado de eventos
  unsigned long now = millis();
  bool rawLevel = (digitalRead(ENC_SW) == LOW) ? LOW : HIGH;  // LOW = presionado

  if (rawLevel != g_btnLastRawLevel) {
    g_btnLastRawLevel  = rawLevel;
    g_btnLastChangeMs  = now;  // posible cambio: arranca ventana de debounce
  }

  if ((now - g_btnLastChangeMs) >= PUL_DEBOUNCE_MS && rawLevel != g_btnStableLevel) {
    g_btnStableLevel = rawLevel;

    if (g_btnStableLevel == LOW) {
      // Flanco de bajada: presionado
      g_btnArmedClick   = true;      // posible click corto si se suelta antes del hold
      g_btnPressStartMs = now;       // medir duración para hold
      // g_evtHoldReady se dispara al consultar encoderHold()
    } else {
      // Flanco de subida: soltado
      if (g_btnArmedClick) {
        g_evtClickReady = true;      // click corto listo (one-shot)
      }
      g_btnArmedClick   = false;
      g_btnPressStartMs = 0;
      g_evtHoldReady    = false;     // reseteo hold para próxima pulsación
    }
  }
}

// ====================== API PASIVA ======================

EncoderTurn encoderTurn() {
  // Consume de a UNA muesca para evitar que el usuario tenga que hacer loops
  if (g_notchBuffer > 0) { g_notchBuffer--; return ENC_RIGHT; }
  if (g_notchBuffer < 0) { g_notchBuffer++; return ENC_LEFT;  }
  return ENC_NONE;
}

int16_t encoderSteps() {
  int16_t v = g_notchBuffer;
  g_notchBuffer = 0;
  return v;
}

bool encoderClick() {
  if (g_evtClickReady) { g_evtClickReady = false; return true; }
  return false;
}

bool encoderHold(uint16_t hold_ms) {
  if (g_btnStableLevel == LOW && g_btnPressStartMs != 0) {
    unsigned long now = millis();
    if (!g_evtHoldReady && (now - g_btnPressStartMs) >= hold_ms) {
      g_evtHoldReady = true;          // one-shot hasta que se suelte
      return true;
    }
  }
  return false;
}

bool encoderPressed() {
  return (g_btnStableLevel == LOW);
}
