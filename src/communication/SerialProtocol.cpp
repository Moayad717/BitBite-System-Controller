#include "SerialProtocol.h"
#include "../scheduling/RTCManager.h"
#include "../scheduling/ScheduleManager.h"
#include "../feeding/FeedingStateMachine.h"
#include "../faults/FaultManager.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SerialProtocol::SerialProtocol()
    : rtcManager_(nullptr),
      scheduleManager_(nullptr),
      feedingMachine_(nullptr),
      faultManager_(nullptr),
      nameCallback_(nullptr),
      commandCallback_(nullptr),
      rxIndex_(0) {
    rxBuffer_[0] = '\0';
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void SerialProtocol::begin(RTCManager* rtcManager,
                           ScheduleManager* scheduleManager,
                           FeedingStateMachine* feedingMachine,
                           FaultManager* faultManager) {
    rtcManager_ = rtcManager;
    scheduleManager_ = scheduleManager;
    feedingMachine_ = feedingMachine;
    faultManager_ = faultManager;
}

void SerialProtocol::setNameUpdateCallback(NameUpdateCallback callback) {
    nameCallback_ = callback;
}

void SerialProtocol::setCommandCallback(CommandCallback callback) {
    commandCallback_ = callback;
}

// ============================================================================
// INCOMING MESSAGE PROCESSING
// ============================================================================

void SerialProtocol::processIncoming() {
    while (Serial2.available()) {
        char c = Serial2.read();

        if (c == '\n' || c == '\r') {
            if (rxIndex_ == 0) continue;  // Skip empty lines

            rxBuffer_[rxIndex_] = '\0';

            // Trim trailing whitespace
            while (rxIndex_ > 0 && (rxBuffer_[rxIndex_ - 1] == ' ' || rxBuffer_[rxIndex_ - 1] == '\t')) {
                rxBuffer_[--rxIndex_] = '\0';
            }

            Serial.printf("[SERIAL] RX: '%s'\n", rxBuffer_);

            // Parse message type and data
            if (strncmp(rxBuffer_, "SCHEDULES:", 10) == 0) {
                Serial.println("[SERIAL] Parsing schedules");
                handleSchedules(rxBuffer_ + 10);
            }
            else if (strncmp(rxBuffer_, "TIME:", 5) == 0) {
                Serial.println("[SERIAL] Syncing time");
                handleTime(rxBuffer_ + 5);
            }
            else if (strncmp(rxBuffer_, "NAME:", 5) == 0) {
                Serial.println("[SERIAL] Updating name");
                handleName(rxBuffer_ + 5);
            }
            else {
                Serial.println("[SERIAL] Processing as command");
                handleCommand(rxBuffer_);
            }

            rxIndex_ = 0;
            return;
        }

        // Append character if buffer has space
        if (rxIndex_ < MAX_MESSAGE_LEN - 1) {
            rxBuffer_[rxIndex_++] = c;
        } else {
            // Buffer overflow - discard entire message
            Serial.println("[SERIAL] Message too long - discarding");
            rxIndex_ = 0;
            // Drain remaining bytes until newline
            while (Serial2.available()) {
                if (Serial2.read() == '\n') break;
            }
            return;
        }
    }
}

// ============================================================================
// MESSAGE HANDLERS
// ============================================================================

void SerialProtocol::handleSchedules(const char* jsonData) {
    if (!scheduleManager_) {
        return;
    }

    // Parse and cache schedules
    scheduleManager_->parseSchedules(jsonData);
    // Note: ScheduleManager will send hash confirmation automatically
}

void SerialProtocol::handleTime(const char* timeString) {
    if (!rtcManager_) {
        return;
    }

    // Sync RTC from WiFi ESP (NTP synced time)
    rtcManager_->syncFromString(timeString);
}

void SerialProtocol::handleName(const char* name) {
    // Notify callback (for LCD display update)
    if (nameCallback_) {
        nameCallback_(name);
    }
}

void SerialProtocol::handleCommand(const char* command) {
    // Handle built-in commands
    if (strcmp(command, "CLEAR_FAULTS") == 0) {
        if (faultManager_) {
            faultManager_->clearAllFaults();
        }
    }
    else {
        // Pass to callback for handling (FEED_NOW, TARE, RESET_FLOW, etc.)
        if (commandCallback_) {
            commandCallback_(command);
        }
    }
}
