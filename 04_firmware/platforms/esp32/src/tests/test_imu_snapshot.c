#include "test_imu_snapshot.h"

#include "config_imu.h"
#include "control/ctrl_balance_types.h"
#include "safety/safety_imu.h"
#include "task_imu.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_IMU_SNAPSHOT_STACK_BYTES    4096u
#define TEST_IMU_SNAPSHOT_PRIORITY       (tskIDLE_PRIORITY + 1)
#define TEST_IMU_SNAPSHOT_PERIOD_MS      500u

#define TEST_IMU_RAD_TO_DEG              57.2957795f

static const char *TAG = "test_imu_snapshot";

static void test_imu_snapshot_entry(void *argument);

void test_imu_snapshot_start(void)
{
    BaseType_t ok = xTaskCreate(
        test_imu_snapshot_entry,
        "test_imu_snapshot",
        TEST_IMU_SNAPSHOT_STACK_BYTES,
        NULL,
        TEST_IMU_SNAPSHOT_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

static void test_imu_snapshot_entry(void *argument)
{
    (void)argument;

    safety_imu_config_t safety_config = safety_imu_default_config();

    safety_config.max_age_us = CONFIG_IMU_MAX_AGE_US;
    safety_config.max_abs_roll_rad = CONFIG_IMU_MAX_ABS_ROLL_RAD;
    safety_config.max_abs_pitch_rad = CONFIG_IMU_MAX_ABS_PITCH_RAD;
    safety_config.min_accel_norm_mps2 = CONFIG_IMU_MIN_ACCEL_NORM_MPS2;
    safety_config.max_accel_norm_mps2 = CONFIG_IMU_MAX_ACCEL_NORM_MPS2;

    for (;;)
    {
        imu_snapshot_t snapshot;
        safety_imu_status_t imu_status;
        balance_state_t balance_state;

        task_imu_get_snapshot(&snapshot);

        safety_imu_check(&snapshot,
                         (uint64_t)esp_timer_get_time(),
                         &safety_config,
                         &imu_status);

        ctrl_balance_state_from_imu(&snapshot,
                                    &imu_status,
                                    &balance_state);

        ESP_LOGI(TAG,
                 "valid=%d calibrated=%d safe=%d faults=0x%08lx age_us=%llu sample=%lu errors=%lu",
                 snapshot.valid ? 1 : 0,
                 snapshot.calibrated ? 1 : 0,
                 imu_status.safe_for_balance ? 1 : 0,
                 (unsigned long)imu_status.fault_flags,
                 (unsigned long long)imu_status.age_us,
                 (unsigned long)snapshot.sample_count,
                 (unsigned long)snapshot.read_error_count);

        ESP_LOGI(TAG,
                 "accel_norm=%7.3f m/s2 accel [%7.3f %7.3f %7.3f] gyro [%8.5f %8.5f %8.5f] rps",
                 imu_status.accel_norm_mps2,
                 snapshot.data.accel_mps2[0],
                 snapshot.data.accel_mps2[1],
                 snapshot.data.accel_mps2[2],
                 snapshot.data.gyro_rps[0],
                 snapshot.data.gyro_rps[1],
                 snapshot.data.gyro_rps[2]);

        ESP_LOGI(TAG,
                 "att deg roll[%7.2f] pitch[%7.2f] yaw[%7.2f]",
                 snapshot.attitude.roll_rad * TEST_IMU_RAD_TO_DEG,
                 snapshot.attitude.pitch_rad * TEST_IMU_RAD_TO_DEG,
                 snapshot.attitude.yaw_rad * TEST_IMU_RAD_TO_DEG);

        ESP_LOGI(TAG,
                 "balance valid=%d pitch[%7.2f deg] pitch_rate[%8.5f rps]",
                 balance_state.valid ? 1 : 0,
                 balance_state.pitch_rad * TEST_IMU_RAD_TO_DEG,
                 balance_state.pitch_rate_rps);

        vTaskDelay(pdMS_TO_TICKS(TEST_IMU_SNAPSHOT_PERIOD_MS));
    }
}