#include "gear_calibration.h"
#include "gear_selector.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static GearCalibration_t MakeValidCalibration(void)
{
    GearCalibration_t calibration =
    {
        .hysteresis_counts = 5U,
        .stable_time_ms = 20U,
        .filter_shift = 0U
    };

    for (uint32_t gear = 0U; gear < (uint32_t)GEAR_COUNT; ++gear)
    {
        const int16_t center = (int16_t)(gear * 100);
        calibration.zones[gear].enabled = true;
        calibration.zones[gear].entry_window.minimum =
            (MagneticVector_t){center, -5, -5};
        calibration.zones[gear].entry_window.maximum =
            (MagneticVector_t){(int16_t)(center + 10), 5, 5};
    }

    return calibration;
}

int main(void)
{
    assert(!GearCalibration_IsValid(&g_gear_calibration));

    GearCalibration_t calibration = MakeValidCalibration();
    assert(GearCalibration_IsValid(&calibration));

    GearCalibration_t overlapping = calibration;
    overlapping.zones[GEAR_REVERSE].entry_window.minimum.x = 10;
    assert(!GearCalibration_IsValid(&overlapping));

    GearSelector_t selector;
    GearSelector_Init(&selector, &calibration, 0U);
    assert(GearSelector_HasValidCalibration(&selector));
    assert(GearSelector_GetGear(&selector) == GEAR_INVALID);

    MagneticVector_t sample = {5, 0, 0};
    assert(!GearSelector_Update(&selector, &sample, 0U));
    assert(!GearSelector_Update(&selector, &sample, 19U));
    assert(GearSelector_Update(&selector, &sample, 20U));
    assert(GearSelector_GetGear(&selector) == GEAR_PARK);

    /* The Park entry box ends at 10, but the held box extends to 15. */
    sample.x = 14;
    assert(!GearSelector_Update(&selector, &sample, 25U));
    assert(GearSelector_GetGear(&selector) == GEAR_PARK);

    sample.x = 16;
    assert(!GearSelector_Update(&selector, &sample, 30U));
    assert(!GearSelector_Update(&selector, &sample, 49U));
    assert(GearSelector_Update(&selector, &sample, 50U));
    assert(GearSelector_GetGear(&selector) == GEAR_INVALID);

    sample.x = 105;
    assert(!GearSelector_Update(&selector, &sample, 60U));
    assert(GearSelector_Update(&selector, &sample, 80U));
    assert(GearSelector_GetGear(&selector) == GEAR_REVERSE);

    assert(GearSelector_Invalidate(&selector, 81U));
    assert(GearSelector_GetGear(&selector) == GEAR_INVALID);

    puts("gear selector tests passed");
    return 0;
}
