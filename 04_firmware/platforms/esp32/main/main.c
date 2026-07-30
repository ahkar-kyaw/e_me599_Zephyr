#include "task_imu.h"
#include "task_rc.h"
#include "task_ui.h"

#include "esp_log.h"

#if APP_ENABLE_BRINGUP_TESTS
#include "test_debug_led.h"
#include "test_imu_snapshot.h"
#include "test_rc_snapshot.h"
#endif

static const char *TAG = "app_main";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 firmware booting");

    task_imu_start();

#if APP_ENABLE_RC_RECEIVER
    task_rc_start();
#endif

#if APP_ENABLE_UI
    task_ui_start();
#endif

#if APP_ENABLE_BRINGUP_TESTS
    ESP_LOGI(TAG, "bring-up tests enabled");

    test_debug_led_start();
    test_imu_snapshot_start();

#if APP_ENABLE_RC_RECEIVER
    test_rc_snapshot_start();
#endif
#else
    ESP_LOGI(TAG, "bring-up tests disabled");
#endif
}
