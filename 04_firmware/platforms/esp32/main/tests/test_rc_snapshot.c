#include "test_rc_snapshot.h"

#include "config_rc.h"
#include "safety/safety_rc.h"
#include "task_rc.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_RC_SNAPSHOT_STACK_BYTES  4096u
#define TEST_RC_SNAPSHOT_PRIORITY     (tskIDLE_PRIORITY + 1)
#define TEST_RC_SNAPSHOT_PERIOD_MS    1000u

static const char *TAG = "test_rc_snapshot";

static void test_rc_snapshot_entry(void *argument);

void test_rc_snapshot_start(void)
{
    const BaseType_t ok = xTaskCreate(
        test_rc_snapshot_entry,
        "test_rc_snapshot",
        TEST_RC_SNAPSHOT_STACK_BYTES,
        NULL,
        TEST_RC_SNAPSHOT_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

static void test_rc_snapshot_entry(void *argument)
{
    (void)argument;

    safety_rc_config_t config = safety_rc_default_config();

    config.max_age_us = APP_RC_MAX_AGE_US;
    config.channel_min = APP_RC_CHANNEL_MIN;
    config.channel_max = APP_RC_CHANNEL_MAX;
    config.channel_count = APP_RC_ACTIVE_CHANNEL_COUNT;

    for (;;)
    {
        rc_snapshot_t snapshot;
        safety_rc_status_t status;

        task_rc_get_snapshot(&snapshot);
        safety_rc_check(&snapshot,
                        (uint64_t)esp_timer_get_time(),
                        &config,
                        &status);

        ESP_LOGI(TAG,
                 "valid=%d safe=%d faults=0x%08lx age_us=%llu frames=%lu crc=%lu parse=%lu uart=%lu",
                 snapshot.valid ? 1 : 0,
                 status.safe_for_control ? 1 : 0,
                 (unsigned long)status.fault_flags,
                 (unsigned long long)status.age_us,
                 (unsigned long)snapshot.frame_count,
                 (unsigned long)snapshot.crc_error_count,
                 (unsigned long)snapshot.parse_error_count,
                 (unsigned long)snapshot.uart_error_count);

        for (uint32_t first = 0u;
             first < APP_RC_CHANNEL_COUNT;
             first += 4u)
        {
            ESP_LOGI(TAG,
                     "ch%02lu=%4u ch%02lu=%4u ch%02lu=%4u ch%02lu=%4u",
                     (unsigned long)(first + 1u),
                     (unsigned int)snapshot.channel[first],
                     (unsigned long)(first + 2u),
                     (unsigned int)snapshot.channel[first + 1u],
                     (unsigned long)(first + 3u),
                     (unsigned int)snapshot.channel[first + 2u],
                     (unsigned long)(first + 4u),
                     (unsigned int)snapshot.channel[first + 3u]);
        }

        vTaskDelay(pdMS_TO_TICKS(TEST_RC_SNAPSHOT_PERIOD_MS));
    }
}
