#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config_manager.h" 

enum class LampMode {
  AUTO,    // lux + presenza → duty automatico
  MANUAL   // duty fisso impostato dall'utente
};

void lamp_init();
void lamp_update(const SensorData& data);

void lamp_set_on(bool set);
bool lamp_get_on();

void lamp_set_mode(LampMode mode);
LampMode lamp_get_mode();

void lamp_set_brightness(uint8_t percent);
uint8_t lamp_get_brightness();

uint32_t lamp_get_duty();