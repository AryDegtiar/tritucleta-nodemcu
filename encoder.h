#pragma once
#include <Arduino.h>
#include "config.h"

// ===== API del encoder (encapsulado, sin side-effects) =====
enum EncoderTurn {
  ENC_NONE  = 0,
  ENC_LEFT  = -1,
  ENC_RIGHT = +1
};

// Inicializa pines e interrupciones del encoder
void setupEncoder();

// Procesa hardware (decoder + botón con debounce) sin bloquear.
// Llamarla en loop() una vez por iteración.
void encoderTick();

// Devuelve una sola muesca si hay (consumiéndola).
EncoderTurn encoderTurn();

// Devuelve y CONSUME todos los pasos acumulados (puede ser >0 o <0).
int16_t encoderSteps();

// Click corto one-shot (true una sola vez por pulsación completa).
bool encoderClick();

// Long-press one-shot al superar hold_ms (default 700 ms).
bool encoderHold(uint16_t hold_ms = 700);

// Estado crudo (true si el botón está presionado ahora).
bool encoderPressed();
