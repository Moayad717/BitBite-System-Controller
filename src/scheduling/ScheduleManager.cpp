#include "ScheduleManager.h"
#include "RTCManager.h"
#include <ArduinoJson.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ScheduleManager::ScheduleManager()
    : rtcManager_(nullptr),
      scheduleCount_(0),
      lastMatchedScheduleIndex_(-1) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ScheduleManager::begin(RTCManager* rtcManager) {
    rtcManager_ = rtcManager;

    // Load cached schedules from flash
    loadFromFlash();
}

// ============================================================================
// SCHEDULE PARSING
// ============================================================================

bool ScheduleManager::parseSchedules(const char* jsonString) {
    Serial.printf("[SCHEDULE] Parsing schedules, JSON length: %d\n", strlen(jsonString));
    Serial.printf("[SCHEDULE] Free heap before parsing: %d bytes\n", ESP.getFreeHeap());

    // Empty or null JSON - clear schedules
    if (!jsonString || strlen(jsonString) == 0 || strcmp(jsonString, "{}") == 0) {
        Serial.println("[SCHEDULE] Empty JSON - clearing schedules");
        scheduleCount_ = 0;
        saveToFlash();
        sendHashConfirmation(0);
        return true;
    }

    // Reject oversized JSON to prevent OOM
    size_t jsonLen = strlen(jsonString);
    const size_t capacity = 8192;
    if (jsonLen >= capacity) {
        Serial.printf("[SCHEDULE] JSON too large (%d bytes) - rejecting\n", jsonLen);
        return false;
    }

    Serial.printf("[SCHEDULE] Attempting to allocate %d bytes for JSON parsing\n", capacity);

    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.printf("[SCHEDULE] JSON parse error: %s\n", error.c_str());
        Serial.printf("[SCHEDULE] Free heap after error: %d bytes\n", ESP.getFreeHeap());
        return false;
    }

    Serial.println("[SCHEDULE] JSON parsed successfully");

    // Clear existing schedules
    scheduleCount_ = 0;

    // Parse each schedule
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        if (scheduleCount_ >= MAX_SCHEDULES) {
            Serial.println("[SCHEDULE] Max schedules reached - skipping remaining");
            break;
        }

        JsonObject scheduleObj = kv.value().as<JsonObject>();

        // Extract fields
        const char* time = scheduleObj["time"];
        JsonArray daysArray = scheduleObj["days"];  // Array of days [0,1,2,3,4,5,6]
        float amount = scheduleObj["amount"];
        bool enabled = scheduleObj["enabled"] | true;  // Default true

        // Convert amount from grams to kg
        float amountKg = amount / 1000.0f;

        if (time && daysArray.size() > 0 && amount > 0) {
            // Convert days array to bitmask
            uint8_t daysBitmask = 0;
            for (JsonVariant dayVariant : daysArray) {
                int day = dayVariant.as<int>();
                if (day >= 0 && day <= 6) {
                    daysBitmask |= (1 << day);  // Set bit for this day
                }
            }

            // Store ONE schedule (not expanded per day)
            Schedule& schedule = schedules_[scheduleCount_];
            strncpy(schedule.time, time, 5);
            schedule.time[5] = '\0';
            schedule.daysOfWeek = daysBitmask;
            schedule.amount = amountKg;
            schedule.enabled = enabled;
            schedule.lastExecutionDate = 0;  // Not executed yet

            Serial.printf("[SCHEDULE] Parsed #%d: time=%s, days=0x%02X, amount=%.3f kg, enabled=%d\n",
                          scheduleCount_, schedule.time, schedule.daysOfWeek, schedule.amount, schedule.enabled);

            scheduleCount_++;
        }
    }

    Serial.printf("[SCHEDULE] Total schedules parsed: %d\n", scheduleCount_);

    // Calculate and send hash BEFORE flash write so WiFi ESP gets the
    // confirmation immediately, without waiting for the slow NVS erase
    unsigned long hash = calculateHash(jsonString);
    sendHashConfirmation(hash);

    // Save to flash (slow NVS erase + write - happens after confirmation sent)
    saveToFlash();

    return true;
}

// ============================================================================
// SCHEDULE CHECKING
// ============================================================================

bool ScheduleManager::checkSchedules(float& amount) {
    if (!rtcManager_ || scheduleCount_ == 0) {
        return false;
    }

    // Check each schedule
    for (int i = 0; i < scheduleCount_; i++) {
        if (scheduleMatches(schedules_[i])) {
            amount = schedules_[i].amount;
            lastMatchedScheduleIndex_ = i;  // Store which schedule matched
            return true;  // Caller must call confirmScheduleCompleted() if feed starts
        }
    }

    return false;
}

void ScheduleManager::confirmScheduleCompleted() {
    // Mark schedule as executed for today
    if (rtcManager_ && lastMatchedScheduleIndex_ >= 0 && lastMatchedScheduleIndex_ < scheduleCount_) {
        uint32_t today = rtcManager_->getCurrentDate();
        schedules_[lastMatchedScheduleIndex_].lastExecutionDate = today;

        // Save to flash immediately to survive reboots
        saveToFlash();

        Serial.printf("[SCHEDULE] Confirmed completed: %s on date %lu\n",
                      schedules_[lastMatchedScheduleIndex_].time, today);

        // Reset matched index
        lastMatchedScheduleIndex_ = -1;
    }
}

