#include "RTCManager.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

RTCManager::RTCManager()
    : initialized_(false),
      lastValidTime_(DateTime(2020, 1, 1, 0, 0, 0)) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool RTCManager::begin() {
    if (!rtc_.begin()) {
        initialized_ = false;
        return false;
    }

    // Check if RTC lost power
    if (rtc_.lostPower()) {
        // Set to a default time (will be synced from WiFi ESP later)
        rtc_.adjust(DateTime(2020, 1, 1, 0, 0, 0));
    }

    // Read initial time
    lastValidTime_ = rtc_.now();
    initialized_ = true;

    return true;
}

// ============================================================================
// TIME READING
// ============================================================================

DateTime RTCManager::now() {
    if (!initialized_) {
        return lastValidTime_;
    }

    DateTime current = rtc_.now();

    // Validate time is reasonable (year > 2020)
    if (current.year() >= 2020) {
        lastValidTime_ = current;
        return current;
    }

    return lastValidTime_;
}

// ============================================================================
// TIME SYNCHRONIZATION
// ============================================================================

bool RTCManager::syncFromString(const char* timeString) {
    if (!initialized_) {
        return false;
    }

    // Parse time string: "YYYY-MM-DD HH:MM:SS"
    int year, month, day, hour, minute, second;
    if (!parseTimeString(timeString, year, month, day, hour, minute, second)) {
        return false;
    }

    // Create DateTime object
    DateTime newTime(year, month, day, hour, minute, second);

    // Adjust RTC
    rtc_.adjust(newTime);
    lastValidTime_ = newTime;

    return true;
}

// ============================================================================
// FORMATTED OUTPUT
// ============================================================================

void RTCManager::getTimestamp(char* buffer, size_t bufferSize) {
    DateTime current = now();

    snprintf(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d",
             current.year(), current.month(), current.day(),
             current.hour(), current.minute(), current.second());
}

// ============================================================================
// TIME COMPONENT GETTERS
// ============================================================================

int RTCManager::getHour() {
    return now().hour();
}

int RTCManager::getMinute() {
    return now().minute();
}

int RTCManager::getDayOfWeek() {
    return now().dayOfTheWeek();  // 0=Sunday
}

int RTCManager::getDayOfMonth() {
    return now().day();
}

uint32_t RTCManager::getCurrentDate() {
    DateTime current = now();
    // Return date in YYYYMMDD format
    return (current.year() * 10000) + (current.month() * 100) + current.day();
}

// ============================================================================
// VALIDATION
// ============================================================================

bool RTCManager::isValid() {
    if (!initialized_) {
        return false;
    }

    DateTime current = now();
    return (current.year() >= 2020);
}

bool RTCManager::needsSync() {
    return !isValid();
}

// ============================================================================
// HELPERS
// ============================================================================

bool RTCManager::parseTimeString(const char* timeString, int& year, int& month, int& day,
                                 int& hour, int& minute, int& second) {
    // Expected format: "YYYY-MM-DD HH:MM:SS"
    // Example: "2024-11-27 13:45:30"

    int parsed = sscanf(timeString, "%d-%d-%d %d:%d:%d",
                       &year, &month, &day, &hour, &minute, &second);

    if (parsed != 6) {
        return false;
    }

    // Validate ranges
    if (year < 2020 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;

    return true;
}
