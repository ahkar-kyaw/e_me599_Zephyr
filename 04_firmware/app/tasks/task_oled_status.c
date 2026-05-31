#include "task_oled_status.h"

#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "drv_ssd1306.h"

#define TASK_OLED_STATUS_STACK_BYTES         2048u
#define TASK_OLED_STATUS_PERIOD_MS           250u
#define TASK_OLED_RETRY_PERIOD_MS            1000u
#define TASK_OLED_I2C_TIMEOUT_MS             20u
#define TASK_OLED_USER_LINE_LEN              22u

typedef struct
{
    task_oled_status_t status;
    char user_line[TASK_OLED_USER_LINE_LEN];
} task_oled_shared_state_t;

static osThreadId_t s_oled_thread_id = NULL;
static osMutexId_t s_oled_state_mutex = NULL;

static StaticTask_t s_oled_task_cb;
static uint64_t s_oled_task_stack[TASK_OLED_STATUS_STACK_BYTES / sizeof(uint64_t)];

static StaticSemaphore_t s_oled_mutex_cb;

static I2C_HandleTypeDef *s_oled_i2c = NULL;
static drv_ssd1306_t s_oled;
static task_oled_shared_state_t s_shared;

static void task_oled_status_thread(void *argument);

static void task_oled_status_default_snapshot(task_oled_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    memset(status, 0, sizeof(*status));

    status->imu_valid = false;
    status->crsf_link_valid = false;
    status->can_motor_valid = false;
    status->battery_low = false;
    status->estop_active = false;
    status->armed = false;
    status->battery_mv = 0u;
    status->pitch_cdeg = 0;
    status->roll_cdeg = 0;
    status->safety_fault_flags = 0u;
    status->heartbeat = 0u;
}

static bool task_oled_lock(uint32_t timeout_ms)
{
    if (s_oled_state_mutex == NULL)
    {
        return false;
    }

    return (osMutexAcquire(s_oled_state_mutex, timeout_ms) == osOK);
}

static void task_oled_unlock(void)
{
    if (s_oled_state_mutex != NULL)
    {
        (void)osMutexRelease(s_oled_state_mutex);
    }
}

static void task_oled_copy_snapshot(task_oled_shared_state_t *out)
{
    if (out == NULL)
    {
        return;
    }

    if (task_oled_lock(5u))
    {
        *out = s_shared;
        task_oled_unlock();
    }
    else
    {
        memset(out, 0, sizeof(*out));
        task_oled_status_default_snapshot(&out->status);
        strncpy(out->user_line, "OLED STATE BUSY", sizeof(out->user_line) - 1u);
        out->user_line[sizeof(out->user_line) - 1u] = '\0';
    }
}

static void task_oled_format_voltage(char *out, size_t out_len, uint16_t mv)
{
    if ((out == NULL) || (out_len == 0u))
    {
        return;
    }

    const uint16_t volts = (uint16_t)(mv / 1000u);
    const uint16_t centivolts = (uint16_t)((mv % 1000u) / 10u);

    (void)snprintf(out, out_len, "%u.%02uV", volts, centivolts);
}

static void task_oled_format_cdeg(char *out, size_t out_len, int16_t cdeg)
{
    if ((out == NULL) || (out_len == 0u))
    {
        return;
    }

    int32_t value = cdeg;
    char sign = '+';

    if (value < 0)
    {
        sign = '-';
        value = -value;
    }

    const int32_t deg = value / 100;
    const int32_t frac = value % 100;

    (void)snprintf(out, out_len, "%c%ld.%02ld", sign, (long)deg, (long)frac);
}

static const char *task_oled_ok_fail(bool value)
{
    return value ? "OK" : "FAIL";
}

static const char *task_oled_yes_no(bool value)
{
    return value ? "YES" : "NO";
}

static bool task_oled_init_hardware(void)
{
    drv_ssd1306_config_t cfg;

    if (s_oled_i2c == NULL)
    {
        return false;
    }

    drv_ssd1306_default_config(&cfg, s_oled_i2c);

    cfg.i2c_addr_7bit = DRV_SSD1306_I2C_ADDR_3C;
    cfg.height = DRV_SSD1306_HEIGHT_64;
    cfg.contrast = 0x7Fu;
    cfg.external_vcc = false;
    cfg.rotate_180 = false;
    cfg.i2c_timeout_ms = TASK_OLED_I2C_TIMEOUT_MS;

    if (drv_ssd1306_init(&s_oled, &cfg) == DRV_SSD1306_OK)
    {
        return true;
    }

    cfg.i2c_addr_7bit = DRV_SSD1306_I2C_ADDR_3D;

    if (drv_ssd1306_init(&s_oled, &cfg) == DRV_SSD1306_OK)
    {
        return true;
    }

    return false;
}

