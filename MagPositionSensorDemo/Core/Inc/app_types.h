#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

typedef enum
{
    GEAR_PARK = 0,
    GEAR_REVERSE,
    GEAR_NEUTRAL,
    GEAR_DRIVE,
    GEAR_COUNT,
    GEAR_INVALID = 0xFF
} GearPosition_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} MagneticVector_t;

typedef struct
{
    MagneticVector_t field;
    int16_t temperature_raw;
    uint8_t frame_number;
    uint8_t channel;
} MagneticSample_t;

#endif /* APP_TYPES_H */
