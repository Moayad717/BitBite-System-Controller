#include "FlowSensor.h"
#include "../config/CalibrationConfig.h"

// Static members
volatile unsigned long FlowSensor::pulseCount_ = 0;
FlowSensor* FlowSensor::instance_ = nullptr;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FlowSensor::FlowSensor()
    : pin_(0),
      calibrationFactor_(FLOW_SENSOR_CALIBRATION),
      lastUpdateTime_(0),
      lastResetDay_(-1),
      totalLiters_(0.0f) {

    instance_ = this;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void FlowSensor::begin(uint8_t pin) {
    pin_ = pin;

    pinMode(pin_, INPUT_PULLUP);

    // Attach interrupt on FALLING edge
    attachInterrupt(digitalPinToInterrupt(pin_), pulseISR, FALLING);

    lastUpdateTime_ = millis();
}

// ============================================================================
// FLOW CALCULATION
// ============================================================================

void FlowSensor::update() {
    unsigned long currentTime = millis();

    // Only update once per second minimum
    if (currentTime - lastUpdateTime_ >= 1000) {
        // Read pulse count atomically
        noInterrupts();
        unsigned long pulses = pulseCount_;
        interrupts();

        // Convert pulses to liters and add to total
        if (pulses > 0) {
            float liters = (float)pulses / calibrationFactor_;
            totalLiters_ += liters;

            // Reset pulse count atomically
            noInterrupts();
            pulseCount_ -= pulses;
            interrupts();
        }

        lastUpdateTime_ = currentTime;
    }
}

// ============================================================================
// GETTERS
// ============================================================================

float FlowSensor::getTotalLiters() const {
    return totalLiters_;
}

void FlowSensor::setTotalLiters(float liters) {
    totalLiters_ = liters;
    Serial.printf("[FLOW] Total liters set to: %.2f L\n", totalLiters_);
}

void FlowSensor::setLastResetDay(int day) {
    lastResetDay_ = day;
    Serial.printf("[FLOW] Last reset day set to: %d\n", lastResetDay_);
}

// ============================================================================
// DAILY RESET
// ============================================================================

void FlowSensor::resetDaily(int currentDay) {
    Serial.printf("[FLOW] Daily reset: currentDay=%d, lastResetDay was=%d, totalLiters was=%.2f L\n",
                  currentDay, lastResetDay_, totalLiters_);

    totalLiters_ = 0.0f;
    lastResetDay_ = currentDay;

    // Reset pulse count atomically
    noInterrupts();
    pulseCount_ = 0;
    interrupts();

    Serial.printf("[FLOW] Daily reset complete: totalLiters=%.2f L, lastResetDay=%d\n",
                  totalLiters_, lastResetDay_);
}

bool FlowSensor::needsMidnightReset(int currentDay) {
    return (currentDay != lastResetDay_);
}

// ============================================================================
// ISR HANDLER
// ============================================================================

void IRAM_ATTR FlowSensor::pulseISR() {
    pulseCount_++;
}