// ============================================================================
// FLASH PERSISTENCE
// ============================================================================

void ScheduleManager::loadFromFlash() {
    preferences_.begin("schedules", true);  // Read-only

    scheduleCount_ = preferences_.getInt("count", 0);

    if (scheduleCount_ > MAX_SCHEDULES) {
        scheduleCount_ = 0;
    }

    for (int i = 0; i < scheduleCount_; i++) {
        char key[16];
        snprintf(key, sizeof(key), "sched_%d", i);

        size_t len = preferences_.getBytes(key, &schedules_[i], sizeof(Schedule));
        if (len != sizeof(Schedule)) {
            // Corrupted data - reset
            scheduleCount_ = 0;
            break;
        }
    }

    preferences_.end();
}

void ScheduleManager::saveToFlash() {
    preferences_.begin("schedules", false);  // Read-write

    // Clear entire namespace to remove old entries (old format had 140+ keys)
    preferences_.clear();

    preferences_.putInt("count", scheduleCount_);

    for (int i = 0; i < scheduleCount_; i++) {
        char key[16];
        snprintf(key, sizeof(key), "sched_%d", i);
        preferences_.putBytes(key, &schedules_[i], sizeof(Schedule));
    }

    preferences_.end();
}

// ============================================================================
// HASH CALCULATION
// ============================================================================

unsigned long ScheduleManager::calculateHash(const char* jsonString) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < strlen(jsonString); i++) {
        hash = ((hash << 5) + hash) + jsonString[i];
    }
    return hash;
}

void ScheduleManager::sendHashConfirmation(unsigned long hash) {
    char message[32];
    snprintf(message, sizeof(message), "SCHEDULE_HASH:%lu", hash);
    Serial2.println(message);
    Serial.printf("[SCHEDULE] Hash sent: %s\n", message);
}

void ScheduleManager::sendScheduleStatus() {
    if (!rtcManager_) {
        Serial2.println("SCHEDULE_STATUS:ERROR - No RTC");
        return;
    }

    uint32_t today = rtcManager_->getCurrentDate();
    int currentHour = rtcManager_->getHour();
    int currentMinute = rtcManager_->getMinute();
    int currentDay = rtcManager_->getDayOfWeek();

    // Send current time and date
    Serial2.printf("SCHEDULE_STATUS:Date=%lu,Time=%02d:%02d,Day=%d,Count=%d\n",
                   today, currentHour, currentMinute, currentDay, scheduleCount_);

    // Send status of each schedule
    for (int i = 0; i < scheduleCount_; i++) {
        const Schedule& sched = schedules_[i];

        // Check if this schedule applies today
        bool appliesToday = (sched.daysOfWeek & (1 << currentDay));
        bool executedToday = (sched.lastExecutionDate == today);

        Serial2.printf("SCHEDULE_ITEM:%d,Time=%s,Days=0x%02X,Amount=%.3f,Enabled=%d,AppliesNow=%d,ExecutedToday=%d,LastExec=%lu\n",
                       i, sched.time, sched.daysOfWeek, sched.amount, sched.enabled,
                       appliesToday, executedToday, sched.lastExecutionDate);

        Serial.printf("[SCHEDULE] Item %d: %s, applies=%d, executed=%d\n",
                      i, sched.time, appliesToday, executedToday);
    }

    Serial2.println("SCHEDULE_STATUS:END");
}

// ============================================================================
// GETTERS
// ============================================================================

int ScheduleManager::getScheduleCount() const {
    return scheduleCount_;
}

// ============================================================================
// HELPERS
// ============================================================================

bool ScheduleManager::scheduleMatches(const Schedule& schedule) {
    if (!schedule.enabled || !rtcManager_) {
        return false;
    }

    // Parse schedule time
    int schedHour, schedMinute;
    if (sscanf(schedule.time, "%d:%d", &schedHour, &schedMinute) != 2) {
        Serial.printf("[SCHEDULE] Failed to parse time: %s\n", schedule.time);
        return false;
    }

    // Get current time components
    int currentHour = rtcManager_->getHour();
    int currentMinute = rtcManager_->getMinute();
    int currentDay = rtcManager_->getDayOfWeek();
    uint32_t currentDate = rtcManager_->getCurrentDate();

    // Check time match first (most common failure)
    if (currentHour != schedHour || currentMinute != schedMinute) {
        return false;
    }

    // Check if today's day is in the bitmask
    if (!(schedule.daysOfWeek & (1 << currentDay))) {
        Serial.printf("[SCHEDULE] Day not in schedule: current=%d, bitmask=0x%02X\n",
                      currentDay, schedule.daysOfWeek);
        return false;
    }

    // Check if already executed today
    if (schedule.lastExecutionDate == currentDate) {
        Serial.printf("[SCHEDULE] Already executed today: %s (date=%lu)\n",
                      schedule.time, currentDate);
        return false;
    }

    Serial.printf("[SCHEDULE] MATCH FOUND! %s on day %d (date=%lu)\n",
                  schedule.time, currentDay, currentDate);
    return true;
}
