#include "config.h"      // pines TFT y ROTATION
#include "display.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Pines (desde config.h con fallback)
#ifndef TFT_SCLK
  #define TFT_SCLK   18
#endif
#ifndef TFT_MOSI
  #define TFT_MOSI   23
#endif
#ifndef TFT_CS
  #define TFT_CS      5
#endif
#ifndef TFT_DC
  #define TFT_DC      2
#endif
#ifndef TFT_RST
  #define TFT_RST     16
#endif
#ifndef TFT_ROTATION
  #define TFT_ROTATION 3
#endif

static Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ===== Layout =====
static const int HEADER_H = 18;

void displayInit() {
  SPI.begin(TFT_SCLK, -1 /*MISO no usado*/, TFT_MOSI, TFT_CS);
  tft.initR(INITR_MINI160x80);   // 80x160
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
}

// ==== HEADER WIFI ====
void drawHeaderWiFi(WifiUiState state, const char* ssid, const char* ip) {
  // Limpiar franja superior
  tft.fillRect(0, 0, 160, HEADER_H, ST77XX_BLACK);
  tft.setTextSize(1);

  // Título “WiFi”
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(3, 4);
  tft.print("WiFi: ");

  // Estado con color
  uint16_t col = COLOR_WARN;
  const char* txt = "Conectando";
  if (state == WIFI_CONNECTED)   { col = COLOR_OK;    txt = "Conectado";   }
  if (state == WIFI_DISCONNECTED){ col = COLOR_ERROR; txt = "Desconectado";}

  tft.setTextColor(col);
  tft.print(txt);

  // SSID (si hay)
  if (ssid && ssid[0]) {
    tft.setTextColor(ST77XX_WHITE);
    tft.print("  ");
    tft.print(ssid);
  }

  // IP (solo si conectado y disponible)
  if (state == WIFI_CONNECTED && ip && ip[0]) {
    tft.setTextColor(ST77XX_YELLOW);
    // Imprimir IP al borde derecho (simple: en la línea de estado)
    // Para pantallas pequeñas priorizamos que se vea "Conectado"
    // Podés mover esto a showStatus si preferís.
    // Aquí la mostramos en la línea inferior de estado:
    // (no hacemos nada aquí, se puede mostrar con showStatus)
  }
}

// ==== CUERPO PRINCIPAL (peso) ====
void showWeight(float grams) {
  // limpia solo el área central del valor
  tft.fillRect(0, 28, 160, 52, ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  char buf[24];
  snprintf(buf, sizeof(buf), "%.1f g", grams);

  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  int cx = (160 - (int)w) / 2; if (cx < 0) cx = 0;

  tft.setCursor(cx, 44);
  tft.print(buf);
}

// ==== BARRA DE ESTADO (bajo el header) ====
void showStatus(const char* msg, uint16_t color) {
  tft.fillRect(0, HEADER_H, 160, 10, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(color);
  tft.setCursor(3, HEADER_H);
  tft.print(msg);
}
