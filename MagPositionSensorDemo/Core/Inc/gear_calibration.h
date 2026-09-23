#ifndef GEAR_CALIBRATION_H
#define GEAR_CALIBRATION_H

#include "app_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    MagneticVector_t minimum;
    MagneticVector_t maximum;
} MagneticWindow_t;

typedef struct
{
    bool enabled;
    MagneticWindow_t entry_window;
} GearZone_t;

typedef struct
{
    GearZone_t zones[GEAR_COUNT];
    uint16_t hysteresis_counts;
    uint16_t stable_time_ms;
    uint8_t filter_shift;
} GearCalibration_t;

/*
 * Replace only the values in gear_calibration.c after measuring the assembled
 * mechanism. The shipped placeholder is deliberately invalid, so uncalibrated
 * hardware cannot issue a gear request.
 */
extern const GearCalibration_t g_gear_calibration;

bool GearCalibration_IsValid(const GearCalibration_t *calibration);

#endif /* GEAR_CALIBRATION_H */
