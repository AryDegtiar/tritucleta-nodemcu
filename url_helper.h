#pragma once
#include <Arduino.h>

// Inicializa storage (Preferences). Llamar 1 vez en setup().
void urlStoreBegin();

// Devuelve el índice actual (0..2)
uint8_t urlGetIndex();

// Devuelve la URL actual (http://...)
const char* urlGet();

// Etiqueta para UI ("localhost", "develop", "production")
const char* urlGetLabel(uint8_t idx);

// Cambia y persiste el índice.
void urlSetIndex(uint8_t idx);

// Cantidad de endpoints disponibles.
uint8_t urlCount();
