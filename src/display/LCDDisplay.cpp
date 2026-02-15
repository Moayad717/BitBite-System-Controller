#include "LCDDisplay.h"
#include "../config/Config.h"
#include "../config/FeedingConfig.h"
#include <Wire.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

LCDDisplay::LCDDisplay()
    : lcd_(nullptr),
      cols_(0),
      rows_(0),
      initialized_(false),
      lastScreenChange_(0),
      showingName_(false) {

    strcpy(deviceName_, "Set Name via WiFi");
    lastLine0_[0] = '\0';
    lastLine1_[0] = '\0';
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool LCDDisplay::begin(uint8_t address, uint8_t cols, uint8_t rows) {
    cols_ = cols;
    rows_ = rows;

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);  // Give I2C bus time to stabilize

    lcd_ = new LiquidCrystal_I2C(address, cols, rows);
    lcd_->init();
    lcd_->backlight();
    lcd_->clear();

    initialized_ = true;
    return true;
}

// ============================================================================
// DISPLAY UPDATE
// ============================================================================

void LCDDisplay::update(float weight, const char* deviceName, const char* timeString) {
    if (!initialized_) {
        return;
    }

    unsigned long now = millis();

    // Alternate between time and name on cycle
    if (now - lastScreenChange_ >= LCD_DISPLAY_CYCLE_TIME) {
        showingName_ = !showingName_;
        lastScreenChange_ = now;
        // Invalidate line 1 cache to force redraw on screen switch
        lastLine1_[0] = '\0';
    }

    // Build line 0: weight
    char line0[17];
    if (weight >= -0.5 && weight < 100) {
        snprintf(line0, sizeof(line0), "Food: %-9.2f", weight);
        // Overwrite trailing spaces with " kg"
        // Find end of number and append unit
        char temp[17];
        snprintf(temp, sizeof(temp), "Food:%5.2fkg", weight);
        snprintf(line0, sizeof(line0), "%-16s", temp);
    } else {
        snprintf(line0, sizeof(line0), "Food: ERR       ");
    }
    line0[16] = '\0';
    writeLineIfChanged(0, line0, lastLine0_);

    // Build line 1: name or time
    char line1[17];
    memset(line1, ' ', 16);
    line1[16] = '\0';

    if (showingName_) {
        snprintf(line1, sizeof(line1), "%-16s", deviceName_);
    } else {
        if (timeString && strlen(timeString) >= 8) {
            const char* timePart = strchr(timeString, 'T');
            if (!timePart) {
                timePart = strrchr(timeString, ' ');
            }
            if (timePart) {
                snprintf(line1, sizeof(line1), "%-16s", timePart + 1);
            }
        }
    }
    line1[16] = '\0';
    writeLineIfChanged(1, line1, lastLine1_);
}

void LCDDisplay::writeLineIfChanged(uint8_t row, const char* content, char* cache) {
    if (strcmp(content, cache) == 0) {
        return;  // No change - skip LCD write
    }
    lcd_->setCursor(0, row);
    lcd_->print(content);
    strncpy(cache, content, 16);
    cache[16] = '\0';
}

// ============================================================================
// DEVICE NAME
// ============================================================================

void LCDDisplay::loadSavedName(const char* savedName) {
    if (savedName && strlen(savedName) > 0) {
        strncpy(deviceName_, savedName, 31);
        deviceName_[31] = '\0';
        Serial.printf("[LCD] Loaded saved name: %s\n", deviceName_);
    } else {
        strcpy(deviceName_, "Set Name via WiFi");
        Serial.println("[LCD] No saved name, using default");
    }
}

void LCDDisplay::setDeviceName(const char* name) {
    if (name) {
        strncpy(deviceName_, name, 31);
        deviceName_[31] = '\0';
    }
}

const char* LCDDisplay::getDeviceName() const {
    return deviceName_;
}
