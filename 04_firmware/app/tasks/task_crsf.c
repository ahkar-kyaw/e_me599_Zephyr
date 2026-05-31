#include "task_crsf.h"

#include <string.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

#include "dma_buffers.h"
#include "drv_crsf.h"
#include "usart.h"

#define TASK_CRSF_STACK_BYTES        2048u
#define TASK_CRSF_PERIOD_MS          2u

#define TASK_CRSF_RAW_MIN            172u
#define TASK_CRSF_RAW_CENTER         992u
#define TASK_CRSF_RAW_MAX            1811u

#define TASK_CRSF_US_MIN             988u
#define TASK_CRSF_US_CENTER          1500u
#define TASK_CRSF_US_MAX             2012u

static osThreadId_t s_task_crsf_handle = NULL;

static StaticTask_t s_task_crsf_cb;
static uint64_t s_task_crsf_stack[TASK_CRSF_STACK_BYTES / sizeof(uint64_t)];

static task_crsf_snapshot_t s_snapshot;
static uint32_t s_sequence = 0u;

static void task_crsf_thread(void *argument);

static uint16_t task_crsf_raw_to_us(uint16_t raw)
{
    if (raw <= TASK_CRSF_RAW_MIN)
    {
        return TASK_CRSF_US_MIN;
    }

    if (raw >= TASK_CRSF_RAW_MAX)
    {
        return TASK_CRSF_US_MAX;
    }

    const uint32_t num =
        ((uint32_t)(raw - TASK_CRSF_RAW_MIN)) *
        ((uint32_t)(TASK_CRSF_US_MAX - TASK_CRSF_US_MIN));

    const uint32_t den =
        (uint32_t)(TASK_CRSF_RAW_MAX - TASK_CRSF_RAW_MIN);

    return (uint16_t)(TASK_CRSF_US_MIN + (num / den));
}

static int16_t task_crsf_raw_to_permille(uint16_t raw)
{
    if (raw >= TASK_CRSF_RAW_CENTER)
    {
        if (raw >= TASK_CRSF_RAW_MAX)
        {
            return 1000;
        }

        const uint32_t num =
            ((uint32_t)(raw - TASK_CRSF_RAW_CENTER)) * 1000u;

        const uint32_t den =
            (uint32_t)(TASK_CRSF_RAW_MAX - TASK_CRSF_RAW_CENTER);

        return (int16_t)(num / den);
    }

    if (raw <= TASK_CRSF_RAW_MIN)
    {
        return -1000;
    }

    const uint32_t num =
        ((uint32_t)(TASK_CRSF_RAW_CENTER - raw)) * 1000u;

    const uint32_t den =
        (uint32_t)(TASK_CRSF_RAW_CENTER - TASK_CRSF_RAW_MIN);

    return (int16_t)(-((int32_t)(num / den)));
}

static void task_crsf_clear_snapshot(task_crsf_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));

    for (uint8_t i = 0u; i < TASK_CRSF_CHANNEL_COUNT; i++)
    {
        snapshot->raw[i] = TASK_CRSF_RAW_CENTER;
        snapshot->us[i] = TASK_CRSF_US_CENTER;
        snapshot->norm_permille[i] = 0;
    }
}

static void task_crsf_publish_from_driver_state(const drv_crsf_state_t *state)
{
    task_crsf_snapshot_t next;

    if (state == NULL)
    {
        return;
    }

    task_crsf_clear_snapshot(&next);

    next.task_running = true;
    next.driver_started = true;
    next.receiver_connected = state->receiver_connected;
    next.channels_valid = state->receiver_connected && state->channels.valid;

    next.last_update_tick_ms = HAL_GetTick();

    if (state->last_rc_frame_tick_ms <= next.last_update_tick_ms)
    {
        next.rc_age_ms = next.last_update_tick_ms - state->last_rc_frame_tick_ms;
    }
    else
    {
        next.rc_age_ms = 0u;
    }

    next.valid_frame_count = state->valid_frame_count;
    next.valid_rc_frame_count = state->valid_rc_frame_count;
    next.crc_error_count = state->crc_error_count;
    next.length_error_count = state->length_error_count;
    next.uart_error_count = state->uart_error_count;
    next.dma_restart_count = state->dma_restart_count;

    for (uint8_t i = 0u; i < TASK_CRSF_CHANNEL_COUNT; i++)
    {
        const uint16_t raw = state->channels.raw[i];

        next.raw[i] = raw;
        next.us[i] = task_crsf_raw_to_us(raw);
        next.norm_permille[i] = task_crsf_raw_to_permille(raw);
    }

    taskENTER_CRITICAL();
    s_sequence++;
    next.sequence = s_sequence;
    s_snapshot = next;
    taskEXIT_CRITICAL();
}

