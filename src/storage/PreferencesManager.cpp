#include "PreferencesManager.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

PreferencesManager::PreferencesManager() {
}

// ============================================================================
// SAFE NVS OPEN/CLOSE HELPERS
// ============================================================================

bool PreferencesManager::openNamespace(bool readOnly) {
    // Always close first in case a previous operation left it open
    preferences_.end();
    return preferences_.begin("feeder", readOnly);
}

void PreferencesManager::closeNamespace() {
    preferences_.end();
}

// ============================================================================
// WATER FLOW PERSISTENCE
// ============================================================================

float PreferencesManager::loadWaterFlow() {
    float totalLiters = 0.0f;
    if (openNamespace(true)) {
        totalLiters = preferences_.getFloat("waterFlow", 0.0f);
        closeNamespace();
    }
    Serial.printf("[PREFS] Water flow loaded: %.2f L\n", totalLiters);
    return totalLiters;
}

void PreferencesManager::saveWaterFlow(float totalLiters) {
    if (openNamespace(false)) {
        preferences_.putFloat("waterFlow", totalLiters);
        closeNamespace();
    }
}

// ============================================================================
// TARE OFFSET PERSISTENCE
// ============================================================================

long PreferencesManager::loadTareOffset() {
    long offset = 0;
    if (openNamespace(true)) {
        offset = preferences_.getLong("tareOffset", 0);
        closeNamespace();
    }
    return offset;
}

void PreferencesManager::saveTareOffset(long offset) {
    if (openNamespace(false)) {
        preferences_.putLong("tareOffset", offset);
        closeNamespace();
    }
}

// ============================================================================
// DISPLAY NAME PERSISTENCE
// ============================================================================

String PreferencesManager::loadDisplayName() {
    String name = "";
    if (openNamespace(true)) {
        name = preferences_.getString("displayName", "");
        closeNamespace();
    }
    return name;
}

void PreferencesManager::saveDisplayName(const char* name) {
    if (openNamespace(false)) {
        preferences_.putString("displayName", name);
        closeNamespace();
    }
    Serial.printf("[PREFS] Display name saved: %s\n", name);
}
