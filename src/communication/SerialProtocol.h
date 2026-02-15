#pragma once

#include <Arduino.h>

// Forward declarations
class RTCManager;
class ScheduleManager;
class FeedingStateMachine;
class FaultManager;

// ============================================================================
// SERIAL PROTOCOL (Serial2 ↔ WiFi ESP)
// ============================================================================
// Handles bidirectional communication with WiFi ESP
// RX: SCHEDULES, TIME, NAME, Commands (FEED_NOW, TARE, etc.)
// TX: Status updates (handled by StatusReporter)

class SerialProtocol {
public:
    SerialProtocol();

    // Initialize with dependencies
    void begin(RTCManager* rtcManager,
               ScheduleManager* scheduleManager,
               FeedingStateMachine* feedingMachine,
               FaultManager* faultManager);

    // Process incoming Serial2 data (call from main loop)
    void processIncoming();

    // Set device name callback
    typedef void (*NameUpdateCallback)(const char* name);
    void setNameUpdateCallback(NameUpdateCallback callback);

    // Set command callback (for FEED_NOW, TARE, etc.)
    typedef void (*CommandCallback)(const char* command);
    void setCommandCallback(CommandCallback callback);

private:
    RTCManager* rtcManager_;
    ScheduleManager* scheduleManager_;
    FeedingStateMachine* feedingMachine_;
    FaultManager* faultManager_;

    NameUpdateCallback nameCallback_;
    CommandCallback commandCallback_;

    // Receive buffer (fixed size to avoid heap fragmentation from String)
    static const size_t MAX_MESSAGE_LEN = 8192;
    char rxBuffer_[MAX_MESSAGE_LEN];
    size_t rxIndex_;

    // Message handlers
    void handleSchedules(const char* jsonData);
    void handleTime(const char* timeString);
    void handleName(const char* name);
    void handleCommand(const char* command);
};
