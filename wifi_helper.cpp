#include "wifi_helper.h"
#include "display.h"
#include "config.h"
#include <WiFi.h>

#ifndef WIFI_CONNECT_TIMEOUT_MS
  #define WIFI_CONNECT_TIMEOUT_MS 8000
#endif
#ifndef WIFI_RECHECK_MS
  #define WIFI_RECHECK_MS 2000
#endif

static bool g_wifiConnected = false;
static unsigned long g_lastWiFiCheck = 0;

static void headerToConnected(const char* ssid) {
  char ipbuf[24] = {0};
  auto ip = WiFi.localIP();
  snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  drawHeaderWiFi(WIFI_CONNECTED, ssid, ipbuf);
}
static void headerToDisconnected(const char* ssid) {
  drawHeaderWiFi(WIFI_DISCONNECTED, ssid, nullptr);
}

void setupWiFi(const char* ssid, const char* pass) {
  drawHeaderWiFi(WIFI_CONNECTING, ssid, nullptr);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    g_wifiConnected = true;
    headerToConnected(ssid);
  } else {
    g_wifiConnected = false;
    headerToDisconnected(ssid);
  }
}

void updateWiFiStatus() {
  unsigned long now = millis();
  if (now - g_lastWiFiCheck < WIFI_RECHECK_MS) return;
  g_lastWiFiCheck = now;

  bool up = (WiFi.status() == WL_CONNECTED);
  if (up != g_wifiConnected) {
    g_wifiConnected = up;
    if (up) headerToConnected(WIFI_SSID);
    else { headerToDisconnected(WIFI_SSID); WiFi.reconnect(); }
  }
}

bool wifiIsConnected() { return g_wifiConnected; }
