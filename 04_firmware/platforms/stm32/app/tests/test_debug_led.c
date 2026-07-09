#include "test_debug_led.h"

#include "board_led.h"
#include "cmsis_os2.h"

#include <stdint.h>

#define TEST_DEBUG_LED_STEP_MS        150u
#define TEST_DEBUG_LED_PAUSE_MS       500u
#define TEST_DEBUG_LED_STACK_BYTES    1024u
#define TEST_DEBUG_LED_PRIORITY       osPriorityLow

static osThreadId_t test_debug_led_handle;

static void test_debug_led_entry(void *argument);

void test_debug_led_start(void)
{
    static const osThreadAttr_t test_debug_led_attributes =
    {
        .name = "test_debug_led",
        .stack_size = TEST_DEBUG_LED_STACK_BYTES,
        .priority = TEST_DEBUG_LED_PRIORITY,
    };

    test_debug_led_handle = osThreadNew(test_debug_led_entry,
                                        NULL,
                                        &test_debug_led_attributes);

    (void)test_debug_led_handle;
}

static void test_debug_led_entry(void *argument)
{
    (void)argument;

    board_led_init();

    for (;;)
    {
        board_led_toggle(BOARD_LED_DEBUG1);
        osDelay(TEST_DEBUG_LED_STEP_MS);

        board_led_toggle(BOARD_LED_DEBUG2);
        osDelay(TEST_DEBUG_LED_STEP_MS);

        board_led_toggle(BOARD_LED_DEBUG3);
        osDelay(TEST_DEBUG_LED_PAUSE_MS);
    }
}