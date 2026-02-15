#pragma once

#include <Arduino.h>
#include <DHT.h>

// ============================================================================
// ENVIRONMENT SENSOR (DHT22)
// ============================================================================
// Manages DHT22 temperature and humidity sensor with error handling

class EnvironmentSensor {
public:
    EnvironmentSensor();
    ~EnvironmentSensor();

    // Initialize sensor
    void begin(uint8_t pin, uint8_t type);

    // Read temperature in Celsius
    float readTemperature();

    // Read humidity in percentage
    float readHumidity();

    // Check if last read was valid
    bool isValid() const;

    // Get time since last read
    unsigned long timeSinceLastRead() const;

private:
    DHT* dht_;
    uint8_t pin_;
    uint8_t type_;
    unsigned long lastReadTime_;
    bool lastReadValid_;
    float lastTemperature_;
    float lastHumidity_;

    // Recovery tracking
    int consecutiveFailures_;
    unsigned long lastRecoveryAttempt_;

    // Read both temp and humidity together (DHT sensor limitation)
    void readSensor();

    // Attempt to recover stuck sensor
    void attemptRecovery();
};
