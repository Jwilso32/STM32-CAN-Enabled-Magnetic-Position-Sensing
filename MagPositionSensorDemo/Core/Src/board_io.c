#include "board_io.h"

#include "app_config.h"
#include "main.h"

#if APP_USE_NUCLEO_CALIBRATION_BUTTON
#include "stm32u3xx_nucleo.h"
#endif

void BoardIO_Init(void)
{
#if APP_USE_NUCLEO_CALIBRATION_BUTTON
    (void)BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);
#endif
}

bool BoardIO_IsCalibrationRequested(void)
{
#if APP_USE_NUCLEO_CALIBRATION_BUTTON
    return BSP_PB_GetState(BUTTON_USER) == BUTTON_PRESSED;
#elif defined(CAL_MODE_GPIO_Port) && defined(CAL_MODE_Pin)
    /* Recommended custom-PCB circuit: pull up and strap CAL_MODE to ground. */
    return HAL_GPIO_ReadPin(CAL_MODE_GPIO_Port, CAL_MODE_Pin) == GPIO_PIN_RESET;
#else
    return false;
#endif
}
