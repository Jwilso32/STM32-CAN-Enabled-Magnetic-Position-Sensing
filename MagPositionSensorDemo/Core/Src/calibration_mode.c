#include "calibration_mode.h"

#include "app_config.h"

#include <stdio.h>

volatile uint8_t g_force_calibration_mode = 0U;
volatile CalibrationTelemetry_t g_calibration_telemetry;
volatile CalibrationCaptureSample_t
    g_calibration_capture[APP_CALIBRATION_CAPTURE_SAMPLES];
volatile uint32_t g_calibration_capture_write_index;
volatile uint32_t g_calibration_capture_total_samples;

static bool calibration_active;
static uint32_t last_log_ms;

#if APP_ENABLE_ITM_TRACE
static char GearCharacter(GearPosition_t gear)
{
    switch (gear)
    {
        case GEAR_PARK:    return 'P';
        case GEAR_REVERSE: return 'R';
        case GEAR_NEUTRAL: return 'N';
        case GEAR_DRIVE:   return 'D';
        default:           return 'X';
    }
}
#endif

void CalibrationMode_Init(bool active, uint32_t now_ms)
{
    calibration_active = active || (g_force_calibration_mode != 0U);
    last_log_ms = now_ms - APP_CALIBRATION_LOG_PERIOD_MS;
    g_calibration_capture_write_index = 0U;
    g_calibration_capture_total_samples = 0U;
    g_calibration_telemetry.active = calibration_active ? 1U : 0U;

#if APP_ENABLE_ITM_TRACE
    if (calibration_active)
    {
        printf("CALIBRATION MODE: gear commands suppressed\r\n");
        printf("CAL,ms,raw_x,raw_y,raw_z,filtered_x,filtered_y,filtered_z,gear,sensor_ok\r\n");
    }
#endif
}

bool CalibrationMode_IsActive(void)
{
    return calibration_active;
}

void CalibrationMode_Update(const MagneticVector_t *raw,
                            const MagneticVector_t *filtered,
                            GearPosition_t gear,
                            bool sensor_ok,
                            uint32_t now_ms)
{
    if ((raw == NULL) || (filtered == NULL))
    {
        return;
    }

    /* Odd/even sequence values let a debugger detect a partially read snapshot. */
    ++g_calibration_telemetry.sequence;
    g_calibration_telemetry.timestamp_ms = now_ms;
    g_calibration_telemetry.raw_x = raw->x;
    g_calibration_telemetry.raw_y = raw->y;
    g_calibration_telemetry.raw_z = raw->z;
    g_calibration_telemetry.filtered_x = filtered->x;
    g_calibration_telemetry.filtered_y = filtered->y;
    g_calibration_telemetry.filtered_z = filtered->z;
    g_calibration_telemetry.gear = (uint8_t)gear;
    g_calibration_telemetry.sensor_ok = sensor_ok ? 1U : 0U;
    g_calibration_telemetry.active = calibration_active ? 1U : 0U;
    ++g_calibration_telemetry.sequence;

    if (calibration_active &&
        ((uint32_t)(now_ms - last_log_ms) >= APP_CALIBRATION_LOG_PERIOD_MS))
    {
        volatile CalibrationCaptureSample_t *capture =
            &g_calibration_capture[g_calibration_capture_write_index];

        capture->timestamp_ms = now_ms;
        capture->raw = *raw;
        capture->filtered = *filtered;
        capture->gear = (uint8_t)gear;
        capture->sensor_ok = sensor_ok ? 1U : 0U;

        g_calibration_capture_write_index =
            (g_calibration_capture_write_index + 1U) %
            APP_CALIBRATION_CAPTURE_SAMPLES;
        ++g_calibration_capture_total_samples;

#if APP_ENABLE_ITM_TRACE
        printf("CAL,%lu,%d,%d,%d,%d,%d,%d,%c,%u\r\n",
               (unsigned long)now_ms,
               raw->x,
               raw->y,
               raw->z,
               filtered->x,
               filtered->y,
               filtered->z,
               GearCharacter(gear),
               sensor_ok ? 1U : 0U);
#endif
        last_log_ms = now_ms;
    }
}
