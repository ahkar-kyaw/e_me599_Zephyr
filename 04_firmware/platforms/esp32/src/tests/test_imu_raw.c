#include "test_imu_raw.h"

#include "drv_ism330dhcx.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>

#define TEST_IMU_RAW_STACK_BYTES       4096u
#define TEST_IMU_RAW_PRIORITY          (tskIDLE_PRIORITY + 2)
#define TEST_IMU_RAW_PRINT_PERIOD_MS   100u

static const char *TAG = "test_imu_raw";

static void test_imu_raw_entry(void *argument);

void test_imu_raw_start(void)
{
    BaseType_t ok = xTaskCreate(
        test_imu_raw_entry,
        "test_imu_raw",
        TEST_IMU_RAW_STACK_BYTES,
        NULL,
        TEST_IMU_RAW_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create test_imu_raw task");
    }
}

static void test_imu_raw_entry(void *argument)
{
    (void)argument;

    drv_ism330dhcx_t imu = {0};
    drv_ism330dhcx_config_t config = drv_ism330dhcx_default_config();

    ESP_LOGI(TAG, "starting ISM330DHCX raw data test");

    esp_err_t err = drv_ism330dhcx_bus_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    err = drv_ism330dhcx_add_device(&imu);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }

    uint8_t who_am_i = 0u;

    err = drv_ism330dhcx_read_who_am_i(&imu, &who_am_i);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "WHO_AM_I read failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "check SCLK, MISO, MOSI, CS, 3.3 V, and GND wiring");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "WHO_AM_I=0x%02X", who_am_i);

    err = drv_ism330dhcx_init(&imu, &config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ISM330DHCX init failed: %s", esp_err_to_name(err));

        if (who_am_i != DRV_ISM330DHCX_WHO_AM_I_EXPECTED)
        {
            ESP_LOGE(TAG,
                     "expected WHO_AM_I=0x%02X but got 0x%02X",
                     DRV_ISM330DHCX_WHO_AM_I_EXPECTED,
                     who_am_i);
        }

        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "raw IMU printing started");

    for (;;)
    {
        drv_ism330dhcx_sample_t sample = {0};

        err = drv_ism330dhcx_read_sample(&imu, &sample);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "sample read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        ESP_LOGI(TAG,
                 "status=0x%02X ready[a=%d g=%d t=%d]",
                 sample.status_reg,
                 sample.accel_data_ready ? 1 : 0,
                 sample.gyro_data_ready ? 1 : 0,
                 sample.temp_data_ready ? 1 : 0);

        ESP_LOGI(TAG,
                 "accel_raw[x=%6d y=%6d z=%6d] accel_mps2[x=%7.3f y=%7.3f z=%7.3f]",
                 sample.accel_raw[0],
                 sample.accel_raw[1],
                 sample.accel_raw[2],
                 sample.accel_mps2[0],
                 sample.accel_mps2[1],
                 sample.accel_mps2[2]);

        ESP_LOGI(TAG,
                 "gyro_raw [x=%6d y=%6d z=%6d] gyro_dps  [x=%7.3f y=%7.3f z=%7.3f] temp=%6.2f C",
                 sample.gyro_raw[0],
                 sample.gyro_raw[1],
                 sample.gyro_raw[2],
                 sample.gyro_dps[0],
                 sample.gyro_dps[1],
                 sample.gyro_dps[2],
                 sample.temp_c);

        vTaskDelay(pdMS_TO_TICKS(TEST_IMU_RAW_PRINT_PERIOD_MS));
    }
}