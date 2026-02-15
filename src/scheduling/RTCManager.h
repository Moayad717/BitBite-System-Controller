#pragma once

#include <Arduino.h>
#include <RTClib.h>

// ============================================================================
// RTC MANAGER (DS3231)
// ============================================================================
// Manages DS3231 Real-Time Clock with time sync from WiFi ESP

class RTCManager {
public:
    RTCManager();

    // Initialize RTC
    bool begin();

    // Get current time
    DateTime now();

    // Sync time from string (from WiFi ESP)
    bool syncFromString(const char* timeString);  // Format: "YYYY-MM-DD HH:MM:SS"

    // Get formatted timestamp
    void getTimestamp(char* buffer, size_t bufferSize);

    // Time component getters
    int getHour();
    int getMinute();
    int getDayOfWeek();  // 0=Sunday, 6=Saturday
    int getDayOfMonth();
    uint32_t getCurrentDate();  // Get current date in YYYYMMDD format

    // Validation
    bool isValid();
    bool needsSync();  // True if RTC time seems invalid

private:
    RTC_DS3231 rtc_;
    bool initialized_;
    DateTime lastValidTime_;

    // Parse time string
    bool parseTimeString(const char* timeString, int& year, int& month, int& day,
                        int& hour, int& minute, int& second);
};
