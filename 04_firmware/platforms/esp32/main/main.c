#include "task_imu.h"
#include "task_motor.h"
#include "task_rc.h"
#include "task_safety.h"
#include "task_supervisor.h"
#include "task_ui.h"

#include "esp_log.h"

#if APP_ENABLE_BRINGUP_TESTS && APP_ENABLE_ACTUATOR_DIAGNOSTICS
#include "test_actuator_snapshot.h"
#endif

#if APP_ENABLE_BRINGUP_TESTS && APP_ENABLE_DEBUG_LED_TEST
#include "test_debug_led.h"
#endif

#if APP_ENABLE_BRINGUP_TESTS && APP_ENABLE_IMU_DIAGNOSTICS
#include "test_imu_snapshot.h"
#endif

#if APP_ENABLE_BRINGUP_TESTS && APP_ENABLE_RC_DIAGNOSTICS
#include "test_rc_snapshot.h"
#endif

static const char *TAG = "app_main";

static void app_configure_logging(void);

void app_main(void)
{
    app_configure_logging();

    ESP_LOGI(TAG, "ESP32 firmware booting");

    task_imu_start();

#if APP_ENABLE_ACTUATORS
    task_motor_start();
#endif

#if APP_ENABLE_RC_RECEIVER
    task_rc_start();
#endif

#if APP_ENABLE_UI
    task_ui_start();
#endif

#if APP_ENABLE_ACTUATORS && APP_ENABLE_RC_RECEIVER && APP_ENABLE_UI
    task_supervisor_start();
    task_safety_start();
#endif

#if APP_ENABLE_BRINGUP_TESTS
    ESP_LOGI(TAG, "bring-up tests enabled");

#if APP_ENABLE_DEBUG_LED_TEST
    test_debug_led_start();
#endif

#if APP_ENABLE_IMU_DIAGNOSTICS
    test_imu_snapshot_start();
#endif

#if APP_ENABLE_ACTUATORS && APP_ENABLE_ACTUATOR_DIAGNOSTICS
    test_actuator_snapshot_start();
#endif

#if APP_ENABLE_RC_RECEIVER && APP_ENABLE_RC_DIAGNOSTICS
    test_rc_snapshot_start();
#endif
#else
    ESP_LOGI(TAG, "bring-up tests disabled");
#endif
}

static void app_configure_logging(void)
{
#if APP_ENABLE_CAN_ONLY_LOGGING
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_level_set("task_motor", ESP_LOG_ERROR);
    esp_log_level_set("task_safety", ESP_LOG_ERROR);
    esp_log_level_set("task_supervisor", ESP_LOG_ERROR);
    esp_log_level_set("test_actuator", ESP_LOG_INFO);
#endif
}
