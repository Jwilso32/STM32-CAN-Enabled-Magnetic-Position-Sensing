#include "gear_selector.h"

#include <limits.h>
#include <stddef.h>

static bool IsRealGear(GearPosition_t gear)
{
    return (uint32_t)gear < (uint32_t)GEAR_COUNT;
}

static int32_t LowWithMargin(int16_t value, uint16_t margin)
{
    int32_t result = (int32_t)value - (int32_t)margin;
    return (result < INT16_MIN) ? INT16_MIN : result;
}

static int32_t HighWithMargin(int16_t value, uint16_t margin)
{
    int32_t result = (int32_t)value + (int32_t)margin;
    return (result > INT16_MAX) ? INT16_MAX : result;
}

static bool WindowContains(const MagneticWindow_t *window,
                           const MagneticVector_t *field,
                           uint16_t margin)
{
    return ((int32_t)field->x >= LowWithMargin(window->minimum.x, margin)) &&
           ((int32_t)field->x <= HighWithMargin(window->maximum.x, margin)) &&
           ((int32_t)field->y >= LowWithMargin(window->minimum.y, margin)) &&
           ((int32_t)field->y <= HighWithMargin(window->maximum.y, margin)) &&
           ((int32_t)field->z >= LowWithMargin(window->minimum.z, margin)) &&
           ((int32_t)field->z <= HighWithMargin(window->maximum.z, margin));
}

static int16_t FilterAxis(int16_t current, int16_t input, uint8_t shift)
{
    if (shift == 0U)
    {
        return input;
    }

    const int32_t delta = (int32_t)input - (int32_t)current;
    return (int16_t)((int32_t)current + (delta / (int32_t)(1UL << shift)));
}

static void FilterSample(GearSelector_t *selector,
                         const MagneticVector_t *raw_field)
{
    const uint8_t filter_shift = (selector->calibration == NULL) ?
                                 0U : selector->calibration->filter_shift;

    if (!selector->filter_initialized)
    {
        selector->filtered = *raw_field;
        selector->filter_initialized = true;
        return;
    }

    selector->filtered.x = FilterAxis(selector->filtered.x,
                                      raw_field->x,
                                      filter_shift);
    selector->filtered.y = FilterAxis(selector->filtered.y,
                                      raw_field->y,
                                      filter_shift);
    selector->filtered.z = FilterAxis(selector->filtered.z,
                                      raw_field->z,
                                      filter_shift);
}

static GearPosition_t Classify(const GearSelector_t *selector)
{
    if (!selector->calibration_valid)
    {
        return GEAR_INVALID;
    }

    if (IsRealGear(selector->stable_gear))
    {
        const GearZone_t *stable_zone =
            &selector->calibration->zones[selector->stable_gear];

        if (WindowContains(&stable_zone->entry_window,
                           &selector->filtered,
                           selector->calibration->hysteresis_counts))
        {
            return selector->stable_gear;
        }
    }

    GearPosition_t match = GEAR_INVALID;
    uint32_t match_count = 0U;

    for (uint32_t gear = 0U; gear < (uint32_t)GEAR_COUNT; ++gear)
    {
        const GearZone_t *zone = &selector->calibration->zones[gear];

        if (WindowContains(&zone->entry_window, &selector->filtered, 0U))
        {
            match = (GearPosition_t)gear;
            ++match_count;
        }
    }

    return (match_count == 1U) ? match : GEAR_INVALID;
}

void GearSelector_Init(GearSelector_t *selector,
                       const GearCalibration_t *calibration,
                       uint32_t now_ms)
{
    if (selector == NULL)
    {
        return;
    }

    selector->calibration = calibration;
    selector->filtered = (MagneticVector_t){0, 0, 0};
    selector->stable_gear = GEAR_INVALID;
    selector->candidate_gear = GEAR_INVALID;
    selector->candidate_since_ms = now_ms;
    selector->filter_initialized = false;
    selector->calibration_valid = GearCalibration_IsValid(calibration);
}

bool GearSelector_Update(GearSelector_t *selector,
                         const MagneticVector_t *raw_field,
                         uint32_t now_ms)
{
    if ((selector == NULL) || (raw_field == NULL))
    {
        return false;
    }

    FilterSample(selector, raw_field);
    const GearPosition_t classified = Classify(selector);

    if (classified != selector->candidate_gear)
    {
        selector->candidate_gear = classified;
        selector->candidate_since_ms = now_ms;
        return false;
    }

    const uint32_t stable_time_ms = (selector->calibration == NULL) ?
                                    0U : selector->calibration->stable_time_ms;

    if ((classified != selector->stable_gear) &&
        ((uint32_t)(now_ms - selector->candidate_since_ms) >=
         stable_time_ms))
    {
        selector->stable_gear = classified;
        return true;
    }

    return false;
}

bool GearSelector_Invalidate(GearSelector_t *selector, uint32_t now_ms)
{
    if (selector == NULL)
    {
        return false;
    }

    const bool changed = selector->stable_gear != GEAR_INVALID;
    selector->stable_gear = GEAR_INVALID;
    selector->candidate_gear = GEAR_INVALID;
    selector->candidate_since_ms = now_ms;
    selector->filter_initialized = false;
    return changed;
}

GearPosition_t GearSelector_GetGear(const GearSelector_t *selector)
{
    return (selector == NULL) ? GEAR_INVALID : selector->stable_gear;
}

const MagneticVector_t *GearSelector_GetFiltered(const GearSelector_t *selector)
{
    return (selector == NULL) ? NULL : &selector->filtered;
}

bool GearSelector_HasValidCalibration(const GearSelector_t *selector)
{
    return (selector != NULL) && selector->calibration_valid;
}
