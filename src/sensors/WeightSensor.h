#pragma once

#include <Arduino.h>
#include <HX711.h>

// ============================================================================
// WEIGHT SENSOR (HX711 Load Cell)
// ============================================================================
// Manages HX711 weight sensor with tare and calibration

class WeightSensor {
public:
    WeightSensor();

    // Initialize sensor
    bool begin(uint8_t doutPin, uint8_t clkPin, float calibrationFactor);

    // Read weight in kg (10 samples - accurate but slow ~1s)
    float readWeight();

    // Read weight in kg (fewer samples - faster ~300ms, for use during active feeding)
    float readWeightFast();

    // Tare (zero) the scale
    bool tare(uint8_t samples = 10);

    // Check if sensor is ready
    bool isReady() const;

    // Set calibration factor
    void setCalibrationFactor(float factor);

    // Get raw reading (for calibration)
    long readRaw();

    // Tare offset management (for persistence)
    long getTareOffset() const;
    void setTareOffset(long offset);

private:
    HX711 scale_;
    float calibrationFactor_;
    bool initialized_;
    float lastValidWeight_;  // Cache last valid reading for when scale is not ready
};
