#include <wifi_manager.h>

static String s_ip = "0.0.0.0";

void wifi_connect(){

    ConfigVariables& cfg = config_get();

    WiFi.setHostname(cfg.hostname.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());

    //DEBUG
    Serial.printf("[wifi] Connecting to %s", cfg.wifi_ssid.c_str());

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < cfg.wifi_timeout_ms) {
        delay(250);
        Serial.print(".");  //DEBUG
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        s_ip = WiFi.localIP().toString();
        Serial.printf("\n[wifi] Connected — IP: %s\n", s_ip.c_str());   //DEBUG
    }

}

const char* wifi_get_ip(){
    return s_ip.c_str();
}