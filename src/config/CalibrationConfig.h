#pragma once

// ============================================================================
// SENSOR CALIBRATION CONFIGURATION
// ============================================================================

// HX711 Weight Sensor Calibration
#define SCALE_CALIBRATION_FACTOR 101.0f          // Calibration factor for HX711 (matches original code)
#define SCALE_TARE_SAMPLES 20                    // Number of samples for tare (matches original code)
#define SCALE_READ_SAMPLES 10                    // Number of samples for each reading (increased for noise filtering)

// YF-S201 Water Flow Sensor Calibration
#define FLOW_SENSOR_CALIBRATION 1046.0f          // Pulses per liter
#define FLOW_SENSOR_MIN_PULSE_WIDTH 10           // ms - debounce time

// DHT22 Sensor Configuration
#define DHT_READ_INTERVAL 2000                   // ms - minimum time between reads

// RTC Configuration
#define RTC_SYNC_TIMEOUT 5000                    // ms - max time to wait for RTC response