static void task_crsf_publish_driver_not_started(void)
{
    task_crsf_snapshot_t next;

    task_crsf_clear_snapshot(&next);

    next.task_running = true;
    next.driver_started = false;
    next.receiver_connected = false;
    next.channels_valid = false;
    next.last_update_tick_ms = HAL_GetTick();

    taskENTER_CRITICAL();
    s_sequence++;
    next.sequence = s_sequence;
    s_snapshot = next;
    taskEXIT_CRITICAL();
}

static void task_crsf_thread(void *argument)
{
    (void)argument;

    task_crsf_clear_snapshot(&s_snapshot);

    bool driver_ok = drv_crsf_init(&huart2, crsf_rx_dma_buf, CRSF_RX_DMA_BUF_LEN);

    if (driver_ok)
    {
        driver_ok = drv_crsf_start();
    }

    if (!driver_ok)
    {
        task_crsf_publish_driver_not_started();
    }

    for (;;)
    {
        if (driver_ok)
        {
            drv_crsf_process();

            drv_crsf_state_t state;
            drv_crsf_get_state(&state);

            task_crsf_publish_from_driver_state(&state);
        }
        else
        {
            task_crsf_publish_driver_not_started();
        }

        osDelay(TASK_CRSF_PERIOD_MS);
    }
}

bool task_crsf_start(void)
{
    if (s_task_crsf_handle != NULL)
    {
        return true;
    }

    task_crsf_clear_snapshot(&s_snapshot);

    static const osThreadAttr_t task_attributes =
    {
        .name = "task_crsf",
        .attr_bits = 0u,
        .cb_mem = &s_task_crsf_cb,
        .cb_size = sizeof(s_task_crsf_cb),
        .stack_mem = s_task_crsf_stack,
        .stack_size = sizeof(s_task_crsf_stack),
        .priority = (osPriority_t)osPriorityNormal
    };

    s_task_crsf_handle = osThreadNew(task_crsf_thread, NULL, &task_attributes);

    return (s_task_crsf_handle != NULL);
}

bool task_crsf_get_snapshot(task_crsf_snapshot_t *out_snapshot)
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

bool task_crsf_is_connected(void)
{
    task_crsf_snapshot_t snapshot;

    if (!task_crsf_get_snapshot(&snapshot))
    {
        return false;
    }

    return snapshot.receiver_connected && snapshot.channels_valid;
}

bool task_crsf_get_channel_raw(uint8_t channel_index, uint16_t *out_raw)
{
    task_crsf_snapshot_t snapshot;

    if ((channel_index >= TASK_CRSF_CHANNEL_COUNT) || (out_raw == NULL))
    {
        return false;
    }

    if (!task_crsf_get_snapshot(&snapshot))
    {
        return false;
    }

    if (!snapshot.receiver_connected || !snapshot.channels_valid)
    {
        return false;
    }

    *out_raw = snapshot.raw[channel_index];

    return true;
}

bool task_crsf_get_channel_us(uint8_t channel_index, uint16_t *out_us)
{
    task_crsf_snapshot_t snapshot;

    if ((channel_index >= TASK_CRSF_CHANNEL_COUNT) || (out_us == NULL))
    {
        return false;
    }

    if (!task_crsf_get_snapshot(&snapshot))
    {
        return false;
    }

    if (!snapshot.receiver_connected || !snapshot.channels_valid)
    {
        return false;
    }

    *out_us = snapshot.us[channel_index];

    return true;
}

bool task_crsf_get_channel_norm_permille(uint8_t channel_index, int16_t *out_norm)
{
    task_crsf_snapshot_t snapshot;

    if ((channel_index >= TASK_CRSF_CHANNEL_COUNT) || (out_norm == NULL))
    {
        return false;
    }

    if (!task_crsf_get_snapshot(&snapshot))
    {
        return false;
    }

    if (!snapshot.receiver_connected || !snapshot.channels_valid)
    {
        return false;
    }

    *out_norm = snapshot.norm_permille[channel_index];

    return true;
}