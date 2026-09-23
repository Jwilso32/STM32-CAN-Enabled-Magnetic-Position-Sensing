#include "app_config.h"
#include "stm32u3xx_hal.h"

int _write(int file, char *data, int length)
{
    (void)file;

#if APP_ENABLE_ITM_TRACE
    for (int index = 0; index < length; ++index)
    {
        ITM_SendChar((uint32_t)(uint8_t)data[index]);
    }
#else
    (void)data;
#endif

    return length;
}
