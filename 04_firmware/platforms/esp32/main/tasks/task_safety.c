#include "task_safety.h"

#include "config_actuator.h"
#include "config_rc.h"
#include "safety/safety_manual_drive.h"
#include "safety/safety_rc.h"
#include "task_motor.h"
#include "task_rc.h"
#include "task_supervisor.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stddef.h>

#define TASK_SAFETY_STACK_BYTES 4096u
#define TASK_SAFETY_PRIORITY    (tskIDLE_PRIORITY + 4)

static const char *TAG = "task_safety";

static app_manual_drive_snapshot_t g_manual_drive_snapshot;
static portMUX_TYPE g_manual_drive_snapshot_lock =
    portMUX_INITIALIZER_UNLOCKED;

static void task_safety_entry(void *argument);
static void task_safety_publish_manual_drive_snapshot(
    const app_manual_drive_snapshot_t *snapshot);

void task_safety_start(void)
{
    const BaseType_t ok = xTaskCreate(
        task_safety_entry,
        "task_safety",
        TASK_SAFETY_STACK_BYTES,
        NULL,
        TASK_SAFETY_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

void task_safety_get_manual_drive_snapshot(
    app_manual_drive_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    portENTER_CRITICAL(&g_manual_drive_snapshot_lock);
    *snapshot = g_manual_drive_snapshot;
    portEXIT_CRITICAL(&g_manual_drive_snapshot_lock);
}

static void task_safety_entry(void *argument)
{
    (void)argument;

    safety_manual_drive_t manual_drive;
    safety_rc_config_t rc_safety_config = safety_rc_default_config();
    const safety_manual_drive_config_t manual_drive_config =
    {
        .request_max_age_us = APP_MANUAL_DRIVE_REQUEST_MAX_AGE_US,
        .stop_hold_us = APP_MANUAL_DRIVE_STOP_HOLD_US,
        .channel_min = APP_RC_CHANNEL_MIN,
        .channel_max = APP_RC_CHANNEL_MAX,
        .stick_neutral_low = APP_MANUAL_DRIVE_STICK_NEUTRAL_LOW,
        .stick_neutral_high = APP_MANUAL_DRIVE_STICK_NEUTRAL_HIGH,
        .max_velocity_erpm = APP_MANUAL_DRIVE_MAX_VELOCITY_ERPM,
        .velocity_channel =
            (uint8_t)(APP_MANUAL_DRIVE_VELOCITY_CHANNEL - 1u),
        .velocity_positive_high =
            (APP_MANUAL_DRIVE_VELOCITY_POSITIVE_HIGH != 0),
    };

    rc_safety_config.max_age_us = APP_RC_MAX_AGE_US;
    rc_safety_config.channel_min = APP_RC_CHANNEL_MIN;
    rc_safety_config.channel_max = APP_RC_CHANNEL_MAX;
    rc_safety_config.channel_count = APP_RC_ACTIVE_CHANNEL_COUNT;

    if (!safety_manual_drive_init(&manual_drive,
                                  &manual_drive_config))
    {
        ESP_LOGE(TAG, "invalid manual-drive safety config");
        vTaskDelete(NULL);
        return;
    }

    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        app_manual_drive_request_t request;
        app_manual_drive_snapshot_t drive_snapshot;
        app_supervisor_snapshot_t supervisor_snapshot;
        actuator_snapshot_t actuator_snapshot;
        rc_snapshot_t rc_snapshot;
        safety_rc_status_t rc_status;
        const uint64_t now_us = (uint64_t)esp_timer_get_time();

        task_supervisor_get_snapshot(&supervisor_snapshot);
        task_rc_get_snapshot(&rc_snapshot);
        task_motor_get_snapshot(&actuator_snapshot);

        request.timestamp_us = supervisor_snapshot.timestamp_us;
        request.actuator_index =
            supervisor_snapshot.manual_drive_actuator_index;
        request.enabled =
            supervisor_snapshot.mode == APP_SYSTEM_MODE_MANUAL_DRIVE;

        safety_rc_check(&rc_snapshot,
                        now_us,
                        &rc_safety_config,
                        &rc_status);
        safety_manual_drive_update(&manual_drive,
                                   &request,
                                   &rc_snapshot,
                                   &rc_status,
                                   &actuator_snapshot,
                                   now_us,
                                   &drive_snapshot);

        task_motor_set_approved_manual_command(&drive_snapshot);
        task_safety_publish_manual_drive_snapshot(&drive_snapshot);

        vTaskDelayUntil(
            &last_wake,
            pdMS_TO_TICKS(APP_MANUAL_DRIVE_TASK_PERIOD_MS));
    }
}

static void task_safety_publish_manual_drive_snapshot(
    const app_manual_drive_snapshot_t *snapshot)
{
    portENTER_CRITICAL(&g_manual_drive_snapshot_lock);
    g_manual_drive_snapshot = *snapshot;
    portEXIT_CRITICAL(&g_manual_drive_snapshot_lock);
}
