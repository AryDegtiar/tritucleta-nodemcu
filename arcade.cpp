#include "arcade.h"
#include "config.h"
#include "wifi_helper.h"
#include "hx711_helper.h"
#include "display.h"

#include <WiFi.h>

// ======================
// Estado LED
// ======================
enum LedMode {
  LEDMODE_OFF,
  LEDMODE_ON,
  LEDMODE_BLINK_POST,
  LEDMODE_BLINK_ERR
};

static LedMode g_ledMode = LEDMODE_OFF;
static bool g_ledState = false;
static unsigned long g_lastToggleMs = 0;
static unsigned long g_errStartMs = 0;

// ======================
// Botón: one-shot hasta soltar
// ======================
static bool g_latched = false;
static unsigned long g_releaseStart = 0;

// ======================
// URL parser simple: http://host:port/path
// ======================
static bool parseHttpUrl(const char* url, String& host, uint16_t& port, String& path) {
  String s(url);
  if (!s.startsWith("http://")) return false;
  s.remove(0, 7);

  int slash = s.indexOf('/');
  String hostport = (slash >= 0) ? s.substring(0, slash) : s;
  path = (slash >= 0) ? s.substring(slash) : "/";

  int colon = hostport.indexOf(':');
  if (colon >= 0) {
    host = hostport.substring(0, colon);
    port = (uint16_t)hostport.substring(colon + 1).toInt();
  } else {
    host = hostport;
    port = 80;
  }
  return (host.length() > 0);
}

// ======================
// LED helper
// ======================
static void ledWrite(bool on) {
  g_ledState = on;
  digitalWrite(LED_ARCD, on ? HIGH : LOW);
}

static void setLedMode(LedMode m) {
  g_ledMode = m;
  g_lastToggleMs = millis();

  if (m == LEDMODE_OFF) ledWrite(false);
  if (m == LEDMODE_ON)  ledWrite(true);

  if (m == LEDMODE_BLINK_POST) {
    ledWrite(false); // arranca apagado para que se note
  }

  if (m == LEDMODE_BLINK_ERR) {
    g_errStartMs = millis();
    ledWrite(false);
  }
}

static void ledTick() {
  unsigned long now = millis();

  // ------------------------------------------------------------
  // Reglas por WiFi:
  // - Boot: LED apagado
  // - Al conectarse: LED ON
  // - Si se desconecta: LED OFF
  // - Sin WiFi: OFF SIEMPRE
  // ------------------------------------------------------------
  static bool s_prevWifi = false;
  bool wifiNow = wifiIsConnected();

  if (!wifiNow && s_prevWifi) {
    if (g_ledMode != LEDMODE_OFF) setLedMode(LEDMODE_OFF);
  }

  if (wifiNow && !s_prevWifi) {
    if (g_ledMode == LEDMODE_OFF) setLedMode(LEDMODE_ON);
  }

  s_prevWifi = wifiNow;

  if (!wifiNow) {
    if (g_ledMode != LEDMODE_OFF) setLedMode(LEDMODE_OFF);
    return;
  }

  switch (g_ledMode) {
    case LEDMODE_OFF:
      break;

    case LEDMODE_ON:
      break;

    case LEDMODE_BLINK_POST:
      if (now - g_lastToggleMs >= LED_BLINK_POST_MS) {
        g_lastToggleMs = now;
        ledWrite(!g_ledState);
      }
      break;

    case LEDMODE_BLINK_ERR:
      if (now - g_lastToggleMs >= LED_BLINK_ERR_MS) {
        g_lastToggleMs = now;
        ledWrite(!g_ledState);
      }
      if (now - g_errStartMs >= LED_ERR_TOTAL_MS) {
        // ✅ después del error, vuelve a ON si hay WiFi
        setLedMode(wifiIsConnected() ? LEDMODE_ON : LEDMODE_OFF);
      }
      break;
  }
}

// Hook para HX711 (si tu readWeightForPost() lo usa)
void hx711Yield() {
  ledTick();
}

// ======================
// Botón: dispara instantáneo al tocar, NO repite hasta soltar estable
// ======================
static bool arcadePressedOnce() {
  unsigned long now = millis();
  bool pressed = (digitalRead(PUL_ARCD) == LOW); // INPUT_PULLUP

  if (!g_latched && pressed) {
    g_latched = true;
    g_releaseStart = 0;
    return true;
  }

  if (g_latched && !pressed) {
    if (g_releaseStart == 0) g_releaseStart = now;
    if (now - g_releaseStart >= DEBOUNCE_MS) {
      g_latched = false;
      g_releaseStart = 0;
    }
  } else if (g_latched && pressed) {
    g_releaseStart = 0;
  }

  return false;
}

