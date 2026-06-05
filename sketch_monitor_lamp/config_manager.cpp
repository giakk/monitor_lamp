#include "config_manager.h"

static ConfigVariables s_cfg;

static void set_defaults(ConfigVariables& vars){
    
    vars.wifi_ssid        = "TUO_SSID";
    vars.wifi_password    = "TUA_PASSWORD";
    vars.hostname         = "monitor-lamp";
    vars.wifi_timeout_ms  = 10000;
    
    vars.fade_time_ms     = 2000;
    vars.pwm_hysteresis   = 50;
    vars.max_lux          = 400.0f;
    
    vars.refresh_rate_ms  = 100;
}

const ConfigVariables& config_get(){
    return s_cfg;
}