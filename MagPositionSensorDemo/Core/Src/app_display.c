#include "app_display.h"

#include "app_config.h"

#include <stddef.h>

#if APP_ENABLE_BENCH_DISPLAY
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>

static const char *GearName(GearPosition_t gear)
{
    switch (gear)
    {
        case GEAR_PARK:    return "P";
        case GEAR_REVERSE: return "R";
        case GEAR_NEUTRAL: return "N";
        case GEAR_DRIVE:   return "D";
        default:           return "X";
    }
}
#endif

void AppDisplay_Init(void)
{
#if APP_ENABLE_BENCH_DISPLAY
    ssd1306_Init();
#endif
}

void AppDisplay_Update(const MagneticVector_t *field,
                       GearPosition_t gear,
                       bool sensor_ok,
                       bool can_ok,
                       bool calibration_mode)
{
    if (field == NULL)
    {
        return;
    }

#if APP_ENABLE_BENCH_DISPLAY
    char text[24];

    ssd1306_Fill(Black);

    if (calibration_mode)
    {
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("CAL RAW", Font_7x10, White);

        (void)snprintf(text, sizeof(text), "X:%+5d", field->x);
        ssd1306_SetCursor(0, 18);
        ssd1306_WriteString(text, Font_7x10, White);

        (void)snprintf(text, sizeof(text), "Y:%+5d", field->y);
        ssd1306_SetCursor(0, 30);
        ssd1306_WriteString(text, Font_7x10, White);

        (void)snprintf(text, sizeof(text), "Z:%+5d", field->z);
        ssd1306_SetCursor(0, 42);
        ssd1306_WriteString(text, Font_7x10, White);
    }
    else
    {
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("GEAR", Font_7x10, White);
        ssd1306_SetCursor(55, 18);
        ssd1306_WriteString((char *)GearName(gear), Font_16x26, White);

        (void)snprintf(text,
                       sizeof(text),
                       "SNS:%s CAN:%s",
                       sensor_ok ? "OK" : "ER",
                       can_ok ? "OK" : "ER");
        ssd1306_SetCursor(0, 52);
        ssd1306_WriteString(text, Font_7x10, White);
    }

    ssd1306_UpdateScreen();
#else
    (void)field;
    (void)gear;
    (void)sensor_ok;
    (void)can_ok;
    (void)calibration_mode;
#endif
}
