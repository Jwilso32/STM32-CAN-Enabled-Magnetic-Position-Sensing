#include "app.h"

#include "ah4931q.h"
#include "app_config.h"
#include "app_display.h"
#include "board_io.h"
#include "calibration_mode.h"
#include "gear_calibration.h"
#include "gear_selector.h"
#include "keypad_can.h"

#include <stdbool.h>
#include <stdio.h>

#define SENSOR_ERRORS_BEFORE_REINIT    5U

typedef struct
{
    AH4931Q_t sensor;
    GearSelector_t gear_selector;
    KeypadCan_t keypad;
    MagneticSample_t sample;
    I2C_HandleTypeDef *sensor_i2c;
    uint32_t last_sensor_ms;
    uint32_t last_display_ms;
    uint32_t next_sensor_retry_ms;
    uint32_t sensor_error_count;
    uint8_t consecutive_sensor_errors;
    bool sensor_initialized;
    bool sensor_ok;
    bool initialized;
} AppContext_t;

static AppContext_t app;

static bool TimeReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static bool PeriodElapsed(uint32_t now_ms,
                          uint32_t *last_run_ms,
                          uint32_t period_ms)
{
    if ((uint32_t)(now_ms - *last_run_ms) < period_ms)
    {
        return false;
    }

    *last_run_ms += period_ms;
    if ((uint32_t)(now_ms - *last_run_ms) >= period_ms)
    {
        /* Skip missed slots instead of running a burst of stale work. */
        *last_run_ms = now_ms;
    }
    return true;
}

static void TryInitializeSensor(uint32_t now_ms)
{
    const HAL_StatusTypeDef status =
        AH4931Q_Init(&app.sensor,
                     app.sensor_i2c,
                     AH4931Q_DEFAULT_ADDRESS_7BIT);

    app.sensor_initialized = status == HAL_OK;
    app.sensor_ok = false;
    app.consecutive_sensor_errors = 0U;
    app.next_sensor_retry_ms = now_ms + APP_SENSOR_RETRY_PERIOD_MS;
}

static void ServiceSensor(uint32_t now_ms)
{
    if (!app.sensor_initialized)
    {
        if (TimeReached(now_ms, app.next_sensor_retry_ms))
        {
            TryInitializeSensor(now_ms);
        }

        if (!app.sensor_initialized)
        {
            (void)GearSelector_Invalidate(&app.gear_selector, now_ms);
            return;
        }
    }

    if (!PeriodElapsed(now_ms, &app.last_sensor_ms, APP_SENSOR_PERIOD_MS))
    {
        return;
    }

    if (AH4931Q_Read(&app.sensor, &app.sample) == HAL_OK)
    {
        app.sensor_ok = true;
        app.consecutive_sensor_errors = 0U;

        const bool gear_changed =
            GearSelector_Update(&app.gear_selector,
                                &app.sample.field,
                                now_ms);

        if (gear_changed && !CalibrationMode_IsActive())
        {
            const GearPosition_t gear =
                GearSelector_GetGear(&app.gear_selector);

            if (gear == GEAR_INVALID)
            {
                KeypadCan_CancelPendingGear(&app.keypad);
            }
            else
            {
                (void)KeypadCan_RequestGear(&app.keypad, gear);
            }
        }
    }
    else
    {
        app.sensor_ok = false;
        ++app.sensor_error_count;
        ++app.consecutive_sensor_errors;
        if (GearSelector_Invalidate(&app.gear_selector, now_ms) &&
            !CalibrationMode_IsActive())
        {
            KeypadCan_CancelPendingGear(&app.keypad);
        }

        if (app.consecutive_sensor_errors >= SENSOR_ERRORS_BEFORE_REINIT)
        {
            app.sensor_initialized = false;
            app.next_sensor_retry_ms = now_ms + APP_SENSOR_RETRY_PERIOD_MS;
        }
    }

    CalibrationMode_Update(&app.sample.field,
                           GearSelector_GetFiltered(&app.gear_selector),
                           GearSelector_GetGear(&app.gear_selector),
                           app.sensor_ok,
                           now_ms);
}

HAL_StatusTypeDef App_Init(I2C_HandleTypeDef *sensor_i2c,
                           FDCAN_HandleTypeDef *fdcan)
{
    if ((sensor_i2c == NULL) || (fdcan == NULL))
    {
        return HAL_ERROR;
    }

    app = (AppContext_t){0};
    app.sensor_i2c = sensor_i2c;

    const uint32_t now_ms = HAL_GetTick();
    app.last_sensor_ms = now_ms - APP_SENSOR_PERIOD_MS;
    app.last_display_ms = now_ms - APP_DISPLAY_PERIOD_MS;

    BoardIO_Init();
    GearSelector_Init(&app.gear_selector, &g_gear_calibration, now_ms);

    const bool calibration_requested =
        BoardIO_IsCalibrationRequested() ||
        !GearSelector_HasValidCalibration(&app.gear_selector);
    CalibrationMode_Init(calibration_requested, now_ms);

    TryInitializeSensor(now_ms);
    AppDisplay_Init();

    if (KeypadCan_Init(&app.keypad, fdcan, now_ms) != HAL_OK)
    {
        return HAL_ERROR;
    }

#if APP_ENABLE_ITM_TRACE
    printf("Magnetic gear selector started: mode=%s calibration=%s\r\n",
           CalibrationMode_IsActive() ? "CAL" : "RUN",
           GearSelector_HasValidCalibration(&app.gear_selector) ?
               "VALID" : "NOT_CONFIGURED");
#endif

    app.initialized = true;
    return HAL_OK;
}

void App_Run(void)
{
    if (!app.initialized)
    {
        return;
    }

    const uint32_t now_ms = HAL_GetTick();

    /* CAN is serviced first so display/sensor I2C traffic cannot starve it. */
    KeypadCan_Service(&app.keypad, now_ms);
    ServiceSensor(now_ms);

    if (PeriodElapsed(now_ms,
                      &app.last_display_ms,
                      APP_DISPLAY_PERIOD_MS))
    {
        AppDisplay_Update(&app.sample.field,
                          GearSelector_GetGear(&app.gear_selector),
                          app.sensor_ok,
                          KeypadCan_IsHealthy(&app.keypad),
                          CalibrationMode_IsActive());
    }
}
