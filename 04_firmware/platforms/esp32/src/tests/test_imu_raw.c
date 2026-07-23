#include "test_imu_raw.h"

#include "bsp_imu_spi_esp32.h"
#include "drivers/drv_ism330dhcx.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>

#define TEST_IMU_RAW_STACK_BYTES       4096u
#define TEST_IMU_RAW_PRIORITY          (tskIDLE_PRIORITY + 2)
#define TEST_IMU_RAW_PERIOD_MS         100u

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
        ESP_LOGE(TAG, "failed to create task");
    }
}

static void test_imu_raw_entry(void *argument)
{
    (void)argument;

    bsp_imu_spi_esp32_t imu_spi;
    drv_ism330dhcx_t imu;

    ESP_LOGI(TAG, "starting");

    if (bsp_imu_spi_esp32_init(&imu_spi) != IF_SPI_OK)
    {
        ESP_LOGE(TAG, "SPI init failed");
        vTaskDelete(NULL);
        return;
    }

    uint8_t who_am_i = 0;

    imu.spi = &imu_spi.spi;

    if (drv_ism330dhcx_read_who_am_i(&imu, &who_am_i) != DRV_ISM330DHCX_OK)
    {
        ESP_LOGE(TAG, "WHO_AM_I read failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "WHO_AM_I=0x%02X", who_am_i);

    if (drv_ism330dhcx_init(&imu, &imu_spi.spi) != DRV_ISM330DHCX_OK)
    {
        ESP_LOGE(TAG, "IMU init failed");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "IMU init OK");

    for (;;)
    {
        drv_ism330dhcx_raw_t raw;

        if (drv_ism330dhcx_read_raw(&imu, &raw) == DRV_ISM330DHCX_OK)
        {
            ESP_LOGI(TAG,
                     "accel_raw x=%6d y=%6d z=%6d | gyro_raw x=%6d y=%6d z=%6d | temp_raw=%6d",
                     raw.accel_raw[0],
                     raw.accel_raw[1],
                     raw.accel_raw[2],
                     raw.gyro_raw[0],
                     raw.gyro_raw[1],
                     raw.gyro_raw[2],
                     raw.temp_raw);
        }
        else
        {
            ESP_LOGE(TAG, "read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(TEST_IMU_RAW_PERIOD_MS));
    }
}