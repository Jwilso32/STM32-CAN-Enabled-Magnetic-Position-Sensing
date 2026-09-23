#ifndef KEYPAD_CAN_H
#define KEYPAD_CAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_types.h"
#include "stm32u3xx_hal.h"

#define KEYPAD_CAN_IDENTIFIER       (0x18EFFF21UL)
#define KEYPAD_CAN_PAYLOAD_SIZE     8U

typedef enum
{
    KEYPAD_BUTTON_NONE = 0x00U,
    KEYPAD_BUTTON_PARK = 0x01U,
    KEYPAD_BUTTON_REVERSE = 0x02U,
    KEYPAD_BUTTON_NEUTRAL = 0x03U,
    KEYPAD_BUTTON_DRIVE = 0x04U
} KeypadButtonCode_t;

typedef enum
{
    KEYPAD_CAN_PULSE_IDLE = 0,
    KEYPAD_CAN_PULSE_PRESS_PENDING,
    KEYPAD_CAN_PULSE_HELD,
    KEYPAD_CAN_PULSE_RELEASE_PENDING
} KeypadCanPulseState_t;

typedef struct
{
    FDCAN_HandleTypeDef *fdcan;
    FDCAN_TxHeaderTypeDef tx_header;
    uint32_t next_heartbeat_ms;
    uint32_t press_started_ms;
    KeypadButtonCode_t pulse_button;
    KeypadButtonCode_t active_button;
    KeypadButtonCode_t queued_button;
    KeypadCanPulseState_t pulse_state;
    uint8_t heartbeat_counter;
    bool active_heartbeat_sent;
    bool healthy;
    bool initialized;
} KeypadCan_t;

/* Pure protocol helpers: these functions do not access hardware or module state. */
KeypadButtonCode_t KeypadCan_ButtonForGear(GearPosition_t gear);
bool KeypadCan_BuildHeartbeat(uint8_t counter,
                             KeypadButtonCode_t active_button,
                             uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE]);
bool KeypadCan_BuildButtonEvent(KeypadButtonCode_t button,
                               bool pressed,
                               uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE]);

/*
 * Initialize and start the controller after MX_FDCAN1_Init(). The CubeMX
 * configuration must enable automatic retransmission for reliable edge frames.
 */
HAL_StatusTypeDef KeypadCan_Init(KeypadCan_t *keypad,
                                FDCAN_HandleTypeDef *fdcan,
                                uint32_t now_ms);

/* Request a pulse for a newly committed, stable gear position. */
bool KeypadCan_RequestGear(KeypadCan_t *keypad, GearPosition_t gear);

/* Drop a not-yet-sent/queued selection; an already pressed key is still released. */
void KeypadCan_CancelPendingGear(KeypadCan_t *keypad);

/* Call frequently from the foreground cooperative scheduler. */
void KeypadCan_Service(KeypadCan_t *keypad, uint32_t now_ms);

bool KeypadCan_IsHealthy(const KeypadCan_t *keypad);

#endif /* KEYPAD_CAN_H */
