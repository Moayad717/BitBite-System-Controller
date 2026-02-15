#pragma once

// ============================================================================
// FEEDING CONTROL CONFIGURATION
// ============================================================================

// Feeding Thresholds
#define FEEDING_LOW_LEVEL_THRESHOLD 0.2f        // kg - minimum food level to feed
#define FEEDING_MANUAL_TARGET 0.15f              // kg - target for manual feeding (150g)
#define FEEDING_MANUAL_PULSE_THRESHOLD 0.075f    // kg - when to start pulsing (50% of target)
#define FEEDING_MIN_DISPENSE 0.1f                // kg - minimum to consider feeding successful

// Feeding Timing
#define FEEDING_TIMEOUT 10000                    // ms - max feeding duration
#define FEEDING_COOLDOWN 10000                   // ms - minimum time between feedings
#define FEEDING_PULSE_ON_TIME 50                 // ms - motor ON during pulse
#define FEEDING_PULSE_OFF_TIME 200               // ms - motor OFF during pulse

// Schedule-based Feeding
#define FEEDING_SCHEDULE_PULSE_THRESHOLD 0.5f    // Pulse when 50% of target reached
#define MAX_SCHEDULES 150                        // Maximum cached schedules (18 schedules × 7 days = 126)

// Status Reporting Deltas (send status only if changed by these amounts)
#define STATUS_FOOD_LEVEL_DELTA 0.05f            // kg - 50g change
#define STATUS_HUMIDITY_DELTA 2.0f               // % - 2% change
#define STATUS_TEMPERATURE_DELTA 1.0f            // °C - 1°C change
#define STATUS_WATER_FLOW_DELTA 0.1f             // L - 0.1L change
#define STATUS_HEARTBEAT_INTERVAL 300000         // ms - 5 min max without update

// Update Intervals
#define STATUS_UPDATE_INTERVAL 10000             // ms - check for status updates every 10s
#define SCHEDULE_CHECK_INTERVAL 60000            // ms - check schedules every 60s
#define FAULT_CHECK_INTERVAL 30000               // ms - check for faults every 30s
#define LCD_DISPLAY_CYCLE_TIME 5000              // ms - cycle LCD display every 5s

// Memory Management
#define MEMORY_LOW_THRESHOLD 50000               // bytes - restart if heap drops below this
