#include "ah4931q.h"

#include <stddef.h>

#define AH4931Q_TRANSFER_TIMEOUT_MS     5U
#define AH4931Q_SAMPLE_SIZE             7U

static int16_t SignExtend12(uint16_t value)
{
    value &= 0x0FFFU;
    if ((value & 0x0800U) != 0U)
    {
        value |= 0xF000U;
    }
    return (int16_t)value;
}

HAL_StatusTypeDef AH4931Q_Init(AH4931Q_t *device,
                               I2C_HandleTypeDef *i2c,
                               uint8_t address_7bit)
{
    if ((device == NULL) || (i2c == NULL))
    {
        return HAL_ERROR;
    }

    device->i2c = i2c;
    device->address = (uint16_t)address_7bit << 1;
    device->timeout_ms = AH4931Q_TRANSFER_TIMEOUT_MS;

    if (HAL_I2C_IsDeviceReady(device->i2c,
                              device->address,
                              1U,
                              device->timeout_ms) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Start at register 0, then configure MOD1 and MOD2 for this application. */
    uint8_t configuration[] =
    {
        0x00U,
        0x01U,
        0x00U,
        0x40U
    };

    return HAL_I2C_Master_Transmit(device->i2c,
                                   device->address,
                                   configuration,
                                   (uint16_t)sizeof(configuration),
                                   device->timeout_ms);
}

HAL_StatusTypeDef AH4931Q_Read(AH4931Q_t *device,
                               MagneticSample_t *sample)
{
    if ((device == NULL) || (device->i2c == NULL) || (sample == NULL))
    {
        return HAL_ERROR;
    }

    uint8_t registers[AH4931Q_SAMPLE_SIZE];
    const HAL_StatusTypeDef status =
        HAL_I2C_Master_Receive(device->i2c,
                               device->address,
                               registers,
                               AH4931Q_SAMPLE_SIZE,
                               device->timeout_ms);

    if (status != HAL_OK)
    {
        return status;
    }

    const uint16_t raw_x = ((uint16_t)registers[0] << 4) |
                           ((uint16_t)registers[4] >> 4);
    const uint16_t raw_y = ((uint16_t)registers[1] << 4) |
                           ((uint16_t)registers[4] & 0x0FU);
    const uint16_t raw_z = ((uint16_t)registers[2] << 4) |
                           ((uint16_t)registers[5] & 0x0FU);
    const uint16_t raw_temperature =
        ((uint16_t)(registers[3] & 0xF0U) << 4) | registers[6];

    sample->field.x = SignExtend12(raw_x);
    sample->field.y = SignExtend12(raw_y);
    sample->field.z = SignExtend12(raw_z);
    sample->temperature_raw = SignExtend12(raw_temperature);
    sample->frame_number = (registers[3] >> 2) & 0x03U;
    sample->channel = registers[3] & 0x03U;

    return HAL_OK;
}
