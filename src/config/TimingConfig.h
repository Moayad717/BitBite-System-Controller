#pragma once

// ============================================================================
// TIMING CONFIGURATION — Feeding ESP
// ============================================================================

// Watchdog
#define WDT_TIMEOUT_S              30       // Hardware watchdog (must survive I2C + sensor reads)

// Main loop intervals
#define SENSOR_READ_INTERVAL_MS    1000     // Sensor reads and status refresh
#define SCHEDULE_CHECK_INTERVAL_MS 10000    // Scheduled feeding checks
#define FAULT_CHECK_INTERVAL_MS    30000    // Fault detector sweep
#define STATUS_REPORT_INTERVAL_MS  1000     // Serial2 status push to Master
