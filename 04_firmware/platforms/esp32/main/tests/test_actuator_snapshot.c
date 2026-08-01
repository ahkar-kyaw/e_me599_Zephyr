#include "test_actuator_snapshot.h"

#include "app/app_actuator_types.h"
#include "config_actuator.h"
#include "task_motor.h"
#include "task_safety.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define TEST_ACTUATOR_SNAPSHOT_STACK_BYTES 4096u
#define TEST_ACTUATOR_SNAPSHOT_PRIORITY    (tskIDLE_PRIORITY + 1)
#define TEST_ACTUATOR_SNAPSHOT_LINE_BYTES  384u

static const char *TAG = "test_actuator";

static void test_actuator_snapshot_entry(void *argument);
static const char *test_actuator_bus_name(
    app_actuator_bus_state_t state);
static const char *test_actuator_feedback_name(
    const actuator_feedback_snapshot_t *feedback);
static const char *test_actuator_drive_name(
    app_manual_drive_state_t state);
static size_t test_actuator_append(char *line,
                                   size_t line_size,
                                   size_t used,
                                   const char *format,
                                   ...);

void test_actuator_snapshot_start(void)
{
    const BaseType_t ok = xTaskCreate(
        test_actuator_snapshot_entry,
        "test_actuator",
        TEST_ACTUATOR_SNAPSHOT_STACK_BYTES,
        NULL,
        TEST_ACTUATOR_SNAPSHOT_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

static void test_actuator_snapshot_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        actuator_snapshot_t snapshot;
        app_manual_drive_snapshot_t drive;
        char line[TEST_ACTUATOR_SNAPSHOT_LINE_BYTES];

        task_motor_get_snapshot(&snapshot);
        task_safety_get_manual_drive_snapshot(&drive);

        size_t used = test_actuator_append(
            line,
            sizeof(line),
            0u,
            "CAN=%s",
            test_actuator_bus_name(snapshot.bus_state));

        for (uint32_t i = 0u; i < APP_ACTUATOR_COUNT; i++)
        {
            const actuator_feedback_snapshot_t *feedback =
                &snapshot.actuator[i];

            if (!feedback->configured)
            {
                continue;
            }

            used = test_actuator_append(
                line,
                sizeof(line),
                used,
                " M%lu=%s P=%+.1f V=%+.0f I=%+.2f T=%d F=%u",
                (unsigned long)(i + 1u),
                test_actuator_feedback_name(feedback),
                (double)feedback->position_deg,
                (double)feedback->velocity_erpm,
                (double)feedback->current_a,
                (int)feedback->temperature_c,
                (unsigned int)feedback->fault_code);
        }

        (void)test_actuator_append(
            line,
            sizeof(line),
            used,
            " DRV=%s CMD=%+.0f TXE=%lu",
            test_actuator_drive_name(drive.state),
            (double)drive.velocity_erpm,
            (unsigned long)snapshot.command_tx_error_count);

        ESP_LOGI(TAG, "%s", line);

        vTaskDelay(pdMS_TO_TICKS(
            APP_ACTUATOR_DIAGNOSTIC_PERIOD_MS));
    }
}

static const char *test_actuator_bus_name(
    app_actuator_bus_state_t state)
{
    switch (state)
    {
        case APP_ACTUATOR_BUS_ACTIVE:
            return "ACTIVE";

        case APP_ACTUATOR_BUS_WARNING:
            return "WARNING";

        case APP_ACTUATOR_BUS_PASSIVE:
            return "PASSIVE";

        case APP_ACTUATOR_BUS_OFF:
            return "OFF";

        default:
            return "WAIT";
    }
}

static const char *test_actuator_feedback_name(
    const actuator_feedback_snapshot_t *feedback)
{
    if (feedback->timestamp_us == 0u)
    {
        return "WAIT";
    }

    if (feedback->fault_code != 0u)
    {
        return "FAULT";
    }

    return feedback->valid ? "OK" : "STALE";
}

static const char *test_actuator_drive_name(
    app_manual_drive_state_t state)
{
    switch (state)
    {
        case APP_MANUAL_DRIVE_WAIT_SAFE:
            return "BLOCK";

        case APP_MANUAL_DRIVE_WAIT_NEUTRAL:
            return "WAIT";

        case APP_MANUAL_DRIVE_ARMED:
            return "LIVE";

        case APP_MANUAL_DRIVE_STOPPING:
            return "STOP";

        case APP_MANUAL_DRIVE_DISABLED:
        default:
            return "OFF";
    }
}

static size_t test_actuator_append(char *line,
                                   size_t line_size,
                                   size_t used,
                                   const char *format,
                                   ...)
{
    if ((line == NULL) || (line_size == 0u) ||
        (used >= (line_size - 1u)) || (format == NULL))
    {
        return used;
    }

    va_list arguments;

    va_start(arguments, format);
    const int written = vsnprintf(&line[used],
                                  line_size - used,
                                  format,
                                  arguments);
    va_end(arguments);

    if (written <= 0)
    {
        return used;
    }

    const size_t available = line_size - used;

    if ((size_t)written >= available)
    {
        return line_size - 1u;
    }

    return used + (size_t)written;
}
