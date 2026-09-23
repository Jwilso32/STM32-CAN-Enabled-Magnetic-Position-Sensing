#include "gear_calibration.h"

#include <stddef.h>

#define AH4931Q_MIN_FIELD_COUNT   (-2048)
#define AH4931Q_MAX_FIELD_COUNT    2047

/*
 * CALIBRATION DATA
 * ----------------
 * Each entry is an inclusive X/Y/Z box measured at the corresponding detent.
 * Leave a gap between neighboring entry boxes. Runtime hysteresis expands the
 * currently selected box by hysteresis_counts, so normal noise does not make
 * the gear chatter at a boundary.
 *
 * These entries are intentionally disabled until measurements are collected
 * from the final, assembled mechanism. Enabling guessed values would be unsafe.
 */
const GearCalibration_t g_gear_calibration =
{
    .zones =
    {
        [GEAR_PARK] =
        {
            .enabled = false,
            .entry_window = { .minimum = {0, 0, 0}, .maximum = {0, 0, 0} }
        },
        [GEAR_REVERSE] =
        {
            .enabled = false,
            .entry_window = { .minimum = {0, 0, 0}, .maximum = {0, 0, 0} }
        },
        [GEAR_NEUTRAL] =
        {
            .enabled = false,
            .entry_window = { .minimum = {0, 0, 0}, .maximum = {0, 0, 0} }
        },
        [GEAR_DRIVE] =
        {
            .enabled = false,
            .entry_window = { .minimum = {0, 0, 0}, .maximum = {0, 0, 0} }
        }
    },
    .hysteresis_counts = 40U,
    .stable_time_ms = 40U,
    .filter_shift = 2U
};

static bool WindowIsOrdered(const MagneticWindow_t *window)
{
    const bool ordered =
           (window->minimum.x <= window->maximum.x) &&
           (window->minimum.y <= window->maximum.y) &&
           (window->minimum.z <= window->maximum.z);

    const bool in_sensor_range =
           (window->minimum.x >= AH4931Q_MIN_FIELD_COUNT) &&
           (window->minimum.y >= AH4931Q_MIN_FIELD_COUNT) &&
           (window->minimum.z >= AH4931Q_MIN_FIELD_COUNT) &&
           (window->maximum.x <= AH4931Q_MAX_FIELD_COUNT) &&
           (window->maximum.y <= AH4931Q_MAX_FIELD_COUNT) &&
           (window->maximum.z <= AH4931Q_MAX_FIELD_COUNT);

    return ordered && in_sensor_range;
}

static bool WindowsOverlap(const MagneticWindow_t *first,
                           const MagneticWindow_t *second)
{
    const bool overlap_x = (first->minimum.x <= second->maximum.x) &&
                           (second->minimum.x <= first->maximum.x);
    const bool overlap_y = (first->minimum.y <= second->maximum.y) &&
                           (second->minimum.y <= first->maximum.y);
    const bool overlap_z = (first->minimum.z <= second->maximum.z) &&
                           (second->minimum.z <= first->maximum.z);

    return overlap_x && overlap_y && overlap_z;
}

bool GearCalibration_IsValid(const GearCalibration_t *calibration)
{
    if ((calibration == NULL) ||
        (calibration->filter_shift > 8U) ||
        (calibration->stable_time_ms == 0U))
    {
        return false;
    }

    for (uint32_t gear = 0U; gear < (uint32_t)GEAR_COUNT; ++gear)
    {
        if (!calibration->zones[gear].enabled ||
            !WindowIsOrdered(&calibration->zones[gear].entry_window))
        {
            return false;
        }
    }

    for (uint32_t first = 0U; first < (uint32_t)GEAR_COUNT; ++first)
    {
        for (uint32_t second = first + 1U;
             second < (uint32_t)GEAR_COUNT;
             ++second)
        {
            if (WindowsOverlap(&calibration->zones[first].entry_window,
                               &calibration->zones[second].entry_window))
            {
                return false;
            }
        }
    }

    return true;
}
