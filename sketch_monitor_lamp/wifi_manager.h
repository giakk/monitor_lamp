#pragma once
#include <WiFi.h>
#include <config_manager.h>


// Init method
void wifi_connect();

// Return the IP assigned by the router
const char* wifi_get_ip();
