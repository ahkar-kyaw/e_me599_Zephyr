#include "task_ui.h"

#include "bsp_oled_i2c_esp32.h"
#include "config_imu.h"
#include "config_rc.h"
#include "config_ui.h"
#include "drivers/drv_ssd1306.h"
#include "safety/safety_imu.h"
#include "safety/safety_rc.h"
#include "task_imu.h"
#include "task_rc.h"
#include "ui/ui_pages.h"
#include "ui/ui_rc_input.h"
#include "ui/ui_state.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>

#define TASK_UI_STACK_BYTES  6144u
#define TASK_UI_PRIORITY     (tskIDLE_PRIORITY + 1)

static const char *TAG = "task_ui";

static void task_ui_entry(void *argument);

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
    safety_rc_config_t rc_safety_config = safety_rc_default_config();
    safety_imu_config_t imu_safety_config = safety_imu_default_config();
    ui_rc_input_t rc_input;
    ui_state_t ui_state;

    const ui_rc_input_config_t rc_input_config =
    {
        .axis_channel = (uint8_t)(APP_UI_RC_AXIS_CHANNEL - 1u),
        .interact_channel =
            (uint8_t)(APP_UI_RC_INTERACT_CHANNEL - 1u),
        .enter_channel = (uint8_t)(APP_UI_RC_ENTER_CHANNEL - 1u),
        .channel_min = APP_RC_CHANNEL_MIN,
        .channel_max = APP_RC_CHANNEL_MAX,
        .axis_left_threshold = APP_UI_RC_AXIS_LEFT_THRESHOLD,
        .axis_neutral_low = APP_UI_RC_AXIS_NEUTRAL_LOW,
        .axis_neutral_high = APP_UI_RC_AXIS_NEUTRAL_HIGH,
        .axis_right_threshold = APP_UI_RC_AXIS_RIGHT_THRESHOLD,
        .switch_off_threshold = APP_UI_RC_SWITCH_OFF_THRESHOLD,
        .switch_on_threshold = APP_UI_RC_SWITCH_ON_THRESHOLD,
        .interact_active_high =
            (APP_UI_RC_INTERACT_ACTIVE_HIGH != 0),
        .enter_active_high = (APP_UI_RC_ENTER_ACTIVE_HIGH != 0),
    };

    display_config.i2c_address = APP_OLED_I2C_ADDRESS;
    display_config.width = APP_OLED_WIDTH;
    display_config.height = APP_OLED_HEIGHT;
    display_config.contrast = APP_OLED_CONTRAST;
    display_config.rotate_180 = (APP_OLED_ROTATE_180 != 0);
    display_config.invert = (APP_OLED_INVERT != 0);

    rc_safety_config.max_age_us = APP_RC_MAX_AGE_US;
    rc_safety_config.channel_min = APP_RC_CHANNEL_MIN;
    rc_safety_config.channel_max = APP_RC_CHANNEL_MAX;

    imu_safety_config.max_age_us = APP_IMU_MAX_AGE_US;
    imu_safety_config.max_abs_roll_rad = APP_IMU_MAX_ABS_ROLL_RAD;
    imu_safety_config.max_abs_pitch_rad = APP_IMU_MAX_ABS_PITCH_RAD;
    imu_safety_config.min_accel_norm_mps2 =
        APP_IMU_MIN_ACCEL_NORM_MPS2;
    imu_safety_config.max_accel_norm_mps2 =
        APP_IMU_MAX_ACCEL_NORM_MPS2;

    if (!ui_rc_input_init(&rc_input, &rc_input_config))
    {
        ESP_LOGE(TAG, "invalid UI RC input config");
        vTaskDelete(NULL);
        return;
    }

    ui_state_init(&ui_state);

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

    ESP_LOGI(TAG,
             "OLED UI started: axis=CH%u interact=CH%u enter=CH%u",
             (unsigned int)APP_UI_RC_AXIS_CHANNEL,
             (unsigned int)APP_UI_RC_INTERACT_CHANNEL,
             (unsigned int)APP_UI_RC_ENTER_CHANNEL);

    TickType_t last_wake = xTaskGetTickCount();
    TickType_t last_render = last_wake;
    bool render_required = true;

    for (;;)
    {
        rc_snapshot_t rc_snapshot;
        imu_snapshot_t imu_snapshot;
        safety_rc_status_t rc_status;
        safety_imu_status_t imu_status;
        const uint64_t now_us = (uint64_t)esp_timer_get_time();

        task_rc_get_snapshot(&rc_snapshot);
        task_imu_get_snapshot(&imu_snapshot);

        safety_rc_check(&rc_snapshot,
                        now_us,
                        &rc_safety_config,
                        &rc_status);
        (void)safety_imu_check(&imu_snapshot,
                               now_us,
                               &imu_safety_config,
                               &imu_status);

        const ui_event_flags_t events =
            ui_rc_input_update(&rc_input,
                               &rc_snapshot,
                               (rc_status.fault_flags &
                                (SAFETY_RC_FAULT_NO_DATA |
                                 SAFETY_RC_FAULT_STALE)) == 0u);

        if (ui_state_update(&ui_state, events))
        {
            render_required = true;

            ESP_LOGI(TAG,
                     "page=%u subpage=%u mode=%s",
                     (unsigned int)(ui_state.page + 1u),
                     (unsigned int)(ui_state.subpage + 1u),
                     (ui_state.mode == UI_MODE_INTERACT)
                         ? "interact"
                         : "browse");
        }

        if ((events & UI_EVENT_ENTER) != 0u)
        {
            ESP_LOGI(TAG,
                     "enter requested on page=%u subpage=%u",
                     (unsigned int)(ui_state.page + 1u),
                     (unsigned int)(ui_state.subpage + 1u));
        }

        const TickType_t now_tick = xTaskGetTickCount();

        if (render_required ||
            ((now_tick - last_render) >=
             pdMS_TO_TICKS(APP_UI_RENDER_PERIOD_MS)))
        {
            const ui_page_model_t model =
            {
                .rc_snapshot = &rc_snapshot,
                .rc_status = &rc_status,
                .imu_snapshot = &imu_snapshot,
                .imu_status = &imu_status,
            };

            ui_pages_render(&display, &ui_state, &model);

            if (drv_ssd1306_update(&display) != DRV_SSD1306_OK)
            {
                ESP_LOGE(TAG, "OLED update failed");
            }

            last_render = now_tick;
            render_required = false;
        }

        vTaskDelayUntil(&last_wake,
                        pdMS_TO_TICKS(APP_UI_TASK_PERIOD_MS));
    }
}
