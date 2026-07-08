#include "board_esp32_nodemcu_v1.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "blink_test";

static void board_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOARD_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(BOARD_LED_GPIO, BOARD_LED_INACTIVE_LEVEL));
}

static void task_blink(void *argument)
{
    (void)argument;

    uint32_t count = 0u;

    board_led_init();
    ESP_LOGI(TAG, "task_blink started on GPIO%d", (int)BOARD_LED_GPIO);

    for (;;)
    {
        ESP_ERROR_CHECK(gpio_set_level(BOARD_LED_GPIO, BOARD_LED_ACTIVE_LEVEL));
        ESP_LOGI(TAG, "LED ON  count=%lu", (unsigned long)count);
        vTaskDelay(pdMS_TO_TICKS(BOARD_LED_PERIOD_MS));

        ESP_ERROR_CHECK(gpio_set_level(BOARD_LED_GPIO, BOARD_LED_INACTIVE_LEVEL));
        ESP_LOGI(TAG, "LED OFF count=%lu", (unsigned long)count);
        vTaskDelay(pdMS_TO_TICKS(BOARD_LED_PERIOD_MS));

        count++;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 ESP-IDF blink test booting");

    BaseType_t ok = xTaskCreate(
        task_blink,
        "task_blink",
        2048,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create task_blink");
    }
}