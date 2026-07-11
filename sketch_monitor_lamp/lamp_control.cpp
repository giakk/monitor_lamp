#include "lamp_control.h";

static bool lamp_is_on = true;
static LampMode lamp_mode = LampMode::AUTO;
static const AppConfig* cfg = nullptr;
static uint8_t s_brightness = 100;

void lamp_init(){
    ledcAttach(LEDC_GPIO, LEDC_FREQ_HZ, LEDC_RESOLUTION);
    ledcWrite(LEDC_GPIO, 0);
    cfg = &config_get();
}

void lamp_update(const SensorData& data){

    uint32_t current_duty = ledcRead(LEDC_GPIO);
    uint32_t pwm_target;

    if (!lamp_is_on || !data.presence) {
        pwm_target = 0;
    } else if (lamp_mode == LampMode::MANUAL) {
        pwm_target = percent_to_duty(s_brightness);
    } else {
        pwm_target = 0;
        if (data.radar_ok && data.lux_ok) {
            pwm_target = data.presence ? uint32_t(sqrtFunction(data.lux)) : 0;
        }
    }

    if (abs((int)current_duty - (int)pwm_target) > cfg->pwm_hysteresis) {
        uint32_t delta = abs((int)pwm_target - (int)current_duty);
        uint32_t fade_ms = map(delta, 0, LEDC_MAX_DUTY, 0, cfg->fade_time_ms);
        ledcFade(LEDC_GPIO, current_duty, pwm_target, fade_ms);
    }
}

// _ Helper functions ___________________________________________________

int sqrtFunction(float lux) {
  
    if (lux >= cfg->lux_max) 
        return LEDC_MAX_DUTY;

    if (lux <= 0.0f)
        return 0;

    int val = (int)(sqrt(lux / cfg->lux_max) * LEDC_MAX_DUTY);

    return constrain(val, 0, LEDC_MAX_DUTY);
}

uint32_t percent_to_duty(uint8_t pct) {
    return (uint32_t)map(constrain(pct, 0, 100), 0, 100, 0, LEDC_MAX_DUTY);
}

void lamp_set_brightness(uint8_t percent){
    s_brightness = constrain(percent, 0, 100);
}

uint8_t lamp_get_brightness(){
    return s_brightness;
}

void lamp_set_mode(LampMode mode){
    lamp_mode = mode;
}

LampMode lamp_get_mode(){
    return lamp_mode;
}

void lamp_set_on(bool set){
    lamp_is_on = set;
}
bool lamp_get_on(){
    return lamp_is_on;
}

uint32_t lamp_get_duty(){
    return ledcRead(LEDC_GPIO);
}