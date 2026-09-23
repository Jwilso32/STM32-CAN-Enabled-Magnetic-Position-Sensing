#ifndef GEAR_SELECTOR_H
#define GEAR_SELECTOR_H

#include "gear_calibration.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    const GearCalibration_t *calibration;
    MagneticVector_t filtered;
    GearPosition_t stable_gear;
    GearPosition_t candidate_gear;
    uint32_t candidate_since_ms;
    bool filter_initialized;
    bool calibration_valid;
} GearSelector_t;

void GearSelector_Init(GearSelector_t *selector,
                       const GearCalibration_t *calibration,
                       uint32_t now_ms);

/* Returns true only when the stable output changes. */
bool GearSelector_Update(GearSelector_t *selector,
                         const MagneticVector_t *raw_field,
                         uint32_t now_ms);

/* Sensor faults invalidate the output immediately; recovery is debounced. */
bool GearSelector_Invalidate(GearSelector_t *selector, uint32_t now_ms);

GearPosition_t GearSelector_GetGear(const GearSelector_t *selector);
const MagneticVector_t *GearSelector_GetFiltered(const GearSelector_t *selector);
bool GearSelector_HasValidCalibration(const GearSelector_t *selector);

#endif /* GEAR_SELECTOR_H */
