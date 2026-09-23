#ifndef CALIBRATION_MODE_H
#define CALIBRATION_MODE_H

#include "app_config.h"
#include "app_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t sequence;
    uint32_t timestamp_ms;
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    int16_t filtered_x;
    int16_t filtered_y;
    int16_t filtered_z;
    uint8_t gear;
    uint8_t sensor_ok;
    uint8_t active;
} CalibrationTelemetry_t;

typedef struct
{
    uint32_t timestamp_ms;
    MagneticVector_t raw;
    MagneticVector_t filtered;
    uint8_t gear;
    uint8_t sensor_ok;
} CalibrationCaptureSample_t;

/* Set to 1 in a debugger before App_Init() to force service mode without a strap. */
extern volatile uint8_t g_force_calibration_mode;

/* Safe to watch with STM32CubeIDE Live Expressions over ordinary SWD. */
extern volatile CalibrationTelemetry_t g_calibration_telemetry;

/*
 * Circular sweep history for debugger-only recovery over ordinary SWD.
 * Halt the target before exporting it so no record can be partially updated.
 */
extern volatile CalibrationCaptureSample_t
    g_calibration_capture[APP_CALIBRATION_CAPTURE_SAMPLES];
extern volatile uint32_t g_calibration_capture_write_index;
extern volatile uint32_t g_calibration_capture_total_samples;

void CalibrationMode_Init(bool active, uint32_t now_ms);
bool CalibrationMode_IsActive(void);
void CalibrationMode_Update(const MagneticVector_t *raw,
                            const MagneticVector_t *filtered,
                            GearPosition_t gear,
                            bool sensor_ok,
                            uint32_t now_ms);

#endif /* CALIBRATION_MODE_H */
