#include "task_imu.h"

#include "esp_log.h"

#if APP_ENABLE_BRINGUP_TESTS
#include "test_debug_led.h"
#include "test_imu_snapshot.h"
#endif

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 firmware booting");

    task_imu_start();

#if APP_ENABLE_BRINGUP_TESTS
    ESP_LOGI(TAG, "bring-up tests enabled");

    test_debug_led_start();
    test_imu_snapshot_start();
#else
    ESP_LOGI(TAG, "bring-up tests disabled");
#endif
}