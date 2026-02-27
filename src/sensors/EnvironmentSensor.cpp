#include "EnvironmentSensor.h"
#include "../config/CalibrationConfig.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

EnvironmentSensor::EnvironmentSensor()
    : dht_(nullptr),
      pin_(0),
      type_(0),
      lastReadTime_(0),
      lastReadValid_(false),
      lastTemperature_(-999),
      lastHumidity_(-999),
      consecutiveFailures_(0),
      lastRecoveryAttempt_(0) {
}

EnvironmentSensor::~EnvironmentSensor() {
    delete dht_;
    dht_ = nullptr;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void EnvironmentSensor::begin(uint8_t pin, uint8_t type) {
    pin_ = pin;
    type_ = type;

    // Clean up previous instance if begin() called again
    delete dht_;
    dht_ = new DHT(pin_, type_);
    dht_->begin();

    // Reset recovery tracking
    consecutiveFailures_ = 0;
    lastRecoveryAttempt_ = 0;

    // Wait for sensor to stabilize
    delay(2000);

    // Force initial read to populate cache
    readSensor();
    Serial.printf("[ENV] Initial DHT read: temp=%.1f°C, humidity=%.1f%%, valid=%d\n",
                  lastTemperature_, lastHumidity_, lastReadValid_);
}

// ============================================================================
// SENSOR READING (reads both temp and humidity together)
// ============================================================================

void EnvironmentSensor::readSensor() {
    unsigned long currentTime = millis();

    // Respect minimum read interval
    if (currentTime - lastReadTime_ < DHT_READ_INTERVAL) {
        return;  // Use cached values
    }

    // Read both temperature and humidity from DHT sensor
    float temp = dht_->readTemperature();
    float humidity = dht_->readHumidity();

    // Check if reads were valid
    bool tempValid = !isnan(temp);
    bool humidityValid = !isnan(humidity);

    if (tempValid && humidityValid) {
        lastReadValid_ = true;
        lastTemperature_ = temp;
        lastHumidity_ = humidity;
        lastReadTime_ = currentTime;
        consecutiveFailures_ = 0;  // Reset failure counter on success
    } else {
        consecutiveFailures_++;
        lastReadTime_ = currentTime;  // Prevent rapid retries

        // Keep last valid readings for first 5 failures (DHT22 is flaky)
        // Only mark invalid after sustained failure
        if (consecutiveFailures_ >= 5) {
            lastReadValid_ = false;
            lastTemperature_ = -999;
            lastHumidity_ = -999;

            // Attempt recovery after sustained failure
            attemptRecovery();
        }
    }
}

// ============================================================================
// TEMPERATURE READING
// ============================================================================

float EnvironmentSensor::readTemperature() {
    readSensor();  // Update cached values if needed
    return lastTemperature_;
}

// ============================================================================
// HUMIDITY READING
// ============================================================================

float EnvironmentSensor::readHumidity() {
    readSensor();  // Update cached values if needed
    return lastHumidity_;
}

// ============================================================================
// STATUS CHECKS
// ============================================================================

bool EnvironmentSensor::isValid() const {
    return lastReadValid_;
}

unsigned long EnvironmentSensor::timeSinceLastRead() const {
    return millis() - lastReadTime_;
}

// ============================================================================
// RECOVERY
// ============================================================================

void EnvironmentSensor::attemptRecovery() {
    unsigned long currentTime = millis();

    // Only attempt recovery every 30 seconds to avoid excessive reinitializations
    if (currentTime - lastRecoveryAttempt_ < 30000) {
        return;
    }

    lastRecoveryAttempt_ = currentTime;

    Serial.printf("[ENV] DHT stuck at -999 (%d failures) - attempting recovery\n", consecutiveFailures_);

    // Pulse the data pin LOW to reset the sensor hardware (clears stuck mid-transmission state)
    delete dht_;
    dht_ = nullptr;
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    delay(20);  // Hold low long enough for sensor to detect reset
    pinMode(pin_, INPUT_PULLUP);

    // Reinitialize the DHT driver and wait for sensor to stabilize
    dht_ = new DHT(pin_, type_);
    dht_->begin();

    // Wait for sensor to fully stabilize (some DHT22 variants need 3s)
    for (int i = 0; i < 30; i++) {
        delay(100);
        yield();  // Feed watchdog / RTOS scheduler
    }

    // Force a fresh hardware read (bypass library's internal cache)
    float temp = dht_->readTemperature(false, true);
    float humidity = dht_->readHumidity(true);

    // Reset failure counter regardless of outcome to give sensor fresh chances
    consecutiveFailures_ = 0;

    if (!isnan(temp) && !isnan(humidity)) {
        Serial.printf("[ENV] Recovery successful! temp=%.1f°C, humidity=%.1f%%\n", temp, humidity);
        lastReadValid_ = true;
        lastTemperature_ = temp;
        lastHumidity_ = humidity;
        lastReadTime_ = currentTime;
    } else {
        Serial.println("[ENV] Recovery failed - will retry in 30s");
    }
}
