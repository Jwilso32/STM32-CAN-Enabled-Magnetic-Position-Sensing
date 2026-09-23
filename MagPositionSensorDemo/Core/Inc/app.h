#ifndef APP_H
#define APP_H

#include "stm32u3xx_hal.h"

HAL_StatusTypeDef App_Init(I2C_HandleTypeDef *sensor_i2c,
                           FDCAN_HandleTypeDef *fdcan);
void App_Run(void);

#endif /* APP_H */
