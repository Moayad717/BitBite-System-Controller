#pragma once

#include <Arduino.h>

// Forward declarations
class FaultManager;
class WeightSensor;
class FlowSensor;
class EnvironmentSensor;
class RTCManager;

// ============================================================================
// FAULT DETECTOR
// ============================================================================
// Periodic fault detection for all subsystems
// Checks: Motor stuck, Water leak, Weight sensor, RTC, DHT sensor

class FaultDetector {
public:
    FaultDetector();

    // Initialize with dependencies
    void begin(FaultManager* faultManager,
               WeightSensor* weightSensor,
               FlowSensor* flowSensor,
               EnvironmentSensor* environmentSensor,
               RTCManager* rtcManager);

    // Check all faults (call periodically every 30s)
    void checkAll();

private:
    FaultManager* faultManager_;
    WeightSensor* weightSensor_;
    FlowSensor* flowSensor_;
    EnvironmentSensor* environmentSensor_;
    RTCManager* rtcManager_;

    // Last flow measurement (for leak detection)
    unsigned long lastFlowCheckTime_;
    float lastFlowReading_;

    // Individual fault checks
    void checkWeightSensor();
    void checkWaterLeak();
    void checkRTC();
    void checkDHTSensor();
};
