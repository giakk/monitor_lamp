#pragma once

// ESP32 setup variables.

// ── I2C (VEML7700) ────────────────────────────────────────────────────────────
#define I2C_ADDRESS 0x10
#define I2C_CLOCK 400000
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// ── PWM (LED strip) ───────────────────────────────────────────────────────────
#define LEDC_GPIO       21
#define LEDC_FREQ_HZ    5000
#define LEDC_RESOLUTION LEDC_TIMER_12_BIT
#define LEDC_MAX_DUTY   ((1 << LEDC_RESOLUTION) - 1)

// ── UART (LD2410C) ────────────────────────────────────────────────────────────
#define TX_PIN 4
#define RX_PIN 3
#define NO_ONE_WINDOW_S 1
