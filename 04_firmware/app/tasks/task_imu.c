#include "task_imu.h"

#include <math.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "drv_ism330dhcx.h"
#include "main.h"
#include "spi.h"

#define TASK_IMU_STACK_BYTES              2048u
#define TASK_IMU_PERIOD_MS                10u
#define TASK_IMU_RETRY_MS                 1000u

#define TASK_IMU_ACCEL_MG_PER_LSB_NUM     122
#define TASK_IMU_ACCEL_MG_PER_LSB_DEN     1000

#define TASK_IMU_GYRO_MDPS_PER_LSB_NUM    875
#define TASK_IMU_GYRO_MDPS_PER_LSB_DEN    100

#define TASK_IMU_COMPLEMENTARY_ALPHA      0.98f
#define TASK_IMU_RAD_TO_DEG               57.2957795f

#define TASK_IMU_CALIBRATION_SAMPLES      200u

static osThreadId_t s_task_imu_handle = NULL;

static StaticTask_t s_task_imu_cb;
static uint64_t s_task_imu_stack[TASK_IMU_STACK_BYTES / sizeof(uint64_t)];

static drv_ism330dhcx_t s_imu_dev;
static task_imu_snapshot_t s_snapshot;

static int64_t s_gx_bias_sum = 0;
static int64_t s_gy_bias_sum = 0;
static int64_t s_gz_bias_sum = 0;

static float s_pitch_deg = 0.0f;
static float s_roll_deg = 0.0f;
static bool s_attitude_initialized = false;

static uint32_t s_last_filter_tick_ms = 0u;

static void task_imu_thread(void *argument);

static void task_imu_clear_snapshot(task_imu_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->state = TASK_IMU_STATE_INIT;
}

static int32_t task_imu_raw_accel_to_mg(int16_t raw)
{
    return ((int32_t)raw * TASK_IMU_ACCEL_MG_PER_LSB_NUM) /
           TASK_IMU_ACCEL_MG_PER_LSB_DEN;
}

static int32_t task_imu_raw_gyro_to_mdps(int32_t raw)
{
    return (raw * TASK_IMU_GYRO_MDPS_PER_LSB_NUM) /
           TASK_IMU_GYRO_MDPS_PER_LSB_DEN;
}

static int16_t task_imu_float_deg_to_cdeg(float value_deg)
{
    float value_cdeg = value_deg * 100.0f;

    if (value_cdeg > 32767.0f)
    {
        value_cdeg = 32767.0f;
    }
    else if (value_cdeg < -32768.0f)
    {
        value_cdeg = -32768.0f;
    }
    else
    {
        /* In range. */
    }

    return (int16_t)value_cdeg;
}

static void task_imu_publish_init_result(
    drv_ism330dhcx_result_t result,
    uint8_t who_am_i)
{
    taskENTER_CRITICAL();

    s_snapshot.initialized = false;
    s_snapshot.data_valid = false;
    s_snapshot.gyro_calibrated = false;
    s_snapshot.state = TASK_IMU_STATE_ERROR;
    s_snapshot.who_am_i = who_am_i;
    s_snapshot.init_result = (uint8_t)result;
    s_snapshot.last_read_result = 0u;
    s_snapshot.error_count++;
    s_snapshot.last_update_ms = HAL_GetTick();

    taskEXIT_CRITICAL();
}

