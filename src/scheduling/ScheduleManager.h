#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../config/DataStructures.h"
#include "../config/FeedingConfig.h"

// Forward declarations
class RTCManager;

// ============================================================================
// SCHEDULE MANAGER
// ============================================================================
// Manages feeding schedules with NVS flash persistence
// Parses JSON schedules from WiFi ESP and checks for matches

class ScheduleManager {
public:
    ScheduleManager();

    // Initialize with RTC dependency
    void begin(RTCManager* rtcManager);

    // Parse and cache schedules from JSON string
    bool parseSchedules(const char* jsonString);

    // Check if any schedule matches current time
    bool checkSchedules(float& amount);

    // Mark current schedule as completed (call ONLY after feeding starts successfully)
    void confirmScheduleCompleted();

    // Get cached schedule count
    int getScheduleCount() const;

    // Load/save schedules from/to flash
    void loadFromFlash();
    void saveToFlash();

    // Calculate hash for schedule verification
    unsigned long calculateHash(const char* jsonString);

    // Send hash confirmation via Serial2
    void sendHashConfirmation(unsigned long hash);

    // Send schedule status for remote debugging
    void sendScheduleStatus();

private:
    RTCManager* rtcManager_;
    Preferences preferences_;

    Schedule schedules_[MAX_SCHEDULES];
    int scheduleCount_;

    // Track which schedule matched (for confirming completion)
    int lastMatchedScheduleIndex_;

    // Helper to check if schedule matches current time
    bool scheduleMatches(const Schedule& schedule);
};
