#include "config.h"
#include "7Semi_VEML7700.h"
#include "MyLD2410.h"
#include <Arduino.h>


struct sensors_data
{                   
    float lux;
    bool presence;
    uint32_t distance_cm;
    bool lux_ok; 
    bool radar_ok;

};

void sensors_init();

sensors_data sensors_read();