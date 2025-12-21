#include "wifi_helper.h"
#include "config.h"
#include "display.h"
#include "hx711_helper.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// ======================================================
// CONFIG: URL de tu backend (EDITAR ESTA CONSTANTE)
// ======================================================
static const char* POST_URL = "http://192.168.1.12:8080/v1/weight";  
// Cambiala a algo como: "http://TU_IP:PUERTO/tus/rutas"

// Declaración de helper del módulo HX711 (definida en hx711_helper.cpp)
float readWeightForPost(uint8_t samples);

// ======================================================
// Estado Wi-Fi
// ======================================================

static const char* g_ssid = nullptr;
static const char* g_pass = nullptr;

static WifiUiState g_uiState = WIFI_DISCONNECTED;
static unsigned long g_lastCheckMs = 0;
static unsigned long g_lastConnectAttemptMs = 0;

// Config de hora (NTP Argentina)
static bool g_timeConfigured = false;

// ======================================================
// Estado botón PUL_ARCD (D21, pulsador arcade)
// ======================================================

static int btnLastStable   = HIGH;  // INPUT_PULLUP: HIGH = suelto
static int btnLastRead     = HIGH;
static unsigned long btnLastChangeMs = 0;

// ======================================================
// Helpers internos: Wi-Fi
// ======================================================

static void drawWifiHeaderOnce(WifiUiState newState) {
  if (newState == WIFI_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    String ipStr = ip.toString();
    drawHeaderWiFi(newState, g_ssid, ipStr.c_str());
  } else {
    drawHeaderWiFi(newState, g_ssid, nullptr);
  }
  g_uiState = newState;
}

static void ensureWifiConnected() {
  unsigned long now = millis();
  wl_status_t st = WiFi.status();

  if (st == WL_CONNECTED) {
    if (g_uiState != WIFI_CONNECTED) {
      drawWifiHeaderOnce(WIFI_CONNECTED);
      Serial.println("[WIFI] Conectado");
    }
    return;
  }

  // No conectado
  if (g_uiState == WIFI_CONNECTING) {
    // Si pasó el timeout y no conectó, lo marcamos como desconectado
    if (now - g_lastConnectAttemptMs > WIFI_CONNECT_TIMEOUT_MS) {
      drawWifiHeaderOnce(WIFI_DISCONNECTED);
      Serial.println("[WIFI] Timeout conectando, marcado como DISCONNECTED");
    }
    return;
  }

  // Estado DISCONNECTED: reintentar cada WIFI_RECHECK_MS
  if (now - g_lastCheckMs >= WIFI_RECHECK_MS) {
    g_lastCheckMs = now;

    Serial.println("[WIFI] Reintentando conexión...");
    WiFi.disconnect(true);
    WiFi.begin(g_ssid, g_pass);

    g_lastConnectAttemptMs = now;
    drawWifiHeaderOnce(WIFI_CONNECTING);
  }
}

// ======================================================
// Helpers internos: hora Argentina (NTP)
// ======================================================

static String getArgTimestampIso() {
  if (!g_timeConfigured) return String("");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) { // timeout 5s
    Serial.println("[TIME] getLocalTime FAIL");
    return String("");
  }

  // Formato base: 2025-11-29T19:45:12-0300
  char buf[40];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);

  // Transformamos -0300 en -03:00 para formato ISO
  String ts(buf);
  if (ts.length() >= 5) {
    ts = ts.substring(0, ts.length() - 2) + ":" + ts.substring(ts.length() - 2);
  }
  return ts;
}

// ======================================================
// Envío de peso por POST
// ======================================================
static void sendWeightNow() {
  // No enviar durante calibración
  if (isCalibrating()) {
    showStatus("CAL en curso", COLOR_WARN);
    Serial.println("[WIFI] Ignorado: en modo CAL");
    return;
  }

  // Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) {
    showStatus("Sin WiFi", COLOR_ERROR);
    Serial.println("[WIFI] Error: no hay WiFi para enviar");
    return;
  }

  Serial.print("[WIFI] IP local ESP32 = ");
  Serial.println(WiFi.localIP());

  // Lectura estable del peso
  const uint8_t N_SAMPLES = 30;
  float weight = readWeightForPost(N_SAMPLES);

  Serial.print("[WIFI] Peso listo para enviar = ");
  Serial.print(weight, 1);
  Serial.println(" g");

  // Timestamp en hora Argentina (UTC-3)
  String ts = getArgTimestampIso();
  Serial.print("[TIME] timestamp ARG = ");
  Serial.println(ts);

  // JSON para tu DTO
  String jsonBody = "{";
  jsonBody += "\"evento\":\"PESO_TOMADO\",";
  jsonBody += "\"weight\":" + String(weight, 1);
  if (ts.length() > 0) {
    jsonBody += ",\"hora\":\"" + ts + "\"";
  }
  jsonBody += "}";

  Serial.print("[WIFI] JSON enviado: ");
  Serial.println(jsonBody);

  // --- usar WiFiClient explícito ---
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, POST_URL)) {
    Serial.println("[WIFI] http.begin() fallo (URL invalida?)");
    showStatus("HTTP begin fail", COLOR_ERROR);
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);  // 5s de timeout por las dudas

  showStatus("Enviando...", COLOR_WARN);

  int code = http.POST(jsonBody);

  if (code > 0) {
    Serial.print("[WIFI] HTTP status = ");
    Serial.println(code);

    if (code == 200 || code == 201) {
      showStatus("POST OK", COLOR_OK);
    } else {
      showStatus("POST ERR", COLOR_ERROR);
    }
  } else {
    Serial.print("[WIFI] HTTP error = ");
    Serial.println(code);
    Serial.print("[WIFI] errorToString = ");
    Serial.println(http.errorToString(code));  // suele decir "connection refused" etc.
    showStatus("POST FAIL", COLOR_ERROR);
  }

  http.end();
}

// ======================================================
// Helpers internos: botón PUL_ARCD (D21)
// ======================================================

static void checkSendButton() {
  unsigned long now = millis();
  int raw = digitalRead(PUL_ARCD);

  if (raw != btnLastRead) {
    btnLastRead = raw;
    btnLastChangeMs = now;
  }

  if ((now - btnLastChangeMs) >= DEBOUNCE_MS && raw != btnLastStable) {
    btnLastStable = raw;

    // Flanco de bajada: botón apretado (INPUT_PULLUP)
    if (btnLastStable == LOW) {
      Serial.println("[WIFI] Botón arcade presionado -> enviar peso");
      sendWeightNow();
    }
  }
}

// ======================================================
// API pública
// ======================================================

void setupWiFi(const char* ssid, const char* pass) {
  g_ssid = ssid;
  g_pass = pass;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  g_lastConnectAttemptMs = millis();
  g_lastCheckMs = g_lastConnectAttemptMs;

  drawWifiHeaderOnce(WIFI_CONNECTING);
  Serial.print("[WIFI] Conectando a SSID: ");
  Serial.println(ssid);

  // Botón arcade
  pinMode(PUL_ARCD, INPUT_PULLUP);

  // Configurar hora NTP en zona horaria de Argentina (UTC-3, sin DST)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  g_timeConfigured = true;
}

void updateWiFiStatus() {
  // 1) Mantener conexión WiFi + header
  ensureWifiConnected();

  // 2) Leer botón y disparar POST en flanco de bajada
  checkSendButton();
}
