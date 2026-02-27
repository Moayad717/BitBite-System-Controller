#include "StatusReporter.h"
#include "../config/FeedingConfig.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

StatusReporter::StatusReporter() {
    previousStatus_.lastUpdateTime = 0;
    lastSentIsFeeding_ = false;
}

// ============================================================================
// UPDATE METHODS
// ============================================================================

void StatusReporter::updateReadings(const SensorReadings& readings) {
    currentReadings_ = readings;
}

void StatusReporter::updateFeedingState(bool isFeeding, FeedingResult lastResult) {
    previousStatus_.isFeeding = isFeeding;
    previousStatus_.lastFeedComplete = lastResult;
}

void StatusReporter::updateFaults(uint8_t activeFaults) {
    previousStatus_.activeFaults = activeFaults;
}

// ============================================================================
// STATUS SENDING LOGIC
// ============================================================================

bool StatusReporter::shouldSendStatus() {
    unsigned long now = millis();

    // Always send if significant change
    if (hasSignificantChange()) {
        return true;
    }

    // Heartbeat: send every 5 minutes even if no change
    if (now - previousStatus_.lastUpdateTime >= STATUS_HEARTBEAT_INTERVAL) {
        return true;
    }

    return false;
}

void StatusReporter::sendStatus() {
    // Build JSON status message
    char message[256];
    snprintf(message, sizeof(message),
             "{\"isFeeding\":%s,\"foodLevel\":%.3f,\"humidity\":%.1f,\"temperature\":%.1f,\"waterFlow\":%.2f,\"activeFaults\":%d,\"lastFeedComplete\":%d}",
             currentReadings_.valid && previousStatus_.isFeeding ? "true" : "false",
             currentReadings_.foodLevel,
             currentReadings_.humidity,
             currentReadings_.temperature,
             currentReadings_.waterFlow,
             previousStatus_.activeFaults,
             previousStatus_.lastFeedComplete);

    // Debug: Log what we're sending
    Serial.printf("[STATUS] TX: %s\n", message);

    // Send via Serial2
    Serial2.println(message);

    // Update previous values
    previousStatus_.foodLevel = currentReadings_.foodLevel;
    previousStatus_.humidity = currentReadings_.humidity;
    previousStatus_.temperature = currentReadings_.temperature;
    previousStatus_.waterFlow = currentReadings_.waterFlow;
    lastSentIsFeeding_ = previousStatus_.isFeeding;
    previousStatus_.lastUpdateTime = millis();
}

void StatusReporter::forceSend() {
    sendStatus();
}

// ============================================================================
// CHANGE DETECTION
// ============================================================================

bool StatusReporter::hasSignificantChange() {
    // Check food level change
    if (fabs(currentReadings_.foodLevel - previousStatus_.foodLevel) >= STATUS_FOOD_LEVEL_DELTA) {
        return true;
    }

    // Check humidity change
    if (fabs(currentReadings_.humidity - previousStatus_.humidity) >= STATUS_HUMIDITY_DELTA) {
        return true;
    }

    // Check temperature change
    if (fabs(currentReadings_.temperature - previousStatus_.temperature) >= STATUS_TEMPERATURE_DELTA) {
        return true;
    }

    // Check water flow change
    if (fabs(currentReadings_.waterFlow - previousStatus_.waterFlow) >= STATUS_WATER_FLOW_DELTA) {
        return true;
    }

    // Check feeding state change
    if (previousStatus_.isFeeding != lastSentIsFeeding_) {
        return true;
    }

    return false;
}
