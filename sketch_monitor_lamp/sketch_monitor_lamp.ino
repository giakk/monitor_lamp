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
#define LEDC_GPIO       2
#define LEDC_FREQ_HZ    19000
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_START_DUTY  (0)
#define LEDC_TARGET_DUTY (4095)
#define FADE_TIME_MS   (3000)

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

  // // ── Autothreshold ────────────────────────────────────────────
  // if (AUTO_THRESHOLD) {
  //   Serial.printf("\nAutothreshold attivo — hai %d secondi per uscire dalla stanza!\n", AUTO_THRESHOLD_TIMEOUT_S);
  //   radar.configMode(true);
    
  //   if (radar.autoThresholds(AUTO_THRESHOLD_TIMEOUT_S)) {
  //     Serial.println("Autothreshold avviato, attendo completamento...");
  //     radar.configMode(false);

  //     // Aspetta finché non termina (successo o fallimento)
  //     unsigned long startTime = millis();
  //     unsigned long timeout = (AUTO_THRESHOLD_TIMEOUT_S + 35) * 1000UL; // timeout di sicurezza

  //     while (millis() - startTime < timeout) {
  //       radar.check();
  //       AutoStatus status = radar.getAutoStatus();

  //       if (status == AutoStatus::COMPLETED) {
  //         Serial.println("Autothreshold completato con successo!");
  //         break;
  //       } else if (status == AutoStatus::NOT_IN_PROGRESS) {
  //         Serial.println("Autothreshold fallito o non avviato.");
  //         break;
  //       }

  //       delay(500);
  //     }

  //     const MyLD2410::ValuesArray& mov = radar.getMovingThresholds();
  //     Serial.println("Moving thresholds:");
  //     for (byte i = 0; i <= mov.N; i++) {
  //       Serial.printf("  Gate %d: %d\n", i, mov.values[i]);
  //     }

  //     const MyLD2410::ValuesArray& sta = radar.getStationaryThresholds();
  //     Serial.println("Stationary thresholds:");
  //     for (byte i = 0; i <= sta.N; i++) {
  //       Serial.printf("  Gate %d: %d\n", i, sta.values[i]);
  //     }

  //   } else {
  //     radar.configMode(false);
  //     Serial.println("Autothreshold: command not accepted (sensor firmware >= 2.44 required)");
  //   }
  // }
}

void loop() {

  float lux = 0;

  if (radar.check() == MyLD2410::Response::DATA) {

    if (light.readLux(lux)) {
      Serial.print("Ambient light: ");
      lux = int(lux);
      Serial.print(lux);

      bool presence = radar.presenceDetected();
      Serial.print(" | Presenza: ");
      Serial.print(presence ? "SI | " : "NOooooooooooooooo | ");
      Serial.printf("Distanza: %lu cm | ", radar.detectedDistance());

      int pwm = presence ? sqrtFunction(lux) : 0;

      targetDuty = (uint32_t) pwm;
      if (currentDuty > targetDuty) {currentDuty -= fadeStep;}
      if (currentDuty < targetDuty) {currentDuty += fadeStep;}
      
      if (currentDuty < 0) currentDuty = 0;
      if (currentDuty > 1023) currentDuty = 1023;

      if (ledc_get_duty(LEDC_MODE, LEDC_CHANNEL) != currentDuty) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, currentDuty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
      }

      Serial.printf("PWM duty: %d/1023 -- %d\n", currentDuty, targetDuty);

    } else {
      Serial.println("Failed to read lux");
    }
  }

  delay(REFRESH_RATE);
}

int parabolicFunction(float lux) {
  double a = 1.0 / 1600.0;
  return (int)(a * lux * lux);
}

int line(float lux) {
  return (int)(0.25f * lux);
}

int sqrtFunction(float lux) {
  return (int)(sqrt(lux / 150) * 1023);
}