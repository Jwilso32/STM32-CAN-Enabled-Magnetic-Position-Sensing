#ifndef AH4931Q_H
#define AH4931Q_H

#include "app_types.h"
#include "stm32u3xx_hal.h"

#include <stdint.h>

#define AH4931Q_DEFAULT_ADDRESS_7BIT   0x5EU

typedef struct
{
    I2C_HandleTypeDef *i2c;
    uint16_t address;
    uint32_t timeout_ms;
} AH4931Q_t;

/* Initializes the device object and writes the sensor operating-mode registers. */
HAL_StatusTypeDef AH4931Q_Init(AH4931Q_t *device,
                               I2C_HandleTypeDef *i2c,
                               uint8_t address_7bit);

HAL_StatusTypeDef AH4931Q_Read(AH4931Q_t *device,
                               MagneticSample_t *sample);

#endif /* AH4931Q_H */
