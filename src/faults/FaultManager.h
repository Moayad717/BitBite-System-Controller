#pragma once

#include <Arduino.h>
#include "../config/DataStructures.h"

// ============================================================================
// FAULT MANAGER
// ============================================================================
// Manages fault states and fault logging with circular buffer

class FaultManager {
public:
    FaultManager();

    // Set/clear faults
    void setFault(FaultCode fault, const char* name, float value = 0);
    void clearFault(FaultCode fault);
    void clearAllFaults();

    // Get active faults bitmask
    uint8_t getActiveFaults() const;

    // Check if specific fault is active
    bool hasFault(FaultCode fault) const;

    // Fault logging
    int getFaultLogCount() const;
    const FaultLog& getFaultLog(int index) const;

    // Send fault to WiFi ESP
    void sendFaultToSerial(const FaultLog& fault);

private:
    uint8_t activeFaults_;

    // Circular fault log buffer
    static const int MAX_FAULT_LOGS = 20;
    FaultLog faultLogs_[MAX_FAULT_LOGS];
    int faultLogCount_;
    int faultLogIndex_;

    // Add fault to log
    void logFault(FaultCode code, const char* name, float value);
};
