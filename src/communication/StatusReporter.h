#pragma once

#include <Arduino.h>
#include "../config/DataStructures.h"

// Forward declarations
class FeedingStateMachine;
class FaultManager;

// ============================================================================
// STATUS REPORTER
// ============================================================================
// Delta-based status reporting to WiFi ESP
// Only sends status if significant changes detected

class StatusReporter {
public:
    StatusReporter();

    // Update sensor readings
    void updateReadings(const SensorReadings& readings);

    // Update feeding state
    void updateFeedingState(bool isFeeding, FeedingResult lastResult);

    // Update active faults
    void updateFaults(uint8_t activeFaults);

    // Check if status should be sent (call every 10s)
    bool shouldSendStatus();

    // Send status to Serial2
    void sendStatus();

    // Force send (heartbeat)
    void forceSend();

private:
    SensorReadings currentReadings_;
    PreviousStatus previousStatus_;

    // Check if any value changed significantly
    bool hasSignificantChange();
};
