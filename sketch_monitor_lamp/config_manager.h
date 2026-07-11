#pragma once
#include <Arduino.h>

// ── Struttura configurazione runtime ─────────────────────────────────────────
// Caricata da /config.json su LittleFS all'avvio.
// Se il file non esiste o è corrotto, vengono usati i valori di default.

struct AppConfig {
  // WiFi
  String wifi_ssid;
  String wifi_password;
  String hostname;
  uint32_t wifi_timeout_ms;

  // Access Point di fallback
  String ap_ssid;
  String ap_password;

  // Lamp — logica automatica
  float    lux_max;        // lux a cui il LED si spegne in AUTO
  uint32_t fade_time_ms;   // durata massima del fade
  uint32_t pwm_hysteresis; // minima variazione duty per triggherare un fade

  // Loop
  uint32_t refresh_rate_ms;
};

// ── API ───────────────────────────────────────────────────────────────────────

// Da chiamare dopo LittleFS.begin(). Carica /config.json o scrive i default.
void config_init();

// Accesso alla config corrente (read-only)
const AppConfig& config_get();

// Sovrascrive la config in memoria e salva su /config.json.
// Restituisce true se il salvataggio è andato a buon fine.
bool config_save(const AppConfig& cfg);

// Ripristina i valori di default e salva.
bool config_reset();

// Serializza la config corrente in JSON (per l'API REST)
String config_to_json();

// Deserializza un JSON e aggiorna solo i campi presenti.
// Restituisce true se il JSON era valido.
bool config_update_from_json(const String& json);
