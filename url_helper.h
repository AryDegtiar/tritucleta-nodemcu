#pragma once
#include <Arduino.h>

void urlStoreBegin();

// índice actual: 0=LOCAL, 1=DEV, 2=PROD
uint8_t urlGetIndex();

// url real (http://...)
const char* urlGet();

// label corto REAL según config
const char* urlGetLabel(uint8_t idx);

// set y persistir
void urlSetIndex(uint8_t idx);

// cantidad de endpoints
uint8_t urlCount();
