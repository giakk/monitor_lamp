#include "config.h"
#include "config_manager.h"
#include "sensors.h"
#include "lamp_control.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "esp_system.h"
#include <LittleFS.h>

static unsigned long lastSensorUpdate = 0;

void setup() {
  
Serial.begin(115200);
  delay(500);
  Serial.println("\n[boot] Monitor Lamp starting...");

  if (!LittleFS.begin(true)) {
    Serial.println("[boot] LittleFS mount failed");
  }  
  
  config_init();
  sensors_init();
  lamp_init();
  wifi_connect();
  webserver_init();
  Serial.println("[boot] Ready.");

}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorUpdate >= config_get().refresh_rate_ms) {
    lastSensorUpdate = now;

    SensorData data = sensors_read();
    lamp_update(data);
    webserver_update_sensors(data);
  }
}