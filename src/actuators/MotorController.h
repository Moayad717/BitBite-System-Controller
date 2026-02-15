#pragma once

#include <Arduino.h>

// ============================================================================
// MOTOR CONTROLLER
// ============================================================================
// Non-blocking motor control with FSM for pulsing
// States: IDLE → RUNNING → PULSING → STOPPED

enum MotorState {
    MOTOR_IDLE,      // Motor off, ready
    MOTOR_RUNNING,   // Motor running continuously
    MOTOR_PULSING,   // Motor pulsing (on/off cycles)
    MOTOR_STOPPED    // Motor stopped (post-operation)
};

class MotorController {
public:
    MotorController();

    // Initialize motor
    void begin(uint8_t relayPin, uint8_t sensePin);

    // Control methods
    void start();                           // Start motor (continuous)
    void stop();                            // Stop motor
    void startPulsing(uint16_t onTime, uint16_t offTime);  // Start pulsing mode
    void setPulseTimings(uint16_t onTime, uint16_t offTime);  // Update pulse timings

    // Update FSM (call from main loop)
    void update();

    // Status
    bool isRunning() const;
    bool isPulsing() const;
    MotorState getState() const;

    // Hardware sense (LOW=running, HIGH=stopped)
    bool isMotorSenseActive() const;

private:
    uint8_t relayPin_;
    uint8_t sensePin_;
    MotorState state_;

    // Pulsing control
    uint16_t pulseOnTime_;
    uint16_t pulseOffTime_;
    unsigned long lastPulseTime_;
    bool pulsePhase_;  // true=ON, false=OFF

    // Hardware control
    void turnOn();
    void turnOff();
};
