#include "config_manager.h"
#include "config.h"   // costanti hardware immutabili (pin, risoluzione LEDC…)
#include <LittleFS.h>
#include <ArduinoJson.h>

#define CONFIG_PATH "/config.json"

// ── Default ───────────────────────────────────────────────────────────────────
static AppConfig s_cfg;

static void set_defaults(AppConfig& c) {
  c.wifi_ssid        = "";
  c.wifi_password    = "";
  c.hostname         = "monitor-lamp";
  c.wifi_timeout_ms  = 10000;

  c.ap_ssid          = "MonitorLamp-AP";
  c.ap_password      = "12345678";

  c.lux_max          = 400.0f;
  c.fade_time_ms     = 1000;
  c.pwm_hysteresis   = 50;

  c.refresh_rate_ms  = 100;
}

// ── Helpers di serializzazione ────────────────────────────────────────────────

static void json_to_config(JsonObjectConst obj, AppConfig& c) {
  // Aggiorna solo i campi presenti nel JSON — campo assente = invariato
  if (obj["wifi_ssid"].is<const char*>())
    c.wifi_ssid = obj["wifi_ssid"].as<String>();

  if (obj["wifi_password"].is<const char*>())
    c.wifi_password = obj["wifi_password"].as<String>();

  if (obj["hostname"].is<const char*>())
    c.hostname = obj["hostname"].as<String>();

  if (obj["wifi_timeout_ms"].is<uint32_t>())
    c.wifi_timeout_ms = obj["wifi_timeout_ms"].as<uint32_t>();

  if (obj["ap_ssid"].is<const char*>())
    c.ap_ssid = obj["ap_ssid"].as<String>();

  if (obj["ap_password"].is<const char*>())
    c.ap_password = obj["ap_password"].as<String>();

  if (obj["lux_max"].is<float>())
    c.lux_max = obj["lux_max"].as<float>();

  if (obj["fade_time_ms"].is<uint32_t>())
    c.fade_time_ms = obj["fade_time_ms"].as<uint32_t>();

  if (obj["pwm_hysteresis"].is<uint32_t>())
    c.pwm_hysteresis = obj["pwm_hysteresis"].as<uint32_t>();

  if (obj["refresh_rate_ms"].is<uint32_t>())
    c.refresh_rate_ms = obj["refresh_rate_ms"].as<uint32_t>();
}

static void config_to_doc(const AppConfig& c, JsonObject obj) {
  obj["wifi_ssid"]       = c.wifi_ssid;
  obj["wifi_password"]   = c.wifi_password;   // salvata in chiaro — vedi README
  obj["hostname"]        = c.hostname;
  obj["wifi_timeout_ms"] = c.wifi_timeout_ms;
  obj["ap_ssid"]         = c.ap_ssid;
  obj["ap_password"]     = c.ap_password;
  obj["lux_max"]         = c.lux_max;
  obj["fade_time_ms"]    = c.fade_time_ms;
  obj["pwm_hysteresis"]  = c.pwm_hysteresis;
  obj["refresh_rate_ms"] = c.refresh_rate_ms;
}

// ── Caricamento da file ───────────────────────────────────────────────────────

static bool load_from_file() {
  if (!LittleFS.exists(CONFIG_PATH)) {
    Serial.println("[config] config.json non trovato, uso default");
    return false;
  }

  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) {
    Serial.println("[config] apertura config.json fallita");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[config] JSON non valido: %s\n", err.c_str());
    return false;
  }

  json_to_config(doc.as<JsonObjectConst>(), s_cfg);
  Serial.println("[config] config.json caricato");
  return true;
}

// ── API ───────────────────────────────────────────────────────────────────────

void config_init() {
  set_defaults(s_cfg);

  if (!load_from_file()) {
    // Primo avvio o file corrotto: scrivi i default su disco
    config_save(s_cfg);
  }
}

const AppConfig& config_get() {
  return s_cfg;
}

bool config_save(const AppConfig& cfg) {
  s_cfg = cfg;

  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) {
    Serial.println("[config] impossibile aprire config.json in scrittura");
    return false;
  }

  StaticJsonDocument<512> doc;
  config_to_doc(s_cfg, doc.as<JsonObject>());
  serializeJsonPretty(doc, f);
  f.close();

  Serial.println("[config] config.json salvato");
  return true;
}

bool config_reset() {
  set_defaults(s_cfg);
  return config_save(s_cfg);
}

String config_to_json() {
  StaticJsonDocument<512> doc;
  config_to_doc(s_cfg, doc.as<JsonObject>());

  // Non esporre la password WiFi nell'API — sostituisci con placeholder
  doc["wifi_password"] = "********";
  doc["ap_password"]   = "********";

  String out;
  serializeJson(doc, out);
  return out;
}

bool config_update_from_json(const String& json) {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) return false;

  AppConfig updated = s_cfg;
  json_to_config(doc.as<JsonObjectConst>(), updated);
  return config_save(updated);
}
