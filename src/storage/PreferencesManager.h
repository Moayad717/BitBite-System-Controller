#pragma once

#include <Arduino.h>
#include <Preferences.h>

// ============================================================================
// PREFERENCES MANAGER
// ============================================================================
// Simple wrapper for ESP32 NVS flash storage
// Stores: Water flow total, Tare offset

class PreferencesManager {
public:
    PreferencesManager();

    // Water flow persistence
    float loadWaterFlow();
    void saveWaterFlow(float totalLiters);

    // Tare offset persistence (future use)
    long loadTareOffset();
    void saveTareOffset(long offset);

    // Display name persistence
    String loadDisplayName();
    void saveDisplayName(const char* name);

private:
    Preferences preferences_;

    // Safe open/close helpers to prevent NVS namespace lockup
    bool openNamespace(bool readOnly);
    void closeNamespace();
};
