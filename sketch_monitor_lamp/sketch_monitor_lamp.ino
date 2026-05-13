#include "7Semi_VEML7700.h"
#include "MyLD2410.h"
#include "esp32-hal-ledc.h"

// I2C settings
#define I2C_ADDRESS 0x10
#define I2C_CLOCK 400000
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// PWM settings
#define LEDC_GPIO       2
#define LEDC_FREQ_HZ    19000
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_START_DUTY  (0)
#define LEDC_TARGET_DUTY (4095)
#define FADE_TIME_MS   (1000)
bool fade_ended = false;  // status of LED fade
bool fade_in = true;

// UART LD2410C settings
#define TX_PIN 4
#define RX_PIN 3
#define NO_ONE_WINDOW_S 1

// ── Metti true per eseguire l'autocalibrazione all'avvio ──
#define AUTO_THRESHOLD false
#define AUTO_THRESHOLD_TIMEOUT_S 10  // secondi per uscire dalla stanza

#define REFRESH_RATE 100 //millisec

VEML7700_7Semi light;

HardwareSerial radarSerial(1); // UART1
MyLD2410 radar(radarSerial);

int sqrtFunction(float lux);
void ld2410_inizialization();
void veml7700_inizialization();
void leds_initialization();

void ARDUINO_ISR_ATTR LED_FADE_ISR() {
  fade_ended = true;
}

float lux = 0;

void setup() {

  Serial.begin(115200);

  veml7700_inizialization();
  ld2410_inizialization();
  leds_initialization();

}

void loop() {
  float lux = 0;

  if (radar.check() == MyLD2410::Response::DATA && light.readLux(lux)) {
    lux = int(lux);

    bool presence = radar.presenceDetected();

    uint32_t pwm_target = uint32_t(presence ? sqrtFunction(lux) : 0);

    uint32_t current_duty = ledcRead(LEDC_GPIO);

    const uint32_t HYSTERESIS = 50;
    if (abs((int)current_duty - (int)pwm_target) > HYSTERESIS) {

      uint32_t delta = abs((int)pwm_target - (int)current_duty);
      uint32_t fade_ms = map(delta, 0, (1 << LEDC_RESOLUTION) - 1, 0, FADE_TIME_MS);

      ledcFade(LEDC_GPIO, current_duty, pwm_target, fade_ms);
    }
  }
}

int luxToDuty(float lux, float lux_max = 500.0f, float gamma = 2.2f) {
  const int max_duty = (1 << LEDC_TIMER_12_BIT) - 1;

  // 1. Normalizza lux in [0.0, 1.0]
  float normalized = constrain(lux / lux_max, 0.0f, 1.0f);

  // 2. Gamma correction: più lux c'è, MENO luce vogliamo dal LED
  //    Invertiamo: con tanta luce ambientale il LED si abbassa
  float inverted = 1.0f - normalized;

  // 3. Applica gamma (percettivamente lineare)
  float corrected = pow(inverted, gamma);

  return (int)(corrected * max_duty);
}


void veml7700_inizialization(){
    // VEML7700 Initialization
  light.begin(I2C_ADDRESS, Wire, I2C_CLOCK, I2C_SDA_PIN, I2C_SCL_PIN);

  // VEML7700 Configuration
  light.setGain(VEML7700_GAIN_2);
  light.setIntegrationTime(VEML7700_IT_100MS);
  light.setPowerMode(VEML7700_PSM_MODE2, true);
}

void ld2410_inizialization(){
  // LD2410 Initialization
  radarSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
  if (radar.begin()) {
    //Serial.println("LD2410 inizializzato");
  } else {
    Serial.println("LD2410 non risponde");
  }

  //radar.setResolution(true);  //true to set resolution 'fine' (20cm)
  //radar.setNoOneWindow(NO_ONE_WINDOW_S);
  // delay(1000);
  // radar.configMode(true); // enter configuration mode
  // radar.setGateParameters(0, 100, 100);
  // radar.setGateParameters(1, 100, 100);
  // radar.setGateParameters(2, 45, 45);
  // radar.setGateParameters(3, 25, 25);
  // radar.setGateParameters(4, 25, 25);
  // radar.setGateParameters(5, 60, 60);
  // radar.setGateParameters(6, 100, 100);
  // radar.setMaxGate(5, 5, 1);

  // const MyLD2410::ValuesArray& mov = radar.getMovingThresholds();
  // Serial.println("Moving thresholds:");
  // for (byte i = 0; i <= mov.N; i++) {
  //   Serial.printf("  Gate %d: %d\n", i, mov.values[i]);
  // }

  // const MyLD2410::ValuesArray& sta = radar.getStationaryThresholds();
  // Serial.println("Stationary thresholds:");
  // for (byte i = 0; i <= sta.N; i++) {
  //   Serial.printf("  Gate %d: %d\n", i, sta.values[i]);
  // }

  // Serial.println(radar.getResolution());

  // //radar.requestBToff();
  // radar.configMode(false);
  // delay(500);
}

void leds_initialization(){
  //led driver inizialization
  ledcAttach(LEDC_GPIO, LEDC_FREQ_HZ, LEDC_RESOLUTION);
  ledcWrite(LEDC_GPIO, 0);

}

int sqrtFunction(float lux) {
  
  // Clamp: lux oltre 150 supererebbe 4095
  const int max_duty = (1 << LEDC_TIMER_12_BIT) - 1; // = 4095

  if (lux >= 400.0f) return max_duty;
  if (lux <= 0.0f)   return 0;
  int val = (int)(sqrt(lux / 400.0f) * max_duty);
  // Sicurezza extra
  return constrain(val, 0, max_duty);
}