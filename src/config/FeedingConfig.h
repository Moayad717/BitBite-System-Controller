#pragma once

// ============================================================================
// FEEDING CONTROL CONFIGURATION
// ============================================================================

// Feeding Thresholds
#define FEEDING_LOW_LEVEL_THRESHOLD 0.2f        // kg - minimum food level to feed
#define FEEDING_MANUAL_TARGET 0.15f              // kg - target for manual feeding (150g)
#define FEEDING_MANUAL_PULSE_THRESHOLD 0.075f    // kg - when to start pulsing (50% of manual target)
#define FEEDING_MIN_DISPENSE 0.1f                // kg - minimum to consider feeding successful

// Feeding Timing
#define FEEDING_TIMEOUT 30000                    // ms - max feeding duration (longer for pulse-and-weigh)
#define FEEDING_COOLDOWN 10000                   // ms - minimum time between feedings

// Manual feeding pulse timings (continuous then pulse)
#define FEEDING_PULSE_ON_TIME 50                 // ms - motor ON during pulse (manual feed fine-tuning)
#define FEEDING_PULSE_OFF_TIME 200               // ms - motor OFF during pulse

// Schedule-based Feeding: Pulse-and-Weigh algorithm
// Motor pulses, stops, waits for scale to settle, reads weight, repeats
#define FEEDING_LONG_PULSE_ON_TIME 150           // ms - motor pulse when far from target (>70% remaining)
#define FEEDING_SHORT_PULSE_ON_TIME 50           // ms - motor pulse when close to target
#define FEEDING_PHASE_THRESHOLD 0.3f             // Switch to short pulses when 70% dispensed (30% remaining)
#define FEEDING_SETTLE_TIME 400                  // ms - wait after motor off for scale to stabilize
#define FEEDING_STOP_EARLY_FACTOR 0.85f          // Stop at 85% of target (in-flight food covers the rest)
#define FEEDING_FAST_READ_SAMPLES 3              // Fewer HX711 samples for faster reads during feeding
#define MAX_SCHEDULES 150                        // Maximum cached schedules (18 schedules × 7 days = 126)

// Status Reporting Deltas (send status only if changed by these amounts)
#define STATUS_FOOD_LEVEL_DELTA 0.05f            // kg - 50g change
#define STATUS_HUMIDITY_DELTA 2.0f               // % - 2% change
#define STATUS_TEMPERATURE_DELTA 1.0f            // °C - 1°C change
#define STATUS_WATER_FLOW_DELTA 0.1f             // L - 0.1L change
#define STATUS_HEARTBEAT_INTERVAL 300000         // ms - 5 min max without update

// Update Intervals
#define LCD_DISPLAY_CYCLE_TIME 5000              // ms - cycle LCD display every 5s

