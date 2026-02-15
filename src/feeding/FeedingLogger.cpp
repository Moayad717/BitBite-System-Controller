#include "FeedingLogger.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FeedingLogger::FeedingLogger() {
}

// ============================================================================
// LOGGING
// ============================================================================

void FeedingLogger::logFeeding(FeedingTrigger trigger, float amount, FeedingResult result, const char* timestamp) {
    // Only log successful or low-level feedings
    if (result == RESULT_SUCCESS || result == RESULT_LOW_LEVEL) {
        sendLog(timestamp, amount, trigger);
    }
}

void FeedingLogger::sendLog(const char* timestamp, float amount, FeedingTrigger trigger) {
    // Build JSON log message (weight as number to match WiFi ESP format)
    char logMessage[256];
    snprintf(logMessage, sizeof(logMessage),
             "LOG:{\"timestamp\":\"%s\",\"weight\":%.2f,\"type\":\"%s\"}",
             timestamp, amount, getTriggerString(trigger));

    // Send via Serial2 to WiFi ESP
    Serial2.println(logMessage);

    // Also print to Serial for debugging
    Serial.printf("[LOG] Feeding logged: %s\n", logMessage);
}

// ============================================================================
// HELPERS
// ============================================================================

const char* FeedingLogger::getTriggerString(FeedingTrigger trigger) {
    switch (trigger) {
        case TRIGGER_MANUAL:
            return "manual";
        case TRIGGER_SCHEDULE:
            return "schedule";
        default:
            return "unknown";
    }
}
