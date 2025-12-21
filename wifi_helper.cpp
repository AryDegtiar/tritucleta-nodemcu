#include "wifi_helper.h"
#include "config.h"
#include "display.h"

#include <WiFi.h>
#include <time.h>

static const char* g_ssid = nullptr;
static const char* g_pass = nullptr;

static WifiUiState g_uiState = WIFI_DISCONNECTED;
static unsigned long g_lastCheckMs = 0;
static unsigned long g_lastConnectAttemptMs = 0;

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

  if (g_uiState == WIFI_CONNECTING) {
    if (now - g_lastConnectAttemptMs > WIFI_CONNECT_TIMEOUT_MS) {
      drawWifiHeaderOnce(WIFI_DISCONNECTED);
      Serial.println("[WIFI] Timeout conectando, marcado como DISCONNECTED");
    }
    return;
  }

  if (now - g_lastCheckMs >= WIFI_RECHECK_MS) {
    g_lastCheckMs = now;

    Serial.println("[WIFI] Reintentando conexión...");
    WiFi.disconnect(true);
    WiFi.begin(g_ssid, g_pass);

    g_lastConnectAttemptMs = now;
    drawWifiHeaderOnce(WIFI_CONNECTING);
  }
}

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

  // Hora Argentina (UTC-3)
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void updateWiFiStatus() {
  ensureWifiConnected();
}

bool wifiIsConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
