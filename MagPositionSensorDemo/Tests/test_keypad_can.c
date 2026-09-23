#include "keypad_can.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_CAPTURED_FRAMES  16U

static uint8_t captured[MAX_CAPTURED_FRAMES][KEYPAD_CAN_PAYLOAD_SIZE];
static uint32_t captured_count;
static bool fail_next_enqueue;

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *fdcan,
                                                uint32_t nonmatching_standard,
                                                uint32_t nonmatching_extended,
                                                uint32_t standard_remote,
                                                uint32_t extended_remote)
{
    (void)fdcan;
    (void)nonmatching_standard;
    (void)nonmatching_extended;
    (void)standard_remote;
    (void)extended_remote;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *fdcan)
{
    (void)fdcan;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(
    FDCAN_HandleTypeDef *fdcan,
    const FDCAN_TxHeaderTypeDef *header,
    const uint8_t *payload)
{
    (void)fdcan;
    assert(header->Identifier == KEYPAD_CAN_IDENTIFIER);
    assert(header->IdType == FDCAN_EXTENDED_ID);

    if (fail_next_enqueue)
    {
        fail_next_enqueue = false;
        return HAL_BUSY;
    }

    assert(captured_count < MAX_CAPTURED_FRAMES);
    memcpy(captured[captured_count], payload, KEYPAD_CAN_PAYLOAD_SIZE);
    ++captured_count;
    return HAL_OK;
}

static void ExpectFrame(uint32_t index, const uint8_t expected[8])
{
    assert(index < captured_count);
    assert(memcmp(captured[index], expected, 8U) == 0);
}

int main(void)
{
    assert(KeypadCan_ButtonForGear(GEAR_PARK) == KEYPAD_BUTTON_PARK);
    assert(KeypadCan_ButtonForGear(GEAR_REVERSE) == KEYPAD_BUTTON_REVERSE);
    assert(KeypadCan_ButtonForGear(GEAR_NEUTRAL) == KEYPAD_BUTTON_NEUTRAL);
    assert(KeypadCan_ButtonForGear(GEAR_DRIVE) == KEYPAD_BUTTON_DRIVE);
    assert(KeypadCan_ButtonForGear(GEAR_INVALID) == KEYPAD_BUTTON_NONE);

    uint8_t payload[8];
    const uint8_t drive_press[8] =
        {0x04, 0x1B, 0x04, 0x01, 0x01, 0x21, 0xFF, 0xFF};
    assert(KeypadCan_BuildButtonEvent(KEYPAD_BUTTON_DRIVE, true, payload));
    assert(memcmp(payload, drive_press, sizeof(payload)) == 0);

    FDCAN_HandleTypeDef fdcan = {0};
    KeypadCan_t keypad;
    assert(KeypadCan_Init(&keypad, &fdcan, 0U) == HAL_OK);

    KeypadCan_Service(&keypad, 0U);
    const uint8_t idle_heartbeat[8] =
        {0x04, 0x1B, 0xF9, 0x00, 0x00, 0x00, 0xFF, 0x21};
    ExpectFrame(0U, idle_heartbeat);

    assert(KeypadCan_RequestGear(&keypad, GEAR_REVERSE));
    KeypadCan_Service(&keypad, 1U);
    const uint8_t reverse_press[8] =
        {0x04, 0x1B, 0x02, 0x01, 0x01, 0x21, 0xFF, 0xFF};
    ExpectFrame(1U, reverse_press);

    KeypadCan_Service(&keypad, 100U);
    const uint8_t reverse_heartbeat[8] =
        {0x04, 0x1B, 0xF9, 0x01, 0x02, 0x00, 0xFF, 0x21};
    ExpectFrame(2U, reverse_heartbeat);

    KeypadCan_Service(&keypad, 171U);
    const uint8_t reverse_release[8] =
        {0x04, 0x1B, 0x02, 0x01, 0x00, 0x21, 0xFF, 0xFF};
    ExpectFrame(3U, reverse_release);

    assert(KeypadCan_RequestGear(&keypad, GEAR_NEUTRAL));
    fail_next_enqueue = true;
    KeypadCan_Service(&keypad, 172U);
    assert(captured_count == 4U);
    assert(!KeypadCan_IsHealthy(&keypad));

    KeypadCan_Service(&keypad, 173U);
    const uint8_t neutral_press[8] =
        {0x04, 0x1B, 0x03, 0x01, 0x01, 0x21, 0xFF, 0xFF};
    ExpectFrame(4U, neutral_press);

    assert(KeypadCan_RequestGear(&keypad, GEAR_DRIVE));
    KeypadCan_CancelPendingGear(&keypad);
    KeypadCan_Service(&keypad, 200U);
    const uint8_t neutral_heartbeat[8] =
        {0x04, 0x1B, 0xF9, 0x02, 0x03, 0x00, 0xFF, 0x21};
    ExpectFrame(5U, neutral_heartbeat);

    KeypadCan_Service(&keypad, 343U);
    const uint8_t neutral_release[8] =
        {0x04, 0x1B, 0x03, 0x01, 0x00, 0x21, 0xFF, 0xFF};
    ExpectFrame(6U, neutral_release);
    const uint8_t post_release_heartbeat[8] =
        {0x04, 0x1B, 0xF9, 0x03, 0x00, 0x00, 0xFF, 0x21};
    ExpectFrame(7U, post_release_heartbeat);
    assert(captured_count == 8U);

    puts("keypad CAN tests passed");
    return 0;
}
