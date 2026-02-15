#include "FaultDetector.h"
#include "FaultManager.h"
#include "../sensors/WeightSensor.h"
#include "../sensors/FlowSensor.h"
#include "../sensors/EnvironmentSensor.h"
#include "../scheduling/RTCManager.h"
#include "../config/DataStructures.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

FaultDetector::FaultDetector()
    : faultManager_(nullptr),
      weightSensor_(nullptr),
      flowSensor_(nullptr),
      environmentSensor_(nullptr),
      rtcManager_(nullptr),
      lastFlowCheckTime_(0),
      lastFlowReading_(0) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void FaultDetector::begin(FaultManager* faultManager,
                          WeightSensor* weightSensor,
                          FlowSensor* flowSensor,
                          EnvironmentSensor* environmentSensor,
                          RTCManager* rtcManager) {
    faultManager_ = faultManager;
    weightSensor_ = weightSensor;
    flowSensor_ = flowSensor;
    environmentSensor_ = environmentSensor;
    rtcManager_ = rtcManager;

    lastFlowCheckTime_ = millis();
    if (flowSensor_) {
        lastFlowReading_ = flowSensor_->getTotalLiters();
    }
}

// ============================================================================
// FAULT CHECKING
// ============================================================================

void FaultDetector::checkAll() {
    checkWeightSensor();
    checkWaterLeak();
    checkRTC();
    checkDHTSensor();
}

void FaultDetector::checkWeightSensor() {
    if (!weightSensor_ || !faultManager_) {
        return;
    }

    // Read current weight
    float currentWeight = weightSensor_->readWeight();

    // Check for invalid readings (sensor hardware failure)
    if (currentWeight < -100 || currentWeight > 1000) {
        // Invalid reading (out of reasonable range)
        faultManager_->setFault(FAULT_WEIGHT_SENSOR, "Weight Sensor Invalid Reading", currentWeight);
    } else {
        faultManager_->clearFault(FAULT_WEIGHT_SENSOR);
    }
}

void FaultDetector::checkWaterLeak() {
    if (!flowSensor_ || !faultManager_) {
        return;
    }

    unsigned long currentTime = millis();
    float currentFlow = flowSensor_->getTotalLiters();

    // Check if >2.5L in 30 seconds
    unsigned long elapsed = currentTime - lastFlowCheckTime_;
    if (elapsed >= 30000) {  // 30 seconds
        float flowDelta = currentFlow - lastFlowReading_;

        if (flowDelta > 2.5f) {
            faultManager_->setFault(FAULT_WATER_LEAK, "Water Leak Detected", flowDelta);
        } else {
            faultManager_->clearFault(FAULT_WATER_LEAK);
        }

        // Update for next check
        lastFlowCheckTime_ = currentTime;
        lastFlowReading_ = currentFlow;
    }
}

void FaultDetector::checkRTC() {
    if (!rtcManager_ || !faultManager_) {
        return;
    }

    if (!rtcManager_->isValid()) {
        faultManager_->setFault(FAULT_RTC_FAIL, "RTC Time Invalid");
    } else {
        faultManager_->clearFault(FAULT_RTC_FAIL);
    }
}

void FaultDetector::checkDHTSensor() {
    if (!environmentSensor_ || !faultManager_) {
        return;
    }

    // Read temperature (will update lastReadValid)
    float temp = environmentSensor_->readTemperature();
    float humidity = environmentSensor_->readHumidity();

    // Check if sensor is valid
    if (!environmentSensor_->isValid()) {
        Serial.printf("[FAULT] DHT sensor failed: temp=%.1f, humidity=%.1f\n", temp, humidity);
        faultManager_->setFault(FAULT_DHT_FAIL, "DHT Sensor Read Failed");
    } else {
        faultManager_->clearFault(FAULT_DHT_FAIL);
    }
}
