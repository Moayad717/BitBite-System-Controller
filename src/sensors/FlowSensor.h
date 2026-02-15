#pragma once

#include <Arduino.h>

// ============================================================================
// WATER FLOW SENSOR (YF-S201)
// ============================================================================
// Manages water flow sensor with ISR-safe pulse counting
// Daily reset at midnight

class FlowSensor {
public:
    FlowSensor();

    // Initialize sensor
    void begin(uint8_t pin);

    // Update flow calculation (call from main loop)
    void update();

    // Get total liters (daily accumulation)
    float getTotalLiters() const;

    // Set total liters (for loading from persistence)
    void setTotalLiters(float liters);

    // Initialize the last reset day (without clearing flow)
    void setLastResetDay(int day);

    // Reset daily total
    void resetDaily(int currentDay);

    // Check if midnight reset is needed
    bool needsMidnightReset(int currentDay);

    // ISR handler (must be static)
    static void IRAM_ATTR pulseISR();

private:
    uint8_t pin_;
    float calibrationFactor_;
    unsigned long lastUpdateTime_;
    int lastResetDay_;

    // ISR-safe pulse counter
    static volatile unsigned long pulseCount_;
    static FlowSensor* instance_;

    float totalLiters_;
};
