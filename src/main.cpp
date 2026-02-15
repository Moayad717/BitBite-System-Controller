#include <Arduino.h>
#include <esp_task_wdt.h>

// Watchdog timeout in seconds (30s allows for sensor reads + I2C delays)
#define WDT_TIMEOUT_S 30

// Configuration
#include "config/Config.h"
#include "config/FeedingConfig.h"
#include "config/CalibrationConfig.h"

// Sensors
#include "sensors/WeightSensor.h"
#include "sensors/FlowSensor.h"
#include "sensors/EnvironmentSensor.h"

// Actuators
#include "actuators/MotorController.h"

// Feeding Logic
#include "feeding/FeedingStateMachine.h"
#include "feeding/FeedingLogger.h"

// Scheduling
#include "scheduling/RTCManager.h"
#include "scheduling/ScheduleManager.h"

// Faults
#include "faults/FaultManager.h"
#include "faults/FaultDetector.h"

// Communication
#include "communication/SerialProtocol.h"
#include "communication/StatusReporter.h"

// Display
#include "display/LCDDisplay.h"

// Storage
#include "storage/PreferencesManager.h"

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

// Sensors
WeightSensor weightSensor;
FlowSensor flowSensor;
EnvironmentSensor envSensor;

// Actuator
MotorController motorController;

// Feeding
FeedingStateMachine feedingFSM;
FeedingLogger feedingLogger;

// Scheduling
RTCManager rtcManager;
ScheduleManager scheduleManager;

// Faults
FaultManager faultManager;
FaultDetector faultDetector;

// Communication
SerialProtocol serialProtocol;
StatusReporter statusReporter;

// Display
LCDDisplay lcdDisplay;

// Storage
PreferencesManager prefsManager;

// ============================================================================
// TIMING VARIABLES
// ============================================================================

unsigned long lastSensorRead = 0;
unsigned long lastScheduleCheck = 0;
unsigned long lastFaultCheck = 0;
unsigned long lastStatusReport = 0;

// ============================================================================
// CALLBACK HANDLERS
// ============================================================================

void onFeedingComplete() {
    // Called when feeding cooldown completes
    FeedingResult result = feedingFSM.getLastResult();
    float amount = feedingFSM.getDispensedAmount();
    FeedingTrigger trigger = feedingFSM.getTrigger();

    // Get timestamp
    char timestamp[32];
    rtcManager.getTimestamp(timestamp, sizeof(timestamp));

    // Log the feeding
    feedingLogger.logFeeding(trigger, amount, result, timestamp);

    Serial.printf("[FEEDING] Complete: trigger=%d, amount=%.3f kg, result=%d\n",
                  trigger, amount, result);

    // Check for motor stuck fault (timeout with insufficient food dispensed)
    // Motor stuck if: timeout AND dispensed less than 50g (reasonable minimum for 10s runtime)
    if (result == RESULT_TIMEOUT) {
        Serial.printf("[FAULT] Motor stuck detected: timeout with only %.3f kg dispensed\n", amount);
        faultManager.setFault(FAULT_MOTOR_STUCK, "Motor Stuck/No Food Flow", amount);

        // Force send status immediately so WiFi ESP gets the fault notification
        statusReporter.updateFaults(faultManager.getActiveFaults());
        statusReporter.forceSend();
        Serial.println("[FAULT] Motor stuck status sent to WiFi ESP");
    } else if (result == RESULT_SUCCESS) {
        // Clear motor stuck fault only on successful feeding
        faultManager.clearFault(FAULT_MOTOR_STUCK);
    }
    // Note: Don't clear on timeout - let user manually clear or retry feeding

    // Reset lastFeedComplete to RESULT_NONE after cooldown
    // This allows the next feeding to be properly reported
    statusReporter.updateFeedingState(false, RESULT_NONE);
}

void onNameUpdate(const char* name) {
    // Update LCD display
    lcdDisplay.setDeviceName(name);

    // Save to preferences
    prefsManager.saveDisplayName(name);
    Serial.printf("[MAIN] Display name updated and saved: %s\n", name);
}