static void task_imu_publish_calibration_progress(
    const drv_ism330dhcx_raw_t *raw,
    uint8_t who_am_i,
    uint32_t calibration_count)
{
    if (raw == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();

    s_snapshot.initialized = true;
    s_snapshot.data_valid = true;
    s_snapshot.gyro_calibrated = false;
    s_snapshot.state = TASK_IMU_STATE_CALIBRATING;
    s_snapshot.who_am_i = who_am_i;
    s_snapshot.init_result = (uint8_t)DRV_ISM330DHCX_OK;
    s_snapshot.last_read_result = (uint8_t)DRV_ISM330DHCX_OK;

    s_snapshot.ax_raw = raw->ax_raw;
    s_snapshot.ay_raw = raw->ay_raw;
    s_snapshot.az_raw = raw->az_raw;

    s_snapshot.gx_raw = raw->gx_raw;
    s_snapshot.gy_raw = raw->gy_raw;
    s_snapshot.gz_raw = raw->gz_raw;

    s_snapshot.ax_mg = task_imu_raw_accel_to_mg(raw->ax_raw);
    s_snapshot.ay_mg = task_imu_raw_accel_to_mg(raw->ay_raw);
    s_snapshot.az_mg = task_imu_raw_accel_to_mg(raw->az_raw);

    s_snapshot.gx_mdps = task_imu_raw_gyro_to_mdps(raw->gx_raw);
    s_snapshot.gy_mdps = task_imu_raw_gyro_to_mdps(raw->gy_raw);
    s_snapshot.gz_mdps = task_imu_raw_gyro_to_mdps(raw->gz_raw);

    s_snapshot.calibration_count = calibration_count;
    s_snapshot.calibration_target = TASK_IMU_CALIBRATION_SAMPLES;
    s_snapshot.last_update_ms = HAL_GetTick();

    taskEXIT_CRITICAL();
}

static void task_imu_publish_read_failed(drv_ism330dhcx_result_t result)
{
    taskENTER_CRITICAL();

    s_snapshot.data_valid = false;
    s_snapshot.state = TASK_IMU_STATE_ERROR;
    s_snapshot.last_read_result = (uint8_t)result;
    s_snapshot.error_count++;
    s_snapshot.last_update_ms = HAL_GetTick();

    taskEXIT_CRITICAL();
}

static void task_imu_update_attitude(
    const drv_ism330dhcx_raw_t *raw,
    int32_t gx_corr_raw,
    int32_t gy_corr_raw)
{
    if (raw == NULL)
    {
        return;
    }

    const uint32_t now_ms = HAL_GetTick();
    float dt_s = 0.0f;

    if (s_last_filter_tick_ms == 0u)
    {
        dt_s = (float)TASK_IMU_PERIOD_MS / 1000.0f;
    }
    else
    {
        dt_s = (float)(now_ms - s_last_filter_tick_ms) / 1000.0f;

        if ((dt_s <= 0.0f) || (dt_s > 0.1f))
        {
            dt_s = (float)TASK_IMU_PERIOD_MS / 1000.0f;
        }
    }

    s_last_filter_tick_ms = now_ms;

    const float ax_g = (float)task_imu_raw_accel_to_mg(raw->ax_raw) / 1000.0f;
    const float ay_g = (float)task_imu_raw_accel_to_mg(raw->ay_raw) / 1000.0f;
    const float az_g = (float)task_imu_raw_accel_to_mg(raw->az_raw) / 1000.0f;

    const float gx_dps = (float)task_imu_raw_gyro_to_mdps(gx_corr_raw) / 1000.0f;
    const float gy_dps = (float)task_imu_raw_gyro_to_mdps(gy_corr_raw) / 1000.0f;

    const float roll_acc_deg = atan2f(ay_g, az_g) * TASK_IMU_RAD_TO_DEG;
    const float pitch_acc_deg =
        atan2f(-ax_g, sqrtf((ay_g * ay_g) + (az_g * az_g))) * TASK_IMU_RAD_TO_DEG;

    if (!s_attitude_initialized)
    {
        s_roll_deg = roll_acc_deg;
        s_pitch_deg = pitch_acc_deg;
        s_attitude_initialized = true;
        return;
    }

    /*
     * Axis signs may need to be adjusted after you mount the IMU on the robot.
     * For now:
     *   roll integrates gyro X
     *   pitch integrates gyro Y
     */
    const float roll_gyro_deg = s_roll_deg + (gx_dps * dt_s);
    const float pitch_gyro_deg = s_pitch_deg + (gy_dps * dt_s);

    s_roll_deg =
        (TASK_IMU_COMPLEMENTARY_ALPHA * roll_gyro_deg) +
        ((1.0f - TASK_IMU_COMPLEMENTARY_ALPHA) * roll_acc_deg);

    s_pitch_deg =
        (TASK_IMU_COMPLEMENTARY_ALPHA * pitch_gyro_deg) +
        ((1.0f - TASK_IMU_COMPLEMENTARY_ALPHA) * pitch_acc_deg);
}

static void task_imu_publish_running(
    const drv_ism330dhcx_raw_t *raw,
    uint8_t who_am_i,
    int32_t gx_corr_raw,
    int32_t gy_corr_raw,
    int32_t gz_corr_raw)
{
    if (raw == NULL)
    {
        return;
    }

    taskENTER_CRITICAL();

    s_snapshot.initialized = true;
    s_snapshot.data_valid = true;
    s_snapshot.gyro_calibrated = true;
    s_snapshot.state = TASK_IMU_STATE_RUNNING;
    s_snapshot.who_am_i = who_am_i;
    s_snapshot.init_result = (uint8_t)DRV_ISM330DHCX_OK;
    s_snapshot.last_read_result = (uint8_t)DRV_ISM330DHCX_OK;

    s_snapshot.ax_raw = raw->ax_raw;
    s_snapshot.ay_raw = raw->ay_raw;
    s_snapshot.az_raw = raw->az_raw;

    s_snapshot.gx_raw = raw->gx_raw;
    s_snapshot.gy_raw = raw->gy_raw;
    s_snapshot.gz_raw = raw->gz_raw;

    s_snapshot.ax_mg = task_imu_raw_accel_to_mg(raw->ax_raw);
    s_snapshot.ay_mg = task_imu_raw_accel_to_mg(raw->ay_raw);
    s_snapshot.az_mg = task_imu_raw_accel_to_mg(raw->az_raw);

    s_snapshot.gx_mdps = task_imu_raw_gyro_to_mdps(gx_corr_raw);
    s_snapshot.gy_mdps = task_imu_raw_gyro_to_mdps(gy_corr_raw);
    s_snapshot.gz_mdps = task_imu_raw_gyro_to_mdps(gz_corr_raw);

    s_snapshot.pitch_cdeg = task_imu_float_deg_to_cdeg(s_pitch_deg);
    s_snapshot.roll_cdeg = task_imu_float_deg_to_cdeg(s_roll_deg);

    s_snapshot.sample_count++;
    s_snapshot.last_update_ms = HAL_GetTick();

    taskEXIT_CRITICAL();
}

static void task_imu_thread(void *argument)
{
    (void)argument;

    bool imu_ready = false;
    bool calibration_done = false;

    uint8_t who_am_i = 0u;
    uint32_t last_retry_ms = HAL_GetTick() - TASK_IMU_RETRY_MS;
    uint32_t calibration_count = 0u;

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

                (void)drv_ism330dhcx_read_who_am_i(&s_imu_dev, &who_am_i);

                if (result == DRV_ISM330DHCX_OK)
                {
                    imu_ready = true;
                    calibration_done = false;
                    calibration_count = 0u;
                    s_gx_bias_sum = 0;
                    s_gy_bias_sum = 0;
                    s_gz_bias_sum = 0;
                    s_attitude_initialized = false;
                    s_last_filter_tick_ms = 0u;
                }
                else
                {
                    task_imu_publish_init_result(result, who_am_i);
                }
            }

            osDelay(50u);
            continue;
        }

        drv_ism330dhcx_raw_t raw;

        drv_ism330dhcx_result_t result =
            drv_ism330dhcx_read_raw(&s_imu_dev, &raw);

        if (result != DRV_ISM330DHCX_OK)
        {
            task_imu_publish_read_failed(result);
            imu_ready = false;
            osDelay(TASK_IMU_PERIOD_MS);
            continue;
        }

        if (!calibration_done)
        {
            s_gx_bias_sum += raw.gx_raw;
            s_gy_bias_sum += raw.gy_raw;
            s_gz_bias_sum += raw.gz_raw;
            calibration_count++;

            task_imu_publish_calibration_progress(&raw, who_am_i, calibration_count);

            if (calibration_count >= TASK_IMU_CALIBRATION_SAMPLES)
            {
                taskENTER_CRITICAL();
                s_snapshot.gx_bias_raw =
                    (int32_t)(s_gx_bias_sum / (int64_t)TASK_IMU_CALIBRATION_SAMPLES);
                s_snapshot.gy_bias_raw =
                    (int32_t)(s_gy_bias_sum / (int64_t)TASK_IMU_CALIBRATION_SAMPLES);
                s_snapshot.gz_bias_raw =
                    (int32_t)(s_gz_bias_sum / (int64_t)TASK_IMU_CALIBRATION_SAMPLES);
                taskEXIT_CRITICAL();

                calibration_done = true;
            }

            osDelay(TASK_IMU_PERIOD_MS);
            continue;
        }

        task_imu_snapshot_t current;

        taskENTER_CRITICAL();
        current = s_snapshot;
        taskEXIT_CRITICAL();

        const int32_t gx_corr_raw = (int32_t)raw.gx_raw - current.gx_bias_raw;
        const int32_t gy_corr_raw = (int32_t)raw.gy_raw - current.gy_bias_raw;
        const int32_t gz_corr_raw = (int32_t)raw.gz_raw - current.gz_bias_raw;

        task_imu_update_attitude(&raw, gx_corr_raw, gy_corr_raw);
        task_imu_publish_running(&raw, who_am_i, gx_corr_raw, gy_corr_raw, gz_corr_raw);

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