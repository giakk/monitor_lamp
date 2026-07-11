#include "wifi_manager.h"
#include "config_manager.h"
#include <WiFi.h>

static bool   s_sta_mode = false;
static String s_ip       = "0.0.0.0";

void wifi_connect() {
  const AppConfig& cfg = config_get();

  WiFi.setHostname(cfg.hostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());

  Serial.printf("[wifi] Connecting to %s", cfg.wifi_ssid.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < cfg.wifi_timeout_ms) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    s_sta_mode = true;
    s_ip       = WiFi.localIP().toString();
    Serial.printf("\n[wifi] Connected — IP: %s\n", s_ip.c_str());
  } else {
    // Fallback Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg.ap_ssid.c_str(), cfg.ap_password.c_str());
    s_sta_mode = false;
    s_ip       = WiFi.softAPIP().toString();
    Serial.printf("\n[wifi] STA failed — AP mode (%s), IP: %s\n",
                  cfg.ap_ssid.c_str(), s_ip.c_str());
  }
}

bool wifi_is_sta()       { return s_sta_mode; }
const char* wifi_get_ip(){ return s_ip.c_str(); }
