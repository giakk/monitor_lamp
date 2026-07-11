#pragma once

#include "config.h"
#include "7Semi_VEML7700.h"
#include "MyLD2410.h"
#include <Arduino.h>


struct SensorData
{                   
    float lux;
    bool presence;
    uint32_t distance_cm;
    bool lux_ok; 
    bool radar_ok;

};

void sensors_init();

SensorData sensors_read();