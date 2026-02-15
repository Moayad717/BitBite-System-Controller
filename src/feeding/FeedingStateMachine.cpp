#include "FeedingStateMachine.h"
#include "../actuators/MotorController.h"
#include "../sensors/WeightSensor.h"
#include "../config/DataStructures.h"
#include "../config/FeedingConfig.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FeedingStateMachine::FeedingStateMachine()
    : motor_(nullptr),
      weightSensor_(nullptr),
      state_(FEEDING_IDLE),
      trigger_(TRIGGER_NONE),
      lastResult_(RESULT_NONE),
      targetAmount_(0),
      weightBefore_(0),
      weightAfter_(0),
      pulseThreshold_(0),
      feedingStartTime_(0),
      cooldownStartTime_(0),
      cooldownCallback_(nullptr) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void FeedingStateMachine::begin(MotorController* motor, WeightSensor* weightSensor) {
    motor_ = motor;
    weightSensor_ = weightSensor;
}

void FeedingStateMachine::setCooldownCallback(CooldownCompleteCallback callback) {
    cooldownCallback_ = callback;
}

// ============================================================================
// START FEEDING
// ============================================================================

bool FeedingStateMachine::startFeeding(FeedingTrigger trigger, float targetAmount) {
    Serial.printf("[FSM] startFeeding() called, trigger=%d, state=%d\n", trigger, state_);

    // Can only start from IDLE
    if (state_ != FEEDING_IDLE) {
        Serial.println("[FSM] ERROR: Not in IDLE state");
        return false;
    }

    // Set trigger and target
    trigger_ = trigger;

    if (trigger == TRIGGER_MANUAL) {
        targetAmount_ = FEEDING_MANUAL_TARGET;
        pulseThreshold_ = FEEDING_MANUAL_PULSE_THRESHOLD;
        Serial.printf("[FSM] Manual feeding: target=%.3f kg\n", targetAmount_);
    } else if (trigger == TRIGGER_SCHEDULE) {
        targetAmount_ = targetAmount;  // From schedule
        pulseThreshold_ = targetAmount * FEEDING_SCHEDULE_PULSE_THRESHOLD;  // 50% of target
        Serial.printf("[FSM] Scheduled feeding: target=%.3f kg\n", targetAmount_);
    } else {
        Serial.println("[FSM] ERROR: Invalid trigger");
        return false;  // Invalid trigger
    }

    // Record starting weight
    weightBefore_ = getCurrentWeight();
    Serial.printf("[FSM] Current weight: %.3f kg (threshold: %.3f kg)\n",
                  weightBefore_, FEEDING_LOW_LEVEL_THRESHOLD);

    // Reject if sensor returned an error
    if (weightBefore_ <= SENSOR_ERROR_VALUE) {
        Serial.println("[FSM] ERROR: Weight sensor error - cannot feed");
        lastResult_ = RESULT_ERROR;
        return false;
    }

    // Check if enough food available
    if (weightBefore_ < FEEDING_LOW_LEVEL_THRESHOLD) {
        Serial.println("[FSM] ERROR: Low food level - cannot feed");
        lastResult_ = RESULT_LOW_LEVEL;
        return false;
    }

    // Check low level for scheduled feeding
    if (trigger == TRIGGER_SCHEDULE && weightBefore_ < targetAmount_) {
        Serial.println("[FSM] ERROR: Not enough food for scheduled amount");
        lastResult_ = RESULT_LOW_LEVEL;
        return false;
    }

    // Start feeding
    state_ = FEEDING_STARTING;
    feedingStartTime_ = millis();
    lastResult_ = RESULT_NONE;

    Serial.println("[FSM] Feeding started successfully");
    return true;
}

// ============================================================================
// STOP FEEDING
// ============================================================================

void FeedingStateMachine::stopFeeding(FeedingResult result) {
    // Stop motor
    if (motor_) {
        motor_->stop();
    }

    lastResult_ = result;
    state_ = FEEDING_FINISHING;
}

// ============================================================================
// FSM UPDATE (NON-BLOCKING)
// ============================================================================

void FeedingStateMachine::update() {
    switch (state_) {
        case FEEDING_IDLE:
            handleIdle();
            break;

        case FEEDING_STARTING:
            handleStarting();
            break;

        case FEEDING_DISPENSING:
            handleDispensing();
            break;

        case FEEDING_PULSING:
            handlePulsing();
            break;

        case FEEDING_FINISHING:
            handleFinishing();
            break;

        case FEEDING_COOLDOWN_STATE:
            handleCooldown();
            break;
    }

    // Always update motor (for pulsing)
    if (motor_) {
        motor_->update();
    }
}

