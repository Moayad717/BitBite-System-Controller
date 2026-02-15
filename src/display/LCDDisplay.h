#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// LCD DISPLAY (16x2 I2C)
// ============================================================================
// Manages 16x2 LCD display with alternating screens
// Screen 1: Weight + Time
// Screen 2: Weight + Device Name

class LCDDisplay {
public:
    LCDDisplay();

    // Initialize display
    bool begin(uint8_t address, uint8_t cols, uint8_t rows);

    // Load saved display name from preferences
    void loadSavedName(const char* savedName);

    // Update display (call periodically)
    void update(float weight, const char* deviceName, const char* timeString);

    // Set device name
    void setDeviceName(const char* name);

    // Get device name
    const char* getDeviceName() const;

private:
    LiquidCrystal_I2C* lcd_;
    uint8_t cols_;
    uint8_t rows_;
    bool initialized_;

    char deviceName_[32];
    unsigned long lastScreenChange_;
    bool showingName_;  // true=showing name, false=showing time

    // Cached line content to avoid redundant LCD writes (prevents flicker)
    char lastLine0_[17];  // 16 chars + null
    char lastLine1_[17];

    // Write a line to LCD only if it differs from what's already displayed
    void writeLineIfChanged(uint8_t row, const char* content, char* cache);
};
