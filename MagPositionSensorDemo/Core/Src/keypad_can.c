#include "keypad_can.h"

#include "app_config.h"

#define KEYPAD_HEARTBEAT_MARKER_INDEX       2U
#define KEYPAD_HEARTBEAT_COUNTER_INDEX      3U
#define KEYPAD_HEARTBEAT_BUTTON_INDEX       4U
#define KEYPAD_EVENT_BUTTON_INDEX           2U
#define KEYPAD_EVENT_PRESSED_INDEX          4U

#define KEYPAD_HEARTBEAT_MARKER             0xF9U

static bool KeypadCan_IsButtonCodeValid(KeypadButtonCode_t button,
                                        bool allow_none);
static bool KeypadCan_TimeReached(uint32_t now_ms, uint32_t deadline_ms);
static void KeypadCan_ScheduleNextHeartbeat(KeypadCan_t *keypad,
                                            uint32_t now_ms);
static HAL_StatusTypeDef KeypadCan_QueuePayload(KeypadCan_t *keypad,
                                                const uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE]);
static void KeypadCan_StartPulse(KeypadCan_t *keypad,
                                 KeypadButtonCode_t button);
static void KeypadCan_FinishPulse(KeypadCan_t *keypad);

KeypadButtonCode_t KeypadCan_ButtonForGear(GearPosition_t gear)
{
    switch (gear)
    {
        case GEAR_PARK:
            return KEYPAD_BUTTON_PARK;

        case GEAR_REVERSE:
            return KEYPAD_BUTTON_REVERSE;

        case GEAR_NEUTRAL:
            return KEYPAD_BUTTON_NEUTRAL;

        case GEAR_DRIVE:
            return KEYPAD_BUTTON_DRIVE;

        case GEAR_COUNT:
        case GEAR_INVALID:
        default:
            return KEYPAD_BUTTON_NONE;
    }
}

bool KeypadCan_BuildHeartbeat(uint8_t counter,
                             KeypadButtonCode_t active_button,
                             uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE])
{
    if ((payload == NULL) ||
        !KeypadCan_IsButtonCodeValid(active_button, true))
    {
        return false;
    }

    payload[0] = 0x04U;
    payload[1] = 0x1BU;
    payload[KEYPAD_HEARTBEAT_MARKER_INDEX] = KEYPAD_HEARTBEAT_MARKER;
    payload[KEYPAD_HEARTBEAT_COUNTER_INDEX] = counter;
    payload[KEYPAD_HEARTBEAT_BUTTON_INDEX] = (uint8_t)active_button;
    payload[5] = 0x00U;
    payload[6] = 0xFFU;
    payload[7] = 0x21U;

    return true;
}

bool KeypadCan_BuildButtonEvent(KeypadButtonCode_t button,
                               bool pressed,
                               uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE])
{
    if ((payload == NULL) ||
        !KeypadCan_IsButtonCodeValid(button, false))
    {
        return false;
    }

    payload[0] = 0x04U;
    payload[1] = 0x1BU;
    payload[KEYPAD_EVENT_BUTTON_INDEX] = (uint8_t)button;
    payload[3] = 0x01U;
    payload[KEYPAD_EVENT_PRESSED_INDEX] = pressed ? 0x01U : 0x00U;
    payload[5] = 0x21U;
    payload[6] = 0xFFU;
    payload[7] = 0xFFU;

    return true;
}

