#include "sensors.h"

static VEML7700_7Semi light;
static HardwareSerial radarSerial(1); // UART1
static MyLD2410 radar(radarSerial);

void sensors_init(){
    // LD2410 Initialization
    radarSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    if (radar.begin()) {
        Serial.println("[sensors] LD2410 initialized"); //DEBUG
    } else {
        Serial.println("[sensors] LD2410 not responding"); //DEBUG
    }

    // VEML7700 Initialization
    if(light.begin(I2C_ADDRESS, Wire, I2C_CLOCK, I2C_SDA_PIN, I2C_SCL_PIN)){
        Serial.println("[sensors] VEML7700 initialized"); //DEBUG
    } else {
        Serial.println("[sensors] VEML7700 not responding"); //DEBUG
    }

    // VEML7700 Configuration
    light.setGain(VEML7700_GAIN_2);
    light.setIntegrationTime(VEML7700_IT_100MS);
    light.setPowerMode(VEML7700_PSM_MODE2, true);
}

SensorData sensors_read(){

    SensorData d = {};

    d.radar_ok = (radar.check() == MyLD2410::Response::DATA);
    if (d.radar_ok){
        d.presence = radar.presenceDetected();
        d.distance_cm = radar.detectedDistance();
    }

    d.lux_ok = light.readLux(d.lux);
    if (d.lux_ok){
        d.lux = (float)(int)d.lux;
    }

    //Serial.printf("[LD2410] Presence: %d | Distance: %lu [VEML7700] Lux: %f \n", d.presence, d.distance_cm, d.lux);


    return d;

}