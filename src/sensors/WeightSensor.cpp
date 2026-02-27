#include "WeightSensor.h"
#include "../config/CalibrationConfig.h"
#include "../config/DataStructures.h"
#include "../config/FeedingConfig.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

WeightSensor::WeightSensor()
    : calibrationFactor_(SCALE_CALIBRATION_FACTOR),
      initialized_(false),
      lastValidWeight_(0.0f) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool WeightSensor::begin(uint8_t doutPin, uint8_t clkPin, float calibrationFactor) {
    calibrationFactor_ = calibrationFactor;

    // Initialize HX711
    scale_.begin(doutPin, clkPin);

    // Set calibration factor immediately after begin()
    scale_.set_scale(calibrationFactor_);

    initialized_ = true;
    return true;
}

// ============================================================================
// WEIGHT READING
// ============================================================================

float WeightSensor::readWeight() {

    // Read average of multiple samples
    // Multiply by 4 (hardware-specific calibration for load cell configuration)
    float rawReading = scale_.get_units(SCALE_READ_SAMPLES);

    if (isnan(rawReading) || isinf(rawReading)) {
        Serial.println("[WEIGHT] Invalid reading from HX711 (NaN/Inf)");
        return SENSOR_ERROR_VALUE;
    }

    float weight = rawReading * 4.0f;

    // Return in kg (get_units returns grams with our calibration)
    float result = weight / 1000.0f;

    // Sanity check: reject readings outside reasonable range
    if (result < -100.0f || result > 1000.0f) {
        Serial.printf("[WEIGHT] Reading out of range: %.2f kg\n", result);
        return SENSOR_ERROR_VALUE;
    }

    lastValidWeight_ = result;
    return result;
}

// ============================================================================
// FAST WEIGHT READING (for active feeding feedback)
// ============================================================================

float WeightSensor::readWeightFast() {

    // Read fewer samples for faster response during active feeding
    float rawReading = scale_.get_units(FEEDING_FAST_READ_SAMPLES);

    if (isnan(rawReading) || isinf(rawReading)) {
        Serial.println("[WEIGHT] Invalid fast reading from HX711 (NaN/Inf)");
        return SENSOR_ERROR_VALUE;
    }

    float weight = rawReading * 4.0f;

    // Return in kg (get_units returns grams with our calibration)
    float result = weight / 1000.0f;

    // Sanity check: reject readings outside reasonable range
    if (result < -100.0f || result > 1000.0f) {
        Serial.printf("[WEIGHT] Fast reading out of range: %.2f kg\n", result);
        return SENSOR_ERROR_VALUE;
    }

    lastValidWeight_ = result;
    return result;
}

// ============================================================================
// TARE (ZERO) SCALE
// ============================================================================

bool WeightSensor::tare(uint8_t samples) {
    if (!initialized_) {
        return false;
    }

    scale_.tare(samples);

    // Wait for tare to complete
    delay(200);

    return true;
}

// ============================================================================
// STATUS CHECKS
// ============================================================================

bool WeightSensor::isReady() const {
    if (!initialized_) {
        return false;
    }
    // Cast away const since HX711::is_ready() is not const (library limitation)
    return const_cast<HX711&>(scale_).is_ready();
}

// ============================================================================
// CALIBRATION
// ============================================================================

void WeightSensor::setCalibrationFactor(float factor) {
    calibrationFactor_ = factor;
    if (initialized_) {
        scale_.set_scale(calibrationFactor_);
    }
}

long WeightSensor::readRaw() {
    if (!initialized_) {
        return 0;
    }
    return scale_.read_average(SCALE_READ_SAMPLES);
}

// ============================================================================
// TARE OFFSET PERSISTENCE
// ============================================================================

long WeightSensor::getTareOffset() const {
    if (!initialized_) {
        return 0;
    }
    // Cast away const since HX711::get_offset() is not const (library limitation)
    return const_cast<HX711&>(scale_).get_offset();
}

void WeightSensor::setTareOffset(long offset) {
    if (initialized_) {
        scale_.set_offset(offset);
    }
}
