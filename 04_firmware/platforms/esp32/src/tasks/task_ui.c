#include "task_ui.h"

#include "bsp_oled_i2c_esp32.h"
#include "config_rc.h"
#include "config_ui.h"
#include "drivers/drv_ssd1306.h"
#include "safety/safety_rc.h"
#include "task_rc.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>

#define TASK_UI_STACK_BYTES  4096u
#define TASK_UI_PRIORITY     (tskIDLE_PRIORITY + 1)

static const char *TAG = "task_ui";

static void task_ui_entry(void *argument);
static const char *task_ui_rc_state(const safety_rc_status_t *status);

void task_ui_start(void)
{
    const BaseType_t ok = xTaskCreate(
        task_ui_entry,
        "task_ui",
        TASK_UI_STACK_BYTES,
        NULL,
        TASK_UI_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

static void task_ui_entry(void *argument)
{
    (void)argument;

    bsp_oled_i2c_esp32_t oled_i2c;
    drv_ssd1306_t display;
    drv_ssd1306_config_t display_config =
        drv_ssd1306_default_config();
    safety_rc_config_t rc_config = safety_rc_default_config();

    display_config.i2c_address = APP_OLED_I2C_ADDRESS;
    display_config.width = APP_OLED_WIDTH;
    display_config.height = APP_OLED_HEIGHT;
    display_config.contrast = APP_OLED_CONTRAST;
    display_config.rotate_180 = (APP_OLED_ROTATE_180 != 0);
    display_config.invert = (APP_OLED_INVERT != 0);

    rc_config.max_age_us = APP_RC_MAX_AGE_US;
    rc_config.channel_min = APP_RC_CHANNEL_MIN;
    rc_config.channel_max = APP_RC_CHANNEL_MAX;

    while (bsp_oled_i2c_esp32_init(&oled_i2c) != IF_I2C_OK)
    {
        ESP_LOGE(TAG, "OLED I2C init failed");
        vTaskDelay(pdMS_TO_TICKS(APP_UI_RETRY_MS));
    }

    while (drv_ssd1306_init(&display,
                            &oled_i2c.i2c,
                            &display_config) != DRV_SSD1306_OK)
    {
        ESP_LOGE(TAG, "SSD1306 init failed");
        vTaskDelay(pdMS_TO_TICKS(APP_UI_RETRY_MS));
    }

    ESP_LOGI(TAG, "OLED UI task started");

    TickType_t last_wake = xTaskGetTickCount();

    for (;;)
    {
        rc_snapshot_t snapshot;
        safety_rc_status_t rc_status;
        char line[24];
        const uint64_t now_us = (uint64_t)esp_timer_get_time();
        const uint32_t page =
            (uint32_t)((now_us / 1000u) / APP_UI_PAGE_PERIOD_MS) % 2u;

        task_rc_get_snapshot(&snapshot);
        safety_rc_check(&snapshot, now_us, &rc_config, &rc_status);

        drv_ssd1306_clear(&display);
        drv_ssd1306_draw_text(&display, 0u, 0u, "ZEPHYR ESP32");

        (void)snprintf(line,
                       sizeof(line),
                       "RC %s",
                       task_ui_rc_state(&rc_status));
        drv_ssd1306_draw_text(&display, 0u, 8u, line);

        (void)snprintf(line,
                       sizeof(line),
                       "FRAMES %lu",
                       (unsigned long)snapshot.frame_count);
        drv_ssd1306_draw_text(&display, 0u, 16u, line);

        for (uint32_t row = 0u; row < 4u; row++)
        {
            const uint32_t first = (page * 8u) + (row * 2u);
            const uint32_t second = first + 1u;

            (void)snprintf(line,
                           sizeof(line),
                           "C%02lu %04u C%02lu %04u",
                           (unsigned long)(first + 1u),
                           (unsigned int)snapshot.channel[first],
                           (unsigned long)(second + 1u),
                           (unsigned int)snapshot.channel[second]);
            drv_ssd1306_draw_text(&display,
                                   0u,
                                   (uint8_t)(24u + (row * 8u)),
                                   line);
        }

        (void)snprintf(line,
                       sizeof(line),
                       "PAGE %lu OF 2",
                       (unsigned long)(page + 1u));
        drv_ssd1306_draw_text(&display, 0u, 56u, line);

        if (drv_ssd1306_update(&display) != DRV_SSD1306_OK)
        {
            ESP_LOGE(TAG, "OLED update failed");
        }

        vTaskDelayUntil(&last_wake,
                        pdMS_TO_TICKS(APP_UI_TASK_PERIOD_MS));
    }
}

static const char *task_ui_rc_state(const safety_rc_status_t *status)
{
    if ((status->fault_flags & SAFETY_RC_FAULT_NO_DATA) != 0u)
    {
        return "WAIT";
    }

    if ((status->fault_flags & SAFETY_RC_FAULT_STALE) != 0u)
    {
        return "STALE";
    }

    if ((status->fault_flags & SAFETY_RC_FAULT_CHANNEL_RANGE) != 0u)
    {
        return "RANGE";
    }

    return "OK";
}
