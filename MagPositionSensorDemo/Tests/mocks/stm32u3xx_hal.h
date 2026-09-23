#ifndef TEST_STM32U3XX_HAL_H
#define TEST_STM32U3XX_HAL_H

#include <stdint.h>

typedef enum
{
    HAL_OK = 0,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

typedef struct
{
    uint32_t dummy;
} FDCAN_HandleTypeDef;

typedef struct
{
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

#define FDCAN_EXTENDED_ID       1U
#define FDCAN_DATA_FRAME        2U
#define FDCAN_DLC_BYTES_8       8U
#define FDCAN_ESI_ACTIVE        3U
#define FDCAN_BRS_OFF           4U
#define FDCAN_CLASSIC_CAN       5U
#define FDCAN_NO_TX_EVENTS      6U
#define FDCAN_REJECT            7U
#define FDCAN_REJECT_REMOTE     8U

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *fdcan,
                                                uint32_t nonmatching_standard,
                                                uint32_t nonmatching_extended,
                                                uint32_t standard_remote,
                                                uint32_t extended_remote);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *fdcan);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(
    FDCAN_HandleTypeDef *fdcan,
    const FDCAN_TxHeaderTypeDef *header,
    const uint8_t *payload);

#endif /* TEST_STM32U3XX_HAL_H */