HAL_StatusTypeDef KeypadCan_Init(KeypadCan_t *keypad,
                                FDCAN_HandleTypeDef *fdcan,
                                uint32_t now_ms)
{
    HAL_StatusTypeDef status;

    if ((keypad == NULL) || (fdcan == NULL))
    {
        return HAL_ERROR;
    }

    *keypad = (KeypadCan_t){0};
    keypad->fdcan = fdcan;
    keypad->pulse_button = KEYPAD_BUTTON_NONE;
    keypad->active_button = KEYPAD_BUTTON_NONE;
    keypad->queued_button = KEYPAD_BUTTON_NONE;
    keypad->pulse_state = KEYPAD_CAN_PULSE_IDLE;
    keypad->next_heartbeat_ms = now_ms;

    keypad->tx_header.Identifier = KEYPAD_CAN_IDENTIFIER;
    keypad->tx_header.IdType = FDCAN_EXTENDED_ID;
    keypad->tx_header.TxFrameType = FDCAN_DATA_FRAME;
    keypad->tx_header.DataLength = FDCAN_DLC_BYTES_8;
    keypad->tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    keypad->tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    keypad->tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    keypad->tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    keypad->tx_header.MessageMarker = 0U;

    status = HAL_FDCAN_ConfigGlobalFilter(fdcan,
                                          FDCAN_REJECT,
                                          FDCAN_REJECT,
                                          FDCAN_REJECT_REMOTE,
                                          FDCAN_REJECT_REMOTE);
    if (status != HAL_OK)
    {
        return status;
    }

    status = HAL_FDCAN_Start(fdcan);
    if (status != HAL_OK)
    {
        return status;
    }

    keypad->initialized = true;
    keypad->healthy = true;
    return HAL_OK;
}

bool KeypadCan_RequestGear(KeypadCan_t *keypad, GearPosition_t gear)
{
    KeypadButtonCode_t button;

    if ((keypad == NULL) || !keypad->initialized)
    {
        return false;
    }

    button = KeypadCan_ButtonForGear(gear);
    if (button == KEYPAD_BUTTON_NONE)
    {
        return false;
    }

    switch (keypad->pulse_state)
    {
        case KEYPAD_CAN_PULSE_IDLE:
            KeypadCan_StartPulse(keypad, button);
            break;

        case KEYPAD_CAN_PULSE_PRESS_PENDING:
            /* No edge has been queued yet, so the newest request can replace it. */
            if (button != keypad->pulse_button)
            {
                keypad->pulse_button = button;
            }
            break;

        case KEYPAD_CAN_PULSE_HELD:
        case KEYPAD_CAN_PULSE_RELEASE_PENDING:
            /* Keep only the latest stable selection while this pulse completes. */
            keypad->queued_button = button;
            break;

        default:
            return false;
    }

    return true;
}

void KeypadCan_CancelPendingGear(KeypadCan_t *keypad)
{
    if ((keypad == NULL) || !keypad->initialized)
    {
        return;
    }

    keypad->queued_button = KEYPAD_BUTTON_NONE;

    if (keypad->pulse_state == KEYPAD_CAN_PULSE_PRESS_PENDING)
    {
        keypad->pulse_button = KEYPAD_BUTTON_NONE;
        keypad->active_button = KEYPAD_BUTTON_NONE;
        keypad->active_heartbeat_sent = false;
        keypad->pulse_state = KEYPAD_CAN_PULSE_IDLE;
    }
}

void KeypadCan_Service(KeypadCan_t *keypad, uint32_t now_ms)
{
    HAL_StatusTypeDef status;
    uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE];
    bool attempted_transmission = false;
    bool all_transmissions_ok = true;

    if ((keypad == NULL) || !keypad->initialized)
    {
        return;
    }

    if (keypad->pulse_state == KEYPAD_CAN_PULSE_PRESS_PENDING)
    {
        if (KeypadCan_BuildButtonEvent(keypad->pulse_button, true, payload))
        {
            attempted_transmission = true;
            status = KeypadCan_QueuePayload(keypad, payload);
            if (status == HAL_OK)
            {
                keypad->active_button = keypad->pulse_button;
                keypad->press_started_ms = now_ms;
                keypad->active_heartbeat_sent = false;
                keypad->pulse_state = KEYPAD_CAN_PULSE_HELD;
            }
            else
            {
                all_transmissions_ok = false;
            }
        }
        else
        {
            all_transmissions_ok = false;
        }
    }
    else if ((keypad->pulse_state == KEYPAD_CAN_PULSE_HELD) &&
             keypad->active_heartbeat_sent &&
             ((uint32_t)(now_ms - keypad->press_started_ms) >=
              APP_KEYPAD_PRESS_DURATION_MS))
    {
        keypad->pulse_state = KEYPAD_CAN_PULSE_RELEASE_PENDING;
    }

    if (keypad->pulse_state == KEYPAD_CAN_PULSE_RELEASE_PENDING)
    {
        if (KeypadCan_BuildButtonEvent(keypad->pulse_button, false, payload))
        {
            attempted_transmission = true;
            status = KeypadCan_QueuePayload(keypad, payload);
            if (status == HAL_OK)
            {
                KeypadCan_FinishPulse(keypad);
            }
            else
            {
                all_transmissions_ok = false;
            }
        }
        else
        {
            all_transmissions_ok = false;
        }
    }

    if (KeypadCan_TimeReached(now_ms, keypad->next_heartbeat_ms))
    {
        KeypadCan_ScheduleNextHeartbeat(keypad, now_ms);

        if (KeypadCan_BuildHeartbeat(keypad->heartbeat_counter,
                                    keypad->active_button,
                                    payload))
        {
            attempted_transmission = true;
            status = KeypadCan_QueuePayload(keypad, payload);
            if (status == HAL_OK)
            {
                keypad->heartbeat_counter =
                    (uint8_t)(keypad->heartbeat_counter + 1U);

                if (keypad->active_button != KEYPAD_BUTTON_NONE)
                {
                    keypad->active_heartbeat_sent = true;
                }
            }
            else
            {
                all_transmissions_ok = false;
            }
        }
        else
        {
            all_transmissions_ok = false;
        }
    }

    if (attempted_transmission)
    {
        keypad->healthy = all_transmissions_ok;
    }
    else if (!all_transmissions_ok)
    {
        keypad->healthy = false;
    }
}

