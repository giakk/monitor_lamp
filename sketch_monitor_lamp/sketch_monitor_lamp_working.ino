/*

THIS FILE IS THE FIRST PROJECT AND WORKS. IT IS HERE AS A BACKUP IN CASE YOU NEED SOMETHING THAT JUST WORKS

#include "7Semi_VEML7700.h"
#include "driver/ledc.h"
#include "MyLD2410.h"
#include "esp_err.h"
#include "esp_log.h" // For ESP_LOGx


// I2C settings
#define I2C_ADDRESS 0x10
#define I2C_CLOCK 400000
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// PWM settings
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_GPIO       21
#define LEDC_FREQ_HZ    19000
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_START_DUTY  (0)
#define LEDC_TARGET_DUTY (4095)
#define FADE_TIME_MS   (1000)

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
uint32_t currentDuty = 0;
uint32_t targetDuty = 0; 
uint32_t fadeStep = 10;

bool fade_ended = false;  // status of LED fade
bool fade_in = true;

int parabolicFunction(float lux);
int line(float lux);
int sqrtFunction(float lux);

void setup() {

  Serial.begin(115200);

  // VEML7700 Initialization
  light.begin(I2C_ADDRESS, Wire, I2C_CLOCK, I2C_SDA_PIN, I2C_SCL_PIN);

  // VEML7700 Configuration
  light.setGain(VEML7700_GAIN_2);
  light.setIntegrationTime(VEML7700_IT_100MS);
  light.setPowerMode(VEML7700_PSM_MODE2, true);

  // PWM Initialization
  ledc_timer_config_t timer_conf = {
    .speed_mode      = LEDC_MODE,
    .duty_resolution = LEDC_RESOLUTION,
    .timer_num       = LEDC_TIMER,
    .freq_hz         = LEDC_FREQ_HZ,
    .clk_cfg         = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {
    .gpio_num   = LEDC_GPIO,
    .speed_mode = LEDC_MODE,
    .channel    = LEDC_CHANNEL,
    .timer_sel  = LEDC_TIMER,
    .duty       = 0,
    .hpoint     = 0,
  };
  ledc_channel_config(&channel_conf);

  // Installa il servizio fade LEDC (chiamare UNA SOLA VOLTA nel setup!)
  ledc_fade_func_install(0);

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




void loop() {
  float lux = 0;

  if (radar.check() == MyLD2410::Response::DATA) {
    if (light.readLux(lux)) {
      lux = int(lux);

      bool presence = radar.presenceDetected();

      Serial.print("Ambient light: ");
      Serial.print(lux);
      Serial.print(" | Presenza: ");
      Serial.print(presence ? "SI | " : "NOooooooooooooooo | ");
      Serial.printf("Distanza: %lu cm\n", radar.detectedDistance());

      uint32_t pwm_target = uint32_t(presence ? sqrtFunction(lux) : 0);
      uint32_t current_duty = ledc_get_duty(LEDC_MODE, LEDC_CHANNEL);

      // Isteresi: riavvia il fade solo se la differenza è significativa
      const uint32_t HYSTERESIS = 50;
      if (abs((int)current_duty - (int)pwm_target) > HYSTERESIS) {

        // Velocità proporzionale: fade_ms è proporzionale alla distanza da percorrere
        uint32_t delta = abs((int)pwm_target - (int)current_duty);
        uint32_t fade_ms = map(delta, 0, (1 << LEDC_TIMER_12_BIT) - 1, 0, FADE_TIME_MS);

        ledc_fade_func_uninstall();
        ledc_fade_func_install(0);

        ledc_set_fade_with_time(LEDC_MODE, LEDC_CHANNEL, pwm_target, fade_ms);
        ledc_fade_start(LEDC_MODE, LEDC_CHANNEL, LEDC_FADE_NO_WAIT);
      }

    } else {
      Serial.println("Failed to read lux");
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

int sqrtFunction(float lux) {
  // Clamp: lux oltre 150 supererebbe 4095

  const int max_duty = (1 << LEDC_TIMER_12_BIT) - 1; // = 4095

  if (lux >= 400.0f) return max_duty;
  if (lux <= 0.0f)   return 0;
  int val = (int)(sqrt(lux / 400.0f) * max_duty);
  // Sicurezza extra
  return constrain(val, 0, max_duty);
}

*/