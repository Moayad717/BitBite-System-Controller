#pragma once

#include <Arduino.h>

// ============================================================================
// CONSTANTS
// ============================================================================

// Sentinel value for sensor errors (NaN, disconnected, out of range)
#define SENSOR_ERROR_VALUE -999.0f

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Schedule Structure
struct Schedule {
    char time[6];              // "HH:MM" format
    uint8_t daysOfWeek;        // Bitmask: bit 0=Sunday, bit 1=Monday, ..., bit 6=Saturday
    float amount;              // kg - amount to dispense
    bool enabled;              // Is this schedule active
    uint32_t lastExecutionDate; // YYYYMMDD format - tracks when this schedule last executed

    Schedule() : daysOfWeek(0), amount(0.0f), enabled(false), lastExecutionDate(0) {
        strcpy(time, "00:00");
    }
};

// Fault Codes (bitmask)
enum FaultCode {
    FAULT_NONE           = 0x00,
    FAULT_MOTOR_STUCK    = 0x02,  // Motor timeout/stuck
    FAULT_WATER_LEAK     = 0x04,  // Excessive water flow detected
    FAULT_WEIGHT_SENSOR  = 0x08,  // HX711 not responding
    FAULT_RTC_FAIL       = 0x10,  // RTC time invalid
    FAULT_DHT_FAIL       = 0x20,  // DHT sensor failure
    FAULT_SCHEDULE_FAILED = 0x40  // Scheduled feeding failed to start
};

// Fault Log Entry
struct FaultLog {
    unsigned long timestamp;    // When fault occurred
    uint8_t code;              // Fault code
    char name[32];             // Fault name
    float value;               // Associated value (if any)

    FaultLog() : timestamp(0), code(0), value(0.0f) {
        strcpy(name, "Unknown");
    }
};

// Feeding State Machine States
enum FeedingState {
    FEEDING_IDLE,           // Not feeding
    FEEDING_STARTING,       // Pre-feed checks
    FEEDING_DISPENSING,     // Motor running, continuous
    FEEDING_PULSING,        // Motor pulsing (near target)
    FEEDING_FINISHING,      // Post-feed cleanup
    FEEDING_COOLDOWN_STATE  // Cooldown period
};

// Feeding Trigger Types
enum FeedingTrigger {
    TRIGGER_NONE,
    TRIGGER_MANUAL,         // Manual command (FEED_NOW)
    TRIGGER_SCHEDULE        // Scheduled feeding
};

// Feeding Result
enum FeedingResult {
    RESULT_NONE = 0,
    RESULT_SUCCESS = 1,     // Feeding completed successfully
    RESULT_LOW_LEVEL = 2,   // Low food level prevented feeding
    RESULT_TIMEOUT = 3,     // Feeding timed out
    RESULT_ERROR = 4        // Other error
};

// Previous Status (for delta detection)
struct PreviousStatus {
    float foodLevel;
    float humidity;
    float temperature;
    float waterFlow;
    bool isFeeding;
    uint8_t activeFaults;
    uint8_t lastFeedComplete;
    unsigned long lastUpdateTime;

    PreviousStatus() :
        foodLevel(0),
        humidity(0),
        temperature(0),
        waterFlow(0),
        isFeeding(false),
        activeFaults(0),
        lastFeedComplete(0),
        lastUpdateTime(0) {}
};

// Sensor Readings
struct SensorReadings {
    float foodLevel;        // kg
    float temperature;      // °C
    float humidity;         // %
    float waterFlow;        // L (daily total)
    bool valid;             // Are readings valid?

    SensorReadings() :
        foodLevel(-999),
        temperature(-999),
        humidity(-999),
        waterFlow(0),
        valid(false) {}
};
