#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include "app_types.h"

#include <stdbool.h>

void AppDisplay_Init(void);
void AppDisplay_Update(const MagneticVector_t *field,
                       GearPosition_t gear,
                       bool sensor_ok,
                       bool can_ok,
                       bool calibration_mode);

#endif /* APP_DISPLAY_H */
