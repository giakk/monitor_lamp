#pragma once

void wifi_connect();

// Restituisce true se connesso in STA mode, false se in AP mode
bool wifi_is_sta();

// IP corrente come stringa (es. "192.168.1.42" o "192.168.4.1" in AP)
const char* wifi_get_ip();
