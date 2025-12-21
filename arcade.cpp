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
    // arranca apagado para que se note
    ledWrite(false);
  }

  if (m == LEDMODE_BLINK_ERR) {
    g_errStartMs = millis();
    ledWrite(false);
  }
}

static void ledTick() {
  unsigned long now = millis();

  // ------------------------------------------------------------
  // Reglas por WiFi (con transición):
  // - Boot: LED apagado (setupArcade)
  // - Al conectarse al WiFi: LED encendido
  // - Si se desconecta / no pudo conectar: LED apagado
  // ------------------------------------------------------------
  static bool s_prevWifi = false;
  bool wifiNow = wifiIsConnected();

  // WiFi se cayó => off inmediato
  if (!wifiNow && s_prevWifi) {
    if (g_ledMode != LEDMODE_OFF) setLedMode(LEDMODE_OFF);
  }

  // WiFi acaba de conectar => ON (si no estamos en trabajo/error)
  if (wifiNow && !s_prevWifi) {
    if (g_ledMode == LEDMODE_OFF) {
      setLedMode(LEDMODE_ON);
    }
  }

  s_prevWifi = wifiNow;

  // Regla principal: sin WiFi => apagado SIEMPRE
  if (!wifiNow) {
    if (g_ledMode != LEDMODE_OFF) setLedMode(LEDMODE_OFF);
    return;
  }

  switch (g_ledMode) {
    case LEDMODE_OFF:
      // se queda apagado
      break;

    case LEDMODE_ON:
      // se queda fijo
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
        // terminado el error => apagado (aunque haya WiFi)
        setLedMode(LEDMODE_OFF);
      }
      break;
  }
}

// -----------------------------------------------------------------
// Hook llamado desde hx711_helper.cpp (weak) para mantener el LED
// vivo mientras se esperan lecturas del HX711.
// -----------------------------------------------------------------
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
    return true; // evento inmediato
  }

  // si está latched, liberamos sólo al soltar estable DEBOUNCE_MS
  if (g_latched && !pressed) {
    if (g_releaseStart == 0) g_releaseStart = now;
    if (now - g_releaseStart >= DEBOUNCE_MS) {
      g_latched = false;
      g_releaseStart = 0;
    }
  } else if (g_latched && pressed) {
    // sigue apretado => no hacer nada
    g_releaseStart = 0;
  }

  return false;
}

// ======================
// HTTP POST "no-bloqueante visualmente" (titila mientras espera respuesta)
// ======================
static int httpPostJsonWithBlink(const char* url, const String& jsonBody) {
  String host, path;
  uint16_t port;
  if (!parseHttpUrl(url, host, port, path)) {
    Serial.println("[ARCADE] URL invalida");
    return -1;
  }

  WiFiClient client;
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

  // esperar status line
  unsigned long start = millis();
  String statusLine;

  while (millis() - start < HTTP_TIMEOUT_MS) {
    ledTick(); // <- mantiene el titileo mientras esperamos
    if (client.available()) {
      statusLine = client.readStringUntil('\n');
      break;
    }
    delay(1);
  }

  client.stop();

  if (statusLine.length() == 0) {
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
  String codeStr = (sp2 > sp1) ? statusLine.substring(sp1 + 1, sp2) : statusLine.substring(sp1 + 1);
  int code = codeStr.toInt();
  return code;
}

// ======================
// API pública
// ======================
void setupArcade() {
  pinMode(LED_ARCD, OUTPUT);
  pinMode(PUL_ARCD, INPUT_PULLUP);

  // arranca en OFF hasta que haya WiFi (regla del usuario)
  setLedMode(LEDMODE_OFF);

  Serial.println("[Arcade] Inicializado");
}

void updateArcade() {
  // primero, mantener LED acorde a WiFi/estado
  ledTick();

  // si no hay WiFi, no aceptamos enviar (y LED queda OFF)
  if (!wifiIsConnected()) {
    return;
  }

  // detectar tap (one-shot hasta soltar)
  if (!arcadePressedOnce()) return;

  // ignorar si está calibrando
  if (isCalibrating()) {
    showStatus("CAL en curso", COLOR_WARN);
    Serial.println("[ARCADE] Ignorado: en CAL");
    return;
  }

  // 1) titilar mientras lee peso y postea
  setLedMode(LEDMODE_BLINK_POST);
  showStatus("Leyendo...", COLOR_WARN);

  float weight = readWeightForPost(POST_SAMPLES);

  Serial.print("[ARCADE] Peso listo = ");
  Serial.print(weight, 1);
  Serial.println(" g");

  // JSON
  String jsonBody = "{";
  jsonBody += "\"evento\":\"PESO_TOMADO\",";
  jsonBody += "\"weight\":" + String(weight, 1);
  jsonBody += "}";

  showStatus("Enviando...", COLOR_WARN);
  int code = httpPostJsonWithBlink(POST_URL, jsonBody);

  Serial.print("[ARCADE] HTTP code = ");
  Serial.println(code);

  if (code == 200 || code == 201) {
    showStatus("POST OK", COLOR_OK);
    setLedMode(LEDMODE_ON);  // queda encendido fijo
  } else {
    showStatus("POST ERR", COLOR_ERROR);
    setLedMode(LEDMODE_BLINK_ERR); // titila muy rápido 5s y luego OFF
  }
}
