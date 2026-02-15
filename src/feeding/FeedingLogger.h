#pragma once

#include <Arduino.h>
#include "../config/DataStructures.h"

// ============================================================================
// FEEDING LOGGER
// ============================================================================
// Logs feeding events and sends to WiFi ESP

class FeedingLogger {
public:
    FeedingLogger();

    // Log a feeding event
    void logFeeding(FeedingTrigger trigger, float amount, FeedingResult result, const char* timestamp);

    // Send log via Serial2
    void sendLog(const char* timestamp, float amount, FeedingTrigger trigger);

private:
    // Format trigger as string
    const char* getTriggerString(FeedingTrigger trigger);
};
