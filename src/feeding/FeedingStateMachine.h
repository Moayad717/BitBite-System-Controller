#pragma once

#include <Arduino.h>
#include "../config/DataStructures.h"

// Forward declarations
class MotorController;
class WeightSensor;

// ============================================================================
// FEEDING STATE MACHINE
// ============================================================================
// Non-blocking state machine for feeding control
// States: IDLE → STARTING → DISPENSING → PULSING → FINISHING → COOLDOWN
// Handles both manual and scheduled feeding with different targets

class FeedingStateMachine {
public:
    FeedingStateMachine();

    // Initialize with dependencies
    void begin(MotorController* motor, WeightSensor* weightSensor);

    // Start feeding
    bool startFeeding(FeedingTrigger trigger, float targetAmount = 0);

    // Stop feeding
    void stopFeeding(FeedingResult result);

    // Update FSM (call from main loop)
    void update();

    // Status
    bool isFeeding() const;
    FeedingState getState() const;
    FeedingTrigger getTrigger() const;
    FeedingResult getLastResult() const;
    float getDispensedAmount() const;
    float getWeightBefore() const;  // Get weight reading before feeding attempt

    // Set cooldown callback (called when cooldown completes)
    typedef void (*CooldownCompleteCallback)();
    void setCooldownCallback(CooldownCompleteCallback callback);

private:
    // Dependencies
    MotorController* motor_;
    WeightSensor* weightSensor_;

    // State
    FeedingState state_;
    FeedingTrigger trigger_;
    FeedingResult lastResult_;

    // Feeding parameters
    float targetAmount_;        // kg - target to dispense
    float weightBefore_;        // kg - weight before feeding
    float weightAfter_;         // kg - weight after feeding (captured when motor stops)
    float pulseThreshold_;      // kg - when to start pulsing (manual feed only)
    unsigned long feedingStartTime_;
    unsigned long cooldownStartTime_;
    unsigned long settleStartTime_;  // When motor stopped for settle phase

    // Callbacks
    CooldownCompleteCallback cooldownCallback_;

    // State handlers
    void handleIdle();
    void handleStarting();
    void handleDispensing();
    void handlePulsing();
    void handleSettling();
    void handleFinishing();
    void handleCooldown();

    // Helpers
    float getCurrentWeight() const;
    float getCurrentWeightFast() const;
    float getDispensedSinceStart() const;
    bool isTimeoutReached();
    bool isTargetReached();
    bool isEffectiveTargetReached();
    bool shouldStartPulsing();
    uint16_t getCurrentPulseOnTime() const;
};
