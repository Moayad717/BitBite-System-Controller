#include "FaultManager.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FaultManager::FaultManager()
    : activeFaults_(FAULT_NONE),
      faultLogCount_(0),
      faultLogIndex_(0) {
}

// ============================================================================
// FAULT CONTROL
// ============================================================================

void FaultManager::setFault(FaultCode fault, const char* name, float value) {
    // Only set if not already active (prevents duplicate logs)
    if (!(activeFaults_ & fault)) {
        activeFaults_ |= fault;
        logFault(fault, name, value);
        Serial.printf("[FAULT] SET: code=0x%02X, name=%s, value=%.2f\n", fault, name, value);
    }
}

void FaultManager::clearFault(FaultCode fault) {
    // Only log if fault was actually active
    if (activeFaults_ & fault) {
        activeFaults_ &= ~fault;
        Serial.printf("[FAULT] CLEARED: code=0x%02X\n", fault);
    }
}

void FaultManager::clearAllFaults() {
    if (activeFaults_ != FAULT_NONE) {
        Serial.printf("[FAULT] CLEARED ALL (was: 0x%02X)\n", activeFaults_);
        activeFaults_ = FAULT_NONE;
    }
}

// ============================================================================
// FAULT STATUS
// ============================================================================

uint8_t FaultManager::getActiveFaults() const {
    return activeFaults_;
}

bool FaultManager::hasFault(FaultCode fault) const {
    return (activeFaults_ & fault) != 0;
}

// ============================================================================
// FAULT LOGGING
// ============================================================================

void FaultManager::logFault(FaultCode code, const char* name, float value) {
    // Add to circular buffer
    FaultLog& log = faultLogs_[faultLogIndex_];
    log.timestamp = millis();
    log.code = code;
    log.value = value;
    strncpy(log.name, name, 31);
    log.name[31] = '\0';

    // Send to WiFi ESP
    sendFaultToSerial(log);

    // Update indices
    faultLogIndex_ = (faultLogIndex_ + 1) % MAX_FAULT_LOGS;
    if (faultLogCount_ < MAX_FAULT_LOGS) {
        faultLogCount_++;
    }
}

void FaultManager::sendFaultToSerial(const FaultLog& fault) {
    // Build JSON fault message
    char message[256];
    snprintf(message, sizeof(message),
             "FAULT:{\"timestamp\":%lu,\"code\":%d,\"name\":\"%s\",\"value\":%.2f}",
             fault.timestamp, fault.code, fault.name, fault.value);

    Serial2.println(message);
}

// ============================================================================
// FAULT LOG ACCESS
// ============================================================================

int FaultManager::getFaultLogCount() const {
    return faultLogCount_;
}

const FaultLog& FaultManager::getFaultLog(int index) const {
    return faultLogs_[index % MAX_FAULT_LOGS];
}