// ============================================================================
// STATE HANDLERS
// ============================================================================

void FeedingStateMachine::handleIdle() {
    // Nothing to do in idle
}

void FeedingStateMachine::handleStarting() {
    // Start motor in continuous mode
    if (motor_) {
        motor_->start();
    }

    // Move to dispensing
    state_ = FEEDING_DISPENSING;
}

void FeedingStateMachine::handleDispensing() {
    // Check timeout
    if (isTimeoutReached()) {
        stopFeeding(RESULT_TIMEOUT);
        return;
    }

    // Check if target reached
    if (isTargetReached()) {
        stopFeeding(RESULT_SUCCESS);
        return;
    }

    // Check if should start pulsing
    if (shouldStartPulsing()) {
        if (motor_) {
            motor_->startPulsing(FEEDING_PULSE_ON_TIME, FEEDING_PULSE_OFF_TIME);
        }
        state_ = FEEDING_PULSING;
    }
}

void FeedingStateMachine::handlePulsing() {
    // Check timeout
    if (isTimeoutReached()) {
        stopFeeding(RESULT_TIMEOUT);
        return;
    }

    // Check if target reached
    if (isTargetReached()) {
        stopFeeding(RESULT_SUCCESS);
        return;
    }

    // Motor update happens in main update() call
}

void FeedingStateMachine::handleFinishing() {
    // Ensure motor is stopped
    if (motor_) {
        motor_->stop();
    }

    // Capture final weight after motor stops (before it can stabilize further)
    // This gives us the best estimate of actual amount dispensed
    weightAfter_ = getCurrentWeight();
    Serial.printf("[FSM] Final weight captured: %.3f kg (dispensed: %.3f kg)\n",
                  weightAfter_, weightBefore_ - weightAfter_);

    // Move to cooldown
    state_ = FEEDING_COOLDOWN_STATE;
    cooldownStartTime_ = millis();
}

void FeedingStateMachine::handleCooldown() {
    unsigned long elapsed = millis() - cooldownStartTime_;

    if (elapsed >= FEEDING_COOLDOWN) {
        // Notify callback BEFORE resetting state (so it can read trigger/result)
        if (cooldownCallback_) {
            cooldownCallback_();
        }

        // Cooldown complete - reset state
        state_ = FEEDING_IDLE;
        trigger_ = TRIGGER_NONE;
        lastResult_ = RESULT_NONE;  // Reset result so status reports 0
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

float FeedingStateMachine::getCurrentWeight() const {
    if (!weightSensor_) {
        Serial.println("[FSM] ERROR: weightSensor_ is NULL!");
        return SENSOR_ERROR_VALUE;
    }
    float weight = weightSensor_->readWeight();
    Serial.printf("[FSM] getCurrentWeight() = %.3f kg\n", weight);
    return weight;
}

float FeedingStateMachine::getDispensedSinceStart() const {
    return weightBefore_ - getCurrentWeight();
}

bool FeedingStateMachine::isTimeoutReached() {
    return (millis() - feedingStartTime_) >= FEEDING_TIMEOUT;
}

bool FeedingStateMachine::isTargetReached() {
    float dispensed = getDispensedSinceStart();

    // For manual: need at least FEEDING_MIN_DISPENSE
    if (trigger_ == TRIGGER_MANUAL) {
        return dispensed >= FEEDING_MIN_DISPENSE;
    }

    // For schedule: target amount
    return dispensed >= targetAmount_;
}

bool FeedingStateMachine::shouldStartPulsing() {
    float dispensed = getDispensedSinceStart();
    Serial.printf("[FSM] shouldStartPulsing: dispensed=%.3f, threshold=%.3f\n",
                  dispensed, pulseThreshold_);
    return dispensed >= pulseThreshold_;
}

// ============================================================================
// STATUS METHODS
// ============================================================================

bool FeedingStateMachine::isFeeding() const {
    return (state_ == FEEDING_STARTING ||
            state_ == FEEDING_DISPENSING ||
            state_ == FEEDING_PULSING);
}

FeedingState FeedingStateMachine::getState() const {
    return state_;
}

FeedingTrigger FeedingStateMachine::getTrigger() const {
    return trigger_;
}

FeedingResult FeedingStateMachine::getLastResult() const {
    return lastResult_;
}

float FeedingStateMachine::getDispensedAmount() const {
    if (state_ == FEEDING_IDLE || state_ == FEEDING_COOLDOWN_STATE) {
        // Use captured final weight instead of live reading (avoids unstable sensor values)
        return weightBefore_ - weightAfter_;
    }
    return getDispensedSinceStart();
}

float FeedingStateMachine::getWeightBefore() const {
    return weightBefore_;
}
