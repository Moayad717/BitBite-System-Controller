#include "MotorController.h"
#include "../config/FeedingConfig.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

MotorController::MotorController()
    : relayPin_(0),
      sensePin_(0),
      state_(MOTOR_IDLE),
      pulseOnTime_(FEEDING_PULSE_ON_TIME),
      pulseOffTime_(FEEDING_PULSE_OFF_TIME),
      lastPulseTime_(0),
      pulsePhase_(false) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void MotorController::begin(uint8_t relayPin, uint8_t sensePin) {
    relayPin_ = relayPin;
    sensePin_ = sensePin;

    // Setup pins
    pinMode(relayPin_, OUTPUT);
    pinMode(sensePin_, INPUT_PULLUP);

    // Ensure motor is off
    turnOff();
    state_ = MOTOR_IDLE;
}

// ============================================================================
// CONTROL METHODS
// ============================================================================

void MotorController::start() {
    if (state_ == MOTOR_IDLE || state_ == MOTOR_STOPPED) {
        turnOn();
        state_ = MOTOR_RUNNING;
    }
}

void MotorController::stop() {
    turnOff();
    state_ = MOTOR_STOPPED;
}

void MotorController::startPulsing(uint16_t onTime, uint16_t offTime) {
    pulseOnTime_ = onTime;
    pulseOffTime_ = offTime;

    // Start with ON phase
    turnOn();
    pulsePhase_ = true;
    lastPulseTime_ = millis();
    state_ = MOTOR_PULSING;
}

void MotorController::setPulseTimings(uint16_t onTime, uint16_t offTime) {
    pulseOnTime_ = onTime;
    pulseOffTime_ = offTime;
}

// ============================================================================
// FSM UPDATE (NON-BLOCKING)
// ============================================================================

void MotorController::update() {
    if (state_ != MOTOR_PULSING) {
        return;  // Only update if pulsing
    }

    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastPulseTime_;

    if (pulsePhase_) {
        // Currently ON - check if time to turn OFF
        if (elapsed >= pulseOnTime_) {
            turnOff();
            pulsePhase_ = false;
            lastPulseTime_ = currentTime;
        }
    } else {
        // Currently OFF - check if time to turn ON
        if (elapsed >= pulseOffTime_) {
            turnOn();
            pulsePhase_ = true;
            lastPulseTime_ = currentTime;
        }
    }
}

// ============================================================================
// STATUS METHODS
// ============================================================================

bool MotorController::isRunning() const {
    return (state_ == MOTOR_RUNNING) || (state_ == MOTOR_PULSING && pulsePhase_);
}

bool MotorController::isPulsing() const {
    return (state_ == MOTOR_PULSING);
}

MotorState MotorController::getState() const {
    return state_;
}

bool MotorController::isMotorSenseActive() const {
    // LOW = motor running, HIGH = motor stopped
    return (digitalRead(sensePin_) == LOW);
}

// ============================================================================
// HARDWARE CONTROL
// ============================================================================

void MotorController::turnOn() {
    digitalWrite(relayPin_, LOW);
}

void MotorController::turnOff() {
    digitalWrite(relayPin_, HIGH);
}
