#include "task_imu.h"

#include "bsp_imu_spi_esp32.h"
#include "config_imu.h"
#include "drivers/drv_ism330dhcx.h"
#include "estimation/est_attitude.h"
#include "estimation/est_imu_calibration.h"
#include "estimation/est_imu_mount.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define TASK_IMU_STACK_BYTES    4096u
#define TASK_IMU_PRIORITY       (tskIDLE_PRIORITY + 3)
#define TASK_IMU_RETRY_MS       1000u

static const char *TAG = "task_imu";

static imu_snapshot_t g_imu_snapshot;
static portMUX_TYPE g_imu_snapshot_lock = portMUX_INITIALIZER_UNLOCKED;

static void task_imu_entry(void *argument);
static void task_imu_publish_snapshot(const imu_snapshot_t *snapshot);
static void task_imu_publish_invalid(uint32_t read_error_count);

void task_imu_start(void)
{
    BaseType_t ok = xTaskCreate(
        task_imu_entry,
        "task_imu",
        TASK_IMU_STACK_BYTES,
        NULL,
        TASK_IMU_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

void task_imu_get_snapshot(imu_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    portENTER_CRITICAL(&g_imu_snapshot_lock);
    *snapshot = g_imu_snapshot;
    portEXIT_CRITICAL(&g_imu_snapshot_lock);
}

static void task_imu_publish_snapshot(const imu_snapshot_t *snapshot)
{
    portENTER_CRITICAL(&g_imu_snapshot_lock);
    g_imu_snapshot = *snapshot;
    portEXIT_CRITICAL(&g_imu_snapshot_lock);
}

static void task_imu_publish_invalid(uint32_t read_error_count)
{
    imu_snapshot_t snapshot;

    memset(&snapshot, 0, sizeof(snapshot));

    snapshot.timestamp_us = (uint64_t)esp_timer_get_time();
    snapshot.valid = false;
    snapshot.calibrated = false;
    snapshot.read_error_count = read_error_count;

    task_imu_publish_snapshot(&snapshot);
}

static void task_imu_entry(void *argument)
{
    (void)argument;

    bsp_imu_spi_esp32_t imu_spi;
    drv_ism330dhcx_t imu;
    drv_ism330dhcx_config_t imu_config;

    est_imu_mount_t imu_mount;
    est_imu_mount_config_t mount_config;

    est_imu_calibration_t calibration;

    est_attitude_t attitude_estimator;
    est_attitude_config_t attitude_config;

    uint32_t sample_count = 0u;
    uint32_t read_error_count = 0u;
    uint64_t last_sample_time_us = 0u;

    memset(&imu_spi, 0, sizeof(imu_spi));
    memset(&imu, 0, sizeof(imu));
    memset(&imu_mount, 0, sizeof(imu_mount));

    task_imu_publish_invalid(0u);

    mount_config = est_imu_mount_default_config();
    mount_config.body_forward = APP_IMU_MOUNT_FORWARD_AXIS;
    mount_config.body_up = APP_IMU_MOUNT_UP_AXIS;

    if (!est_imu_mount_init(&imu_mount, &mount_config))
    {
        ESP_LOGE(TAG, "invalid IMU mount config");
        task_imu_publish_invalid(1u);
        vTaskDelete(NULL);
        return;
    }

    while (bsp_imu_spi_esp32_init(&imu_spi) != IF_SPI_OK)
    {
        read_error_count++;
        task_imu_publish_invalid(read_error_count);

        ESP_LOGE(TAG, "SPI init failed");
        vTaskDelay(pdMS_TO_TICKS(TASK_IMU_RETRY_MS));
    }

    imu_config = drv_ism330dhcx_default_config();
    imu_config.accel_odr = APP_IMU_ACCEL_ODR;
    imu_config.gyro_odr = APP_IMU_GYRO_ODR;
    imu_config.accel_fs = APP_IMU_ACCEL_FS;
    imu_config.gyro_fs = APP_IMU_GYRO_FS;

    while (drv_ism330dhcx_init_config(&imu,
                                      &imu_spi.spi,
                                      &imu_config) != DRV_ISM330DHCX_OK)
    {
        read_error_count++;
        task_imu_publish_invalid(read_error_count);

        ESP_LOGE(TAG, "IMU init failed");
        vTaskDelay(pdMS_TO_TICKS(TASK_IMU_RETRY_MS));
    }

    est_imu_calibration_reset(&calibration,
                              APP_IMU_CALIBRATION_SAMPLES);

    attitude_config = est_attitude_default_config();
    attitude_config.algorithm = APP_IMU_ATTITUDE_ALGORITHM;
    attitude_config.sensor_mode = APP_IMU_ATTITUDE_SENSOR_MODE;
    attitude_config.complementary_alpha = APP_IMU_COMPLEMENTARY_ALPHA;
    attitude_config.complementary_mag_alpha = APP_IMU_COMPLEMENTARY_MAG_ALPHA;

    est_attitude_init(&attitude_estimator, &attitude_config);

    ESP_LOGI(TAG, "IMU task started");
    ESP_LOGI(TAG, "keep IMU stationary for gyro bias calibration");

    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        imu_snapshot_t snapshot;
        drv_ism330dhcx_data_t sensor_data;

        memset(&snapshot, 0, sizeof(snapshot));
        memset(&sensor_data, 0, sizeof(sensor_data));

        snapshot.timestamp_us = (uint64_t)esp_timer_get_time();

        if (drv_ism330dhcx_read_raw(&imu, &snapshot.raw) == DRV_ISM330DHCX_OK)
        {
            drv_ism330dhcx_convert_raw(&imu,
                                       &snapshot.raw,
                                       &sensor_data);

            if (!est_imu_mount_apply_data(&imu_mount,
                                          &sensor_data,
                                          &snapshot.data))
            {
                read_error_count++;

                snapshot.valid = false;
                snapshot.calibrated = false;
                snapshot.sample_count = sample_count;
                snapshot.read_error_count = read_error_count;

                ESP_LOGE(TAG, "IMU mount mapping failed");
                task_imu_publish_snapshot(&snapshot);

                vTaskDelayUntil(&last_wake,
                                pdMS_TO_TICKS(APP_IMU_TASK_PERIOD_MS));
                continue;
            }

            sample_count++;

            snapshot.sample_count = sample_count;
            snapshot.read_error_count = read_error_count;

            if (!est_imu_calibration_is_complete(&calibration))
            {
                bool done = est_imu_calibration_update(&calibration,
                                                       &snapshot.data);

                snapshot.valid = false;
                snapshot.calibrated = false;

                if (done)
                {
                    last_sample_time_us = snapshot.timestamp_us;
                    est_attitude_reset(&attitude_estimator);

                    ESP_LOGI(TAG,
                             "gyro bias calibration complete: [%7.5f %7.5f %7.5f] rad/s",
                             calibration.gyro_bias_rps[0],
                             calibration.gyro_bias_rps[1],
                             calibration.gyro_bias_rps[2]);
                }
            }
            else
            {
                est_imu_calibration_apply(&calibration, &snapshot.data);

                uint64_t dt_us = snapshot.timestamp_us - last_sample_time_us;
                float dt_s = (float)dt_us * 0.000001f;

                est_attitude_input_t attitude_input;

                memset(&attitude_input, 0, sizeof(attitude_input));

                for (uint32_t i = 0u; i < 3u; i++)
                {
                    attitude_input.accel_mps2[i] = snapshot.data.accel_mps2[i];
                    attitude_input.gyro_rps[i] = snapshot.data.gyro_rps[i];
                }

                attitude_input.mag_valid = false;
                attitude_input.dt_s = dt_s;

                snapshot.attitude.valid =
                    est_attitude_update(&attitude_estimator,
                                        &attitude_input,
                                        &snapshot.attitude);

                snapshot.valid = snapshot.attitude.valid;
                snapshot.calibrated = true;

                last_sample_time_us = snapshot.timestamp_us;
            }

            for (uint32_t i = 0u; i < 3u; i++)
            {
                snapshot.gyro_bias_rps[i] = calibration.gyro_bias_rps[i];
            }
        }
        else
        {
            read_error_count++;

            snapshot.valid = false;
            snapshot.calibrated = est_imu_calibration_is_complete(&calibration);
            snapshot.sample_count = sample_count;
            snapshot.read_error_count = read_error_count;

            ESP_LOGE(TAG, "IMU read failed");
        }

        task_imu_publish_snapshot(&snapshot);

        vTaskDelayUntil(&last_wake,
                        pdMS_TO_TICKS(APP_IMU_TASK_PERIOD_MS));
    }
}