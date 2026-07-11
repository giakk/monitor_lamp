#pragma once
#include "sensors.h"

void webserver_init();

// Chiamata dal loop principale per aggiornare i dati esposti via API
void webserver_update_sensors(const SensorData& data);
