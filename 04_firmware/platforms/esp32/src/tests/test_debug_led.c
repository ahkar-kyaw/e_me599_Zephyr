#include "test_debug_led.h"

#include "board_esp32_nodemcu_v1.h"
#include "board_led.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>

#define TEST_DEBUG_LED_PERIOD_MS       BOARD_DEBUG_LED_PERIOD_MS
#define TEST_DEBUG_LED_STACK_BYTES     2048u
#define TEST_DEBUG_LED_PRIORITY        (tskIDLE_PRIORITY + 1)

static const char *TAG = "test_debug_led";

static void test_debug_led_entry(void *argument);

void test_debug_led_start(void)
{
    BaseType_t ok = xTaskCreate(
        test_debug_led_entry,
        "test_debug_led",
        TEST_DEBUG_LED_STACK_BYTES,
        NULL,
        TEST_DEBUG_LED_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create test_debug_led");
    }
}

static void test_debug_led_entry(void *argument)
{
    (void)argument;

    uint32_t count = 0u;

    board_led_init();

    ESP_LOGI(TAG,
             "debug LED bring-up test started on GPIO%d",
             (int)BOARD_DEBUG_LED1_GPIO);

    for (;;)
    {
        board_led_toggle(BOARD_LED_DEBUG1);

        ESP_LOGI(TAG,
                 "debug LED toggle count=%lu",
                 (unsigned long)count);

        count++;

        vTaskDelay(pdMS_TO_TICKS(TEST_DEBUG_LED_PERIOD_MS));
    }
}