bool KeypadCan_IsHealthy(const KeypadCan_t *keypad)
{
    return (keypad != NULL) && keypad->initialized && keypad->healthy;
}

static bool KeypadCan_IsButtonCodeValid(KeypadButtonCode_t button,
                                        bool allow_none)
{
    if (button == KEYPAD_BUTTON_NONE)
    {
        return allow_none;
    }

    return (button == KEYPAD_BUTTON_PARK) ||
           (button == KEYPAD_BUTTON_REVERSE) ||
           (button == KEYPAD_BUTTON_NEUTRAL) ||
           (button == KEYPAD_BUTTON_DRIVE);
}

static bool KeypadCan_TimeReached(uint32_t now_ms, uint32_t deadline_ms)
{
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static void KeypadCan_ScheduleNextHeartbeat(KeypadCan_t *keypad,
                                            uint32_t now_ms)
{
    uint32_t next_deadline = keypad->next_heartbeat_ms +
                             APP_KEYPAD_HEARTBEAT_PERIOD_MS;

    /* A delayed service sends once, then resumes from now without a burst. */
    if (KeypadCan_TimeReached(now_ms, next_deadline))
    {
        next_deadline = now_ms + APP_KEYPAD_HEARTBEAT_PERIOD_MS;
    }

    keypad->next_heartbeat_ms = next_deadline;
}

static HAL_StatusTypeDef KeypadCan_QueuePayload(
    KeypadCan_t *keypad,
    const uint8_t payload[KEYPAD_CAN_PAYLOAD_SIZE])
{
    return HAL_FDCAN_AddMessageToTxFifoQ(keypad->fdcan,
                                         &keypad->tx_header,
                                         payload);
}

static void KeypadCan_StartPulse(KeypadCan_t *keypad,
                                 KeypadButtonCode_t button)
{
    keypad->pulse_button = button;
    keypad->active_button = KEYPAD_BUTTON_NONE;
    keypad->active_heartbeat_sent = false;
    keypad->pulse_state = KEYPAD_CAN_PULSE_PRESS_PENDING;
}

static void KeypadCan_FinishPulse(KeypadCan_t *keypad)
{
    KeypadButtonCode_t queued_button = keypad->queued_button;

    keypad->pulse_button = KEYPAD_BUTTON_NONE;
    keypad->active_button = KEYPAD_BUTTON_NONE;
    keypad->queued_button = KEYPAD_BUTTON_NONE;
    keypad->active_heartbeat_sent = false;
    keypad->pulse_state = KEYPAD_CAN_PULSE_IDLE;

    if (queued_button != KEYPAD_BUTTON_NONE)
    {
        KeypadCan_StartPulse(keypad, queued_button);
    }
}
