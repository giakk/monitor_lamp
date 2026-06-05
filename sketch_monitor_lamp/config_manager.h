#pragma once
#include <Arduino.h>

struct ConfigVariables {

    // WiFi
    String wifi_ssid;
    String wifi_password;
    String hostname;
    uint32_t wifi_timeout_ms;   // Time to try to connect to wifi

    // Lamp logic variables
    uint32_t fade_time_ms;   // Fade max duration
    uint32_t pwm_hysteresis; // Minimum duty variation to trigger a fade action
    float max_lux;

    // Loop
    uint32_t refresh_rate_ms;   // Refresh time of the loop cycle

};

const ConfigVariables& config_get();