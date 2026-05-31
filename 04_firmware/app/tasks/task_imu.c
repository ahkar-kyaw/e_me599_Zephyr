#include "task_imu.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "drv_ism330dhcx.h"
#include "main.h"
#include "spi.h"

#define TASK_IMU_STACK_BYTES          2048u
#define TASK_IMU_PERIOD_MS            10u
#define TASK_IMU_RETRY_MS             1000u

static osThreadId_t s_task_imu_handle = NULL;

static StaticTask_t s_task_imu_cb;
static uint64_t s_task_imu_stack[TASK_IMU_STACK_BYTES / sizeof(uint64_t)];

static drv_ism330dhcx_t s_imu_dev;
static task_imu_snapshot_t s_snapshot;

static void task_imu_thread(void *argument);

static void task_imu_clear_snapshot(task_imu_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
}

static void task_imu_publish_init_failed(uint8_t who_am_i)
{
    taskENTER_CRITICAL();
    s_snapshot.initialized = false;
    s_snapshot.data_valid = false;
    s_snapshot.who_am_i = who_am_i;
    s_snapshot.error_count++;
    s_snapshot.last_update_ms = HAL_GetTick();
    taskEXIT_CRITICAL();
}

static void task_imu_publish_raw(const drv_ism330dhcx_raw_t *raw, uint8_t who_am_i)
{
    if (raw == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();

    s_snapshot.initialized = true;
    s_snapshot.data_valid = true;
    s_snapshot.who_am_i = who_am_i;

    s_snapshot.ax_raw = raw->ax_raw;
    s_snapshot.ay_raw = raw->ay_raw;
    s_snapshot.az_raw = raw->az_raw;

    s_snapshot.gx_raw = raw->gx_raw;
    s_snapshot.gy_raw = raw->gy_raw;
    s_snapshot.gz_raw = raw->gz_raw;

    s_snapshot.sample_count++;
    s_snapshot.last_update_ms = HAL_GetTick();

    taskEXIT_CRITICAL();
}

static void task_imu_thread(void *argument)
{
    (void)argument;

    bool imu_ready = false;
    uint8_t who_am_i = 0u;
    uint32_t last_retry_ms = 0u;

    task_imu_clear_snapshot(&s_snapshot);

    for (;;)
    {
        const uint32_t now = HAL_GetTick();

        if (!imu_ready)
        {
            if ((now - last_retry_ms) >= TASK_IMU_RETRY_MS)
            {
                last_retry_ms = now;

                who_am_i = 0u;

                drv_ism330dhcx_result_t result =
                    drv_ism330dhcx_init(
                        &s_imu_dev,
                        &hspi1,
                        IMU_CS_N_GPIO_Port,
                        IMU_CS_N_Pin);

                if (result == DRV_ISM330DHCX_OK)
                {
                    (void)drv_ism330dhcx_read_who_am_i(&s_imu_dev, &who_am_i);
                    imu_ready = true;
                }
                else
                {
                    task_imu_publish_init_failed(who_am_i);
                }
            }

            osDelay(50u);
            continue;
        }

        drv_ism330dhcx_raw_t raw;

        if (drv_ism330dhcx_read_raw(&s_imu_dev, &raw) == DRV_ISM330DHCX_OK)
        {
            task_imu_publish_raw(&raw, who_am_i);
        }
        else
        {
            taskENTER_CRITICAL();
            s_snapshot.data_valid = false;
            s_snapshot.error_count++;
            s_snapshot.last_update_ms = HAL_GetTick();
            taskEXIT_CRITICAL();

            imu_ready = false;
        }

        osDelay(TASK_IMU_PERIOD_MS);
    }
}

void task_imu_start(void)
{
    if (s_task_imu_handle != NULL)
    {
        return;
    }

    task_imu_clear_snapshot(&s_snapshot);

    static const osThreadAttr_t task_attributes =
    {
        .name = "imu",
        .attr_bits = 0u,
        .cb_mem = &s_task_imu_cb,
        .cb_size = sizeof(s_task_imu_cb),
        .stack_mem = s_task_imu_stack,
        .stack_size = sizeof(s_task_imu_stack),
        .priority = (osPriority_t)osPriorityNormal
    };

    s_task_imu_handle = osThreadNew(task_imu_thread, NULL, &task_attributes);
}

bool task_imu_get_snapshot(task_imu_snapshot_t *out_snapshot)
{
    if (out_snapshot == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();
    *out_snapshot = s_snapshot;
    taskEXIT_CRITICAL();

    return true;
}