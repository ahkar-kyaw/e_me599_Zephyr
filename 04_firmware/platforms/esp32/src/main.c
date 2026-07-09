#include "esp_log.h"

#if APP_ENABLE_BRINGUP_TESTS
#include "test_debug_led.h"
#endif

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 firmware booting");

#if APP_ENABLE_BRINGUP_TESTS
    ESP_LOGI(TAG, "bring-up tests enabled");
    test_debug_led_start();
#else
    ESP_LOGI(TAG, "bring-up tests disabled");
#endif
}