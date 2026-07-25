#include "test_imu_snapshot.h"

#include "task_imu.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

#define TEST_IMU_SNAPSHOT_STACK_BYTES    4096u
#define TEST_IMU_SNAPSHOT_PRIORITY       (tskIDLE_PRIORITY + 1)
#define TEST_IMU_SNAPSHOT_PERIOD_MS      100u

#define TEST_IMU_RAD_TO_DEG              57.2957795f

static void test_imu_snapshot_entry(void *argument);

void test_imu_snapshot_start(void)
{
    xTaskCreate(
        test_imu_snapshot_entry,
        "test_imu_snapshot",
        TEST_IMU_SNAPSHOT_STACK_BYTES,
        NULL,
        TEST_IMU_SNAPSHOT_PRIORITY,
        NULL);
}

static void test_imu_snapshot_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        imu_snapshot_t snapshot;

        task_imu_get_snapshot(&snapshot);

        if (snapshot.valid && snapshot.attitude.valid)
        {
            printf("%7.2f %7.2f %7.2f\n",
                   snapshot.attitude.roll_rad * TEST_IMU_RAD_TO_DEG,
                   snapshot.attitude.pitch_rad * TEST_IMU_RAD_TO_DEG,
                   snapshot.attitude.yaw_rad * TEST_IMU_RAD_TO_DEG);

            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(TEST_IMU_SNAPSHOT_PERIOD_MS));
    }
}