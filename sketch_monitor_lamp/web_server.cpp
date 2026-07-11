#include "web_server.h"
#include "config.h"
#include "config_manager.h"
#include "lamp_control.h"
#include "wifi_manager.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

static AsyncWebServer server(80);

// ── Stato sensori (aggiornato dal loop, letto dagli handler HTTP) ─────────────
// Usiamo volatile per le variabili scalari — sufficiente per ESP32 single-core C3
static volatile float    s_lux         = 0;
static volatile bool     s_presence    = false;
static volatile uint32_t s_distance_cm = 0;
static volatile bool     s_lux_ok      = false;
static volatile bool     s_radar_ok    = false;

// ── Helper: costruisce JSON di stato completo ─────────────────────────────────
static String build_state_json() {
  StaticJsonDocument<256> doc;

  doc["on"]          = lamp_get_on();
  doc["mode"]        = (lamp_get_mode() == LampMode::AUTO) ? "auto" : "manual";
  doc["brightness"]  = lamp_get_brightness();
  doc["duty"]        = lamp_get_duty();
  doc["duty_pct"]    = (int)((float)lamp_get_duty() / LEDC_MAX_DUTY * 100);
  doc["lux"]         = (float)s_lux;
  doc["presence"]    = (bool)s_presence;
  doc["distance_cm"] = (uint32_t)s_distance_cm;
  doc["lux_ok"]      = (bool)s_lux_ok;
  doc["radar_ok"]    = (bool)s_radar_ok;
  // doc["wifi_sta"]    = wifi_is_sta();
  doc["ip"]          = wifi_get_ip();

  String out;
  serializeJson(doc, out);
  return out;
}

// ── Route handlers ────────────────────────────────────────────────────────────

// GET /api/state
static void handle_get_state(AsyncWebServerRequest* req) {
  req->send(200, "application/json", build_state_json());
}

// POST /api/control
// Body JSON: { "on": bool, "mode": "auto"|"manual", "brightness": 0-100 }
// Tutti i campi sono opzionali — invia solo quelli da modificare.
static void handle_post_control(AsyncWebServerRequest* req, uint8_t* data,
                                 size_t len, size_t, size_t) {
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, data, len);

  if (err) {
    req->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  if (doc.containsKey("on")) {
    lamp_set_on(doc["on"].as<bool>());
  }
  if (doc.containsKey("mode")) {
    String m = doc["mode"].as<String>();
    lamp_set_mode(m == "auto" ? LampMode::AUTO : LampMode::MANUAL);
  }
  if (doc.containsKey("brightness")) {
    lamp_set_brightness(doc["brightness"].as<uint8_t>());
  }

  req->send(200, "application/json", build_state_json());
}

// GET /api/config
// Restituisce la configurazione corrente (password oscurate).
static void handle_get_config(AsyncWebServerRequest* req) {
  req->send(200, "application/json", config_to_json());
}

// POST /api/config
// Body JSON con i campi da modificare. Salva su LittleFS.
// NOTA: per aggiornare wifi_ssid/wifi_password il dispositivo va riavviato.
static void handle_post_config(AsyncWebServerRequest* req, uint8_t* data,
                                size_t len, size_t, size_t) {
  String body((char*)data, len);

  if (!config_update_from_json(body)) {
    req->send(400, "application/json", "{\"error\":\"json non valido o salvataggio fallito\"}");
    return;
  }

  // Risponde con la config aggiornata (password oscurate)
  req->send(200, "application/json", config_to_json());
}

// POST /api/config/reset
// Ripristina tutti i valori di default e salva.
static void handle_post_config_reset(AsyncWebServerRequest* req) {
  config_reset();
  req->send(200, "application/json", config_to_json());
}

// ── Init ──────────────────────────────────────────────────────────────────────

void webserver_init() {
  // LittleFS per i file statici dell'UI
  if (!LittleFS.begin(true)) {
    Serial.println("[web] LittleFS mount failed — UI non disponibile");
  } else {
    Serial.println("[web] LittleFS mounted");
  }

  // ── Serve UI ──────────────────────────────────────────────────────────────
  // Se esiste /data/index.html.gz lo serve compresso, altrimenti fallback inline
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (LittleFS.exists("/index.html.gz")) {
      AsyncWebServerResponse* resp =
        req->beginResponse(LittleFS, "/index.html.gz", "text/html");
      resp->addHeader("Content-Encoding", "gzip");
      req->send(resp);
    } else {
      // Fallback minimale — verrà sostituito dall'HTML completo su LittleFS
      req->send(200, "text/html",
        "<!DOCTYPE html><html><head><meta charset='utf-8'>"
        "<title>Monitor Lamp</title></head><body>"
        "<h2>Monitor Lamp</h2>"
        "<p>Carica <code>index.html.gz</code> su LittleFS per l'interfaccia completa.</p>"
        "<p><a href='/api/state'>GET /api/state</a></p>"
        "</body></html>");
    }
  });

  // ── API ───────────────────────────────────────────────────────────────────
  server.on("/api/state", HTTP_GET, handle_get_state);

  server.on("/api/control", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    nullptr,
    handle_post_control
  );

  server.on("/api/config", HTTP_GET, handle_get_config);

  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    nullptr,
    handle_post_config
  );

  server.on("/api/config/reset", HTTP_POST, handle_post_config_reset);

  // ── 404 ───────────────────────────────────────────────────────────────────
  server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "application/json", "{\"error\":\"not found\"}");
  });

  // ── CORS — utile durante sviluppo UI ─────────────────────────────────────
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();
  Serial.printf("[web] Server started — http://%s/\n", wifi_get_ip());
}

void webserver_update_sensors(const SensorData& data) {
  s_lux         = data.lux;
  s_presence    = data.presence;
  s_distance_cm = data.distance_cm;
  s_lux_ok      = data.lux_ok;
  s_radar_ok    = data.radar_ok;
}