// ======================
// HTTP POST con titileo real (sin readStringUntil bloqueante)
// ======================
static int httpPostJsonWithBlink(const char* url, const String& jsonBody) {
  String host, path;
  uint16_t port;
  if (!parseHttpUrl(url, host, port, path)) {
    Serial.println("[ARCADE] URL invalida");
    return -1;
  }

  WiFiClient client;
  client.setTimeout(1);

  if (!client.connect(host.c_str(), port)) {
    Serial.println("[ARCADE] connect() fail");
    return -2;
  }

  // request
  String req;
  req += "POST " + path + " HTTP/1.1\r\n";
  req += "Host: " + host + "\r\n";
  req += "Content-Type: application/json\r\n";
  req += "Connection: close\r\n";
  req += "Content-Length: " + String(jsonBody.length()) + "\r\n\r\n";
  req += jsonBody;

  client.print(req);

  // Leer status line char-by-char, tickeando LED siempre
  unsigned long start = millis();
  String statusLine;
  bool gotLine = false;

  while (millis() - start < HTTP_TIMEOUT_MS) {
    ledTick();

    while (client.available()) {
      char c = (char)client.read();
      if (c == '\r') continue;
      if (c == '\n') { gotLine = true; break; }
      statusLine += c;
      if (statusLine.length() > 120) { gotLine = true; break; }
    }

    if (gotLine) break;
    delay(1);
  }

  client.stop();

  if (!gotLine || statusLine.length() == 0) {
    Serial.println("[ARCADE] timeout sin respuesta");
    return -3;
  }

  statusLine.trim();
  Serial.print("[ARCADE] statusLine: ");
  Serial.println(statusLine);

  // parse "HTTP/1.1 200 OK"
  int sp1 = statusLine.indexOf(' ');
  if (sp1 < 0) return -4;
  int sp2 = statusLine.indexOf(' ', sp1 + 1);

  String codeStr = (sp2 > sp1)
    ? statusLine.substring(sp1 + 1, sp2)
    : statusLine.substring(sp1 + 1);

  return codeStr.toInt();
}

// ======================
// API pública
// ======================
void setupArcade() {
  pinMode(LED_ARCD, OUTPUT);
  pinMode(PUL_ARCD, INPUT_PULLUP);

  setLedMode(LEDMODE_OFF); // arranca OFF hasta que haya WiFi
  Serial.println("[Arcade] Inicializado");
}

void updateArcade() {
  // mantener LED acorde a WiFi/estado
  ledTick();

  // si no hay WiFi, no aceptamos enviar
  if (!wifiIsConnected()) return;

  // si estamos en flash, igual dejamos que el LED siga tickeando.
  // (no bloqueamos el programa)

  // detectar tap
  if (!arcadePressedOnce()) return;

  if (isCalibrating()) {
    showStatus("CAL en curso", COLOR_WARN);
    Serial.println("[ARCADE] Ignorado: en CAL");
    return;
  }

  // titilar mientras lee + postea
  setLedMode(LEDMODE_BLINK_POST);
  showStatus("Leyendo...", COLOR_WARN);

  float weight = readWeightForPost(POST_SAMPLES);

  Serial.print("[ARCADE] Peso listo = ");
  Serial.print(weight, 1);
  Serial.println(" g");

  String jsonBody = "{";
  jsonBody += "\"evento\":\"PESO_TOMADO\",";
  jsonBody += "\"weight\":" + String(weight, 1);
  jsonBody += "}";

  showStatus("Enviando...", COLOR_WARN);
  int code = httpPostJsonWithBlink(POST_URL, jsonBody);

  Serial.print("[ARCADE] HTTP code = ");
  Serial.println(code);

  bool ok = (code >= 200 && code <= 299);

  if (ok) {
    showStatus("POST OK", COLOR_OK);
    flashStart(displayOkColor(), "OK", 5000);
    setLedMode(LEDMODE_ON);
  } else {
    showStatus("POST ERR", COLOR_ERROR);

    // LED titila EN PARALELO con la pantalla roja
    setLedMode(LEDMODE_BLINK_ERR);

    // pantalla roja 5s (NO bloqueante)
    flashStart(displayErrColor(), "ERROR", 5000);
  }
  drawHeaderWiFi(wifiIsConnected() ? WIFI_CONNECTED : WIFI_DISCONNECTED, WIFI_SSID, nullptr);

}