void onCommand(const char* command) {
    Serial.printf("[CMD] Received command: '%s'\n", command);

    if (strcmp(command, "FEED_NOW") == 0) {
        Serial.println("[CMD] Processing FEED_NOW");
        if (!feedingFSM.isFeeding()) {
            bool success = feedingFSM.startFeeding(TRIGGER_MANUAL, FEEDING_MANUAL_TARGET);
            if (success) {
                Serial.println("[CMD] Feeding started");
            } else {
                Serial.println("[CMD] Feeding failed to start (check FSM logs above)");
                // Update status to report the failure (lastFeedComplete = 2 for low level)
                statusReporter.updateFeedingState(false, feedingFSM.getLastResult());
                // Force send status immediately so WiFi ESP gets the failure notification
                statusReporter.forceSend();

                // Reset lastFeedComplete back to 0 after sending
                delay(100);  // Small delay to ensure message is sent
                statusReporter.updateFeedingState(false, RESULT_NONE);
            }
        } else {
            Serial.println("[CMD] Already feeding - ignored");
        }
    }
    else if (strcmp(command, "STOP") == 0) {
        Serial.println("[CMD] Processing STOP");
        if (feedingFSM.isFeeding()) {
            feedingFSM.stopFeeding(RESULT_ERROR);
        }
    }
    else if (strcmp(command, "TARE") == 0) {
        Serial.println("[CMD] Processing TARE");
        if (!feedingFSM.isFeeding()) {
            if (weightSensor.tare()) {
                long offset = weightSensor.getTareOffset();
                prefsManager.saveTareOffset(offset);
                Serial.printf("[CMD] Tare complete, offset: %ld\n", offset);
            } else {
                Serial.println("[CMD] Tare failed");
            }
        } else {
            Serial.println("[CMD] Cannot tare while feeding");
        }
    }
    else if (strcmp(command, "RESET_FLOW") == 0) {
        Serial.println("[CMD] Resetting flow sensor");
        flowSensor.resetDaily(rtcManager.getDayOfMonth());
        prefsManager.saveWaterFlow(0.0f);  // Save reset to flash
        Serial.println("[CMD] Flow reset saved to flash");
    }
    else if (strcmp(command, "CLEAR_FAULTS") == 0) {
        Serial.println("[CMD] Clearing all faults");
        faultManager.clearAllFaults();
    }
    else if (strcmp(command, "GET_SCHEDULE_STATUS") == 0) {
        Serial.println("[CMD] Sending schedule status");
        scheduleManager.sendScheduleStatus();
    }
    else {
        Serial.printf("[CMD] Unknown command: '%s'\n", command);
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    Serial.println("\n\n=================================");
    Serial.println("ESP32 Horse Feeder - FEEDING ESP");
    Serial.println("=================================\n");

    // Initialize Serial2 for WiFi ESP communication
    Serial2.setRxBufferSize(4096);  // Increase RX buffer for large JSON payloads
    Serial2.begin(SERIAL2_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.println("[INIT] Serial2 initialized (9600 baud, 4096 byte RX buffer)");

    // Initialize weight sensor (matching original working code)
    Serial.print("[INIT] Initializing weight sensor...");
    weightSensor.begin(SCALE_DOUT_PIN, SCALE_CLK_PIN, SCALE_CALIBRATION_FACTOR);

    // Initialize storage
    Serial.println("[INIT] Initializing preferences...");

    // Load tare offset from flash if available
    long savedOffset = prefsManager.loadTareOffset();
    if (savedOffset != 0) {
        weightSensor.setTareOffset(savedOffset);
        Serial.printf(" OK (loaded tare: %ld)\n", savedOffset);
    } else {
        Serial.println(" OK (no saved tare - will need calibration)");
    }

    // Initialize flow sensor
    Serial.print("[INIT] Initializing flow sensor...");
    flowSensor.begin(FLOW_SENSOR_PIN);
    Serial.println(" OK");

    // Delay to allow sensors to stabilize (matches original code)
    delay(2000);

    // Initialize environment sensor
    Serial.print("[INIT] Initializing DHT22 sensor...");
    envSensor.begin(DHT_PIN, DHT_TYPE);
    Serial.println(" OK");

    // Initialize RTC
    Serial.print("[INIT] Initializing RTC...");
    if (rtcManager.begin()) {
        Serial.println(" OK");
        char timestamp[32];
        rtcManager.getTimestamp(timestamp, sizeof(timestamp));
        Serial.printf("[INIT] Current time: %s\n", timestamp);

        // Load water flow from flash and set current day to prevent immediate reset
        float savedWaterFlow = prefsManager.loadWaterFlow();
        flowSensor.setTotalLiters(savedWaterFlow);
        flowSensor.setLastResetDay(rtcManager.getDayOfMonth());  // Set lastResetDay without clearing flow
        Serial.printf("[INIT] Loaded water flow: %.2f L (day=%d)\n", savedWaterFlow, rtcManager.getDayOfMonth());
    } else {
        Serial.println(" FAILED");
        faultManager.setFault(FAULT_RTC_FAIL, "RTC Init Failed");

        // Load water flow anyway (even without RTC)
        float savedWaterFlow = prefsManager.loadWaterFlow();
        flowSensor.setTotalLiters(savedWaterFlow);
        Serial.printf("[INIT] Loaded water flow: %.2f L (no RTC)\n", savedWaterFlow);
    }

    // Initialize LCD display
    Serial.print("[INIT] Initializing LCD display...");
    if (lcdDisplay.begin(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS)) {
        Serial.println(" OK");

        // Load saved display name from preferences
        String savedName = prefsManager.loadDisplayName();
        lcdDisplay.loadSavedName(savedName.c_str());
    } else {
        Serial.println(" FAILED");
    }

    // Initialize motor controller
    Serial.print("[INIT] Initializing motor controller...");
    motorController.begin(MOTOR_RELAY_PIN, MOTOR_SENSE_PIN);
    Serial.println(" OK");

    // Initialize feeding state machine
    Serial.print("[INIT] Initializing feeding FSM...");
    feedingFSM.begin(&motorController, &weightSensor);
    feedingFSM.setCooldownCallback(onFeedingComplete);
    Serial.println(" OK");

    // Initialize schedule manager
    Serial.print("[INIT] Initializing schedule manager...");
    scheduleManager.begin(&rtcManager);
    scheduleManager.loadFromFlash();
    Serial.println(" OK");

    // Initialize fault detector
    Serial.print("[INIT] Initializing fault detector...");
    faultDetector.begin(&faultManager, &weightSensor, &flowSensor, &envSensor, &rtcManager);
    Serial.println(" OK");

    // Initialize serial protocol
    Serial.print("[INIT] Initializing serial protocol...");
    serialProtocol.begin(&rtcManager, &scheduleManager, &feedingFSM, &faultManager);
    serialProtocol.setNameUpdateCallback(onNameUpdate);
    serialProtocol.setCommandCallback(onCommand);
    Serial.println(" OK");

    // Initialize hardware watchdog timer
    Serial.print("[INIT] Initializing watchdog timer...");
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // true = auto-reset on timeout
    esp_task_wdt_add(NULL);  // Subscribe current task (loopTask)
    Serial.println(" OK");

    Serial.println("\n[INIT] All systems initialized!");
    Serial.println("=================================\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    unsigned long currentMillis = millis();

    // ========================================================================
    // HIGH PRIORITY: Process incoming serial commands (always)
    // ========================================================================
    serialProtocol.processIncoming();

    // ========================================================================
    // HIGH PRIORITY: Update feeding state machine (non-blocking)
    // ========================================================================
    feedingFSM.update();

    // ========================================================================
    // HIGH PRIORITY: Update motor controller (non-blocking FSM)
    // ========================================================================
    motorController.update();

    // ========================================================================
    // MEDIUM PRIORITY: Read sensors (every 1 second)
    // ========================================================================
    if (currentMillis - lastSensorRead >= 1000) {
        lastSensorRead = currentMillis;

        // Update flow sensor
        flowSensor.update();

        // Check if midnight passed (reset daily water flow)
        if (flowSensor.needsMidnightReset(rtcManager.getDayOfMonth())) {
            flowSensor.resetDaily(rtcManager.getDayOfMonth());
            prefsManager.saveWaterFlow(0.0f);  // Save reset to flash immediately
            Serial.println("[MAIN] Midnight reset saved to flash");
        }

        // Save water flow periodically (every read, but NVS handles write leveling)
        float totalLiters = flowSensor.getTotalLiters();
        static float lastSavedLiters = 0;
        if (fabs(totalLiters - lastSavedLiters) > 0.01f) {  // 10ml threshold
            prefsManager.saveWaterFlow(totalLiters);
            lastSavedLiters = totalLiters;
        }

        // Read all sensors and update status reporter
        SensorReadings readings;
        readings.foodLevel = weightSensor.readWeight();
        readings.humidity = envSensor.readHumidity();
        readings.temperature = envSensor.readTemperature();
        readings.waterFlow = flowSensor.getTotalLiters();
        readings.valid = true;
        statusReporter.updateReadings(readings);

        // Update feeding state
        statusReporter.updateFeedingState(feedingFSM.isFeeding(), feedingFSM.getLastResult());

        // Update faults
        statusReporter.updateFaults(faultManager.getActiveFaults());

        // Update LCD display
        char timestamp[32];
        rtcManager.getTimestamp(timestamp, sizeof(timestamp));
        lcdDisplay.update(readings.foodLevel, lcdDisplay.getDeviceName(), timestamp);
    }

    // ========================================================================
    // MEDIUM PRIORITY: Check schedules (every 10 seconds)
    // ========================================================================
    if (currentMillis - lastScheduleCheck >= 10000) {
        lastScheduleCheck = currentMillis;

        if (!feedingFSM.isFeeding() && rtcManager.isValid()) {
            float amount = 0;
            if (scheduleManager.checkSchedules(amount)) {
                Serial.printf("[SCHEDULE] Matched! Amount: %.3f kg\n", amount);
                bool started = feedingFSM.startFeeding(TRIGGER_SCHEDULE, amount);

                if (started) {
                    // ✅ Only mark schedule as done if feeding ACTUALLY started
                    scheduleManager.confirmScheduleCompleted();
                    Serial.println("[SCHEDULE] Feeding started successfully");

                    // Clear schedule failed fault on success
                    faultManager.clearFault(FAULT_SCHEDULE_FAILED);
                } else {
                    // ❌ Feeding failed to start - set fault and retry
                    FeedingResult reason = feedingFSM.getLastResult();
                    float currentWeight = feedingFSM.getWeightBefore();
                    Serial.printf("[SCHEDULE] Failed to start! Reason: %d, Weight: %.3f kg\n", reason, currentWeight);

                    // Set fault with current weight reading (not schedule amount)
                    if (reason == RESULT_LOW_LEVEL) {
                        faultManager.setFault(FAULT_SCHEDULE_FAILED, "Schedule Skip: Low Food", currentWeight);
                    } else if (reason == RESULT_ERROR) {
                        faultManager.setFault(FAULT_SCHEDULE_FAILED, "Schedule Skip: Sensor Error", currentWeight);
                    } else {
                        faultManager.setFault(FAULT_SCHEDULE_FAILED, "Schedule Skip: Unknown", currentWeight);
                    }

                    // Force send fault notification to WiFi ESP
                    statusReporter.updateFaults(faultManager.getActiveFaults());
                    statusReporter.forceSend();
                }
            }
        }
    }

    // ========================================================================
    // LOW PRIORITY: Check for faults (every 30 seconds)
    // ========================================================================
    if (currentMillis - lastFaultCheck >= 30000) {
        lastFaultCheck = currentMillis;
        faultDetector.checkAll();
    }

    // ========================================================================
    // LOW PRIORITY: Send status updates (delta-based or 5-minute heartbeat)
    // ========================================================================
    if (currentMillis - lastStatusReport >= 1000) {  // Check every second
        lastStatusReport = currentMillis;

        if (statusReporter.shouldSendStatus()) {
            statusReporter.sendStatus();
        }
    }

    // Feed the watchdog - if loop hangs, ESP32 will auto-reset
    esp_task_wdt_reset();

    // Small delay to prevent tight loop
    delay(10);
}