static void task_oled_draw_status(const task_oled_shared_state_t *snapshot)
{
    char line[24];
    char voltage[12];
    char pitch[12];
    char roll[12];

    if (snapshot == NULL)
    {
        return;
    }

    task_oled_format_voltage(voltage, sizeof(voltage), snapshot->status.battery_mv);
    task_oled_format_cdeg(pitch, sizeof(pitch), snapshot->status.pitch_cdeg);
    task_oled_format_cdeg(roll, sizeof(roll), snapshot->status.roll_cdeg);

    (void)drv_ssd1306_clear(&s_oled, DRV_SSD1306_COLOR_BLACK);

    if (snapshot->status.armed)
    {
        (void)drv_ssd1306_draw_text_line(&s_oled, 0u, "ZEPHYR ARMED", DRV_SSD1306_COLOR_WHITE);
    }
    else
    {
        (void)drv_ssd1306_draw_text_line(&s_oled, 0u, "ZEPHYR SAFE", DRV_SSD1306_COLOR_WHITE);
    }

    (void)snprintf(
        line,
        sizeof(line),
        "IMU %s CRSF %s",
        task_oled_ok_fail(snapshot->status.imu_valid),
        task_oled_ok_fail(snapshot->status.crsf_link_valid));
    (void)drv_ssd1306_draw_text_line(&s_oled, 1u, line, DRV_SSD1306_COLOR_WHITE);

    (void)snprintf(
        line,
        sizeof(line),
        "CAN %s VB %s",
        task_oled_ok_fail(snapshot->status.can_motor_valid),
        voltage);
    (void)drv_ssd1306_draw_text_line(&s_oled, 2u, line, DRV_SSD1306_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "PITCH %s", pitch);
    (void)drv_ssd1306_draw_text_line(&s_oled, 3u, line, DRV_SSD1306_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "ROLL  %s", roll);
    (void)drv_ssd1306_draw_text_line(&s_oled, 4u, line, DRV_SSD1306_COLOR_WHITE);

    (void)snprintf(
        line,
        sizeof(line),
        "ESTOP %s LOW %s",
        task_oled_yes_no(snapshot->status.estop_active),
        task_oled_yes_no(snapshot->status.battery_low));
    (void)drv_ssd1306_draw_text_line(&s_oled, 5u, line, DRV_SSD1306_COLOR_WHITE);

    (void)snprintf(
        line,
        sizeof(line),
        "FLT 0x%08lX",
        (unsigned long)snapshot->status.safety_fault_flags);
    (void)drv_ssd1306_draw_text_line(&s_oled, 6u, line, DRV_SSD1306_COLOR_WHITE);

    if (snapshot->user_line[0] != '\0')
    {
        (void)drv_ssd1306_draw_text_line(&s_oled, 7u, snapshot->user_line, DRV_SSD1306_COLOR_WHITE);
    }
    else
    {
        (void)snprintf(
            line,
            sizeof(line),
            "HB %lu",
            (unsigned long)snapshot->status.heartbeat);
        (void)drv_ssd1306_draw_text_line(&s_oled, 7u, line, DRV_SSD1306_COLOR_WHITE);
    }
}

static void task_oled_status_thread(void *argument)
{
    (void)argument;

    bool oled_ready = false;
    uint32_t retry_tick = 0u;

    for (;;)
    {
        const uint32_t now = osKernelGetTickCount();

        if (!oled_ready)
        {
            if ((now - retry_tick) >= TASK_OLED_RETRY_PERIOD_MS)
            {
                retry_tick = now;
                oled_ready = task_oled_init_hardware();
            }

            osDelay(TASK_OLED_STATUS_PERIOD_MS);
            continue;
        }

        task_oled_shared_state_t snapshot;
        task_oled_copy_snapshot(&snapshot);

        task_oled_draw_status(&snapshot);

        const drv_ssd1306_result_t result = drv_ssd1306_update(&s_oled);

        if (result != DRV_SSD1306_OK)
        {
            oled_ready = false;
        }

        osDelay(TASK_OLED_STATUS_PERIOD_MS);
    }
}

bool task_oled_status_start(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
    {
        return false;
    }

    if (s_oled_thread_id != NULL)
    {
        return true;
    }

    s_oled_i2c = hi2c;

    task_oled_status_default_snapshot(&s_shared.status);
    s_shared.user_line[0] = '\0';

    static const osMutexAttr_t mutex_attr =
    {
        .name = "oled_state_mutex",
        .attr_bits = osMutexPrioInherit,
        .cb_mem = &s_oled_mutex_cb,
        .cb_size = sizeof(s_oled_mutex_cb)
    };

    s_oled_state_mutex = osMutexNew(&mutex_attr);

    if (s_oled_state_mutex == NULL)
    {
        return false;
    }

    static const osThreadAttr_t thread_attr =
    {
        .name = "oled_status",
        .attr_bits = 0u,
        .cb_mem = &s_oled_task_cb,
        .cb_size = sizeof(s_oled_task_cb),
        .stack_mem = s_oled_task_stack,
        .stack_size = sizeof(s_oled_task_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_oled_thread_id = osThreadNew(task_oled_status_thread, NULL, &thread_attr);

    return (s_oled_thread_id != NULL);
}

bool task_oled_status_publish(const task_oled_status_t *status)
{
    if (status == NULL)
    {
        return false;
    }

    if (!task_oled_lock(0u))
    {
        return false;
    }

    s_shared.status = *status;

    task_oled_unlock();

    return true;
}

bool task_oled_status_set_user_line(const char *text)
{
    if (text == NULL)
    {
        return false;
    }

    if (!task_oled_lock(0u))
    {
        return false;
    }

    strncpy(s_shared.user_line, text, sizeof(s_shared.user_line) - 1u);
    s_shared.user_line[sizeof(s_shared.user_line) - 1u] = '\0';

    task_oled_unlock();

    return true;
}