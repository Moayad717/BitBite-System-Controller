#pragma once

// ============================================================================
// HARDWARE PIN CONFIGURATION - ESP32 Feeding Module
// ============================================================================

// Motor Control
#define MOTOR_RELAY_PIN 5       // Relay/transistor control for feeding motor
#define MOTOR_SENSE_PIN 32      // Motor current sense (LOW=running, HIGH=stopped)

// Weight Sensor (HX711)
#define SCALE_DOUT_PIN 18       // HX711 data output
#define SCALE_CLK_PIN 25        // HX711 clock

// I2C Devices (SDA=21, SCL=22)
#define I2C_SDA 21
#define I2C_SCL 22
#define LCD_I2C_ADDRESS 0x27    // 16x2 LCD display

// Temperature/Humidity Sensor
#define DHT_PIN 4               // DHT22 data pin
#define DHT_TYPE DHT22          // DHT sensor type

// Water Flow Sensor
#define FLOW_SENSOR_PIN 33      // YF-S201 pulse output (interrupt capable)

// Serial Communication with WiFi ESP
#define RXD2 16                 // Serial2 RX (receives from WiFi ESP)
#define TXD2 17                 // Serial2 TX (sends to WiFi ESP)
#define SERIAL2_BAUD 9600       // Must match WiFi ESP

// LCD Display Configuration
#define LCD_COLS 16
#define LCD_ROWS 2
