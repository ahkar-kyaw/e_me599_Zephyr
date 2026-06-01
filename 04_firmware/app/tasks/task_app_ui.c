#include "task_app_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "task_crsf.h"
#include "task_imu.h"
#include "task_oled_ui.h"
#include "task_log.h"

#define TASK_APP_UI_STACK_BYTES              2048u
#define TASK_APP_UI_PERIOD_MS                200u

#define TASK_APP_UI_NEXT_PAGE_CHANNEL_INDEX  6u
#define TASK_APP_UI_BUTTON_HIGH_US           1700u
#define TASK_APP_UI_BUTTON_LOW_US            1300u

typedef enum
{
    TASK_APP_UI_PAGE_1 = 0,
    TASK_APP_UI_PAGE_2,
    TASK_APP_UI_PAGE_3,
    TASK_APP_UI_PAGE_4,
    TASK_APP_UI_PAGE_5,
    TASK_APP_UI_PAGE_COUNT
} task_app_ui_page_t;

static osThreadId_t s_task_app_ui_handle = NULL;

static StaticTask_t s_task_app_ui_cb;
static uint64_t s_task_app_ui_stack[TASK_APP_UI_STACK_BYTES / sizeof(uint64_t)];

static task_app_ui_page_t s_active_page = TASK_APP_UI_PAGE_1;
static bool s_page_button_armed = true;

static void task_app_ui_thread(void *argument);

static void app_ui_screen_clear(task_oled_ui_screen_t screen)
{
    for (uint8_t line = 0u; line < TASK_OLED_UI_LINE_COUNT; line++)
    {
        screen[line][0] = '\0';
    }
}

static void app_ui_screen_set_line(
    task_oled_ui_screen_t screen,
    uint8_t line,
    const char *text)
{
    if ((line >= TASK_OLED_UI_LINE_COUNT) || (text == NULL))
    {
        return;
    }

    strncpy(screen[line], text, TASK_OLED_UI_LINE_MAX_LEN - 1u);
    screen[line][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
}

static void app_ui_screen_printf(
    task_oled_ui_screen_t screen,
    uint8_t line,
    const char *format,
    ...)
{
    va_list args;

    if ((line >= TASK_OLED_UI_LINE_COUNT) || (format == NULL))
    {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(
        screen[line],
        TASK_OLED_UI_LINE_MAX_LEN,
        format,
        args);
    va_end(args);

    screen[line][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
}

static void app_ui_next_page(void)
{
    uint8_t next = (uint8_t)s_active_page;

    next++;

    if (next >= (uint8_t)TASK_APP_UI_PAGE_COUNT)
    {
        next = 0u;
    }

    s_active_page = (task_app_ui_page_t)next;
}

static void app_ui_update_page_button(const task_crsf_snapshot_t *crsf)
{
    if (crsf == NULL)
    {
        return;
    }

    if (!crsf->receiver_connected || !crsf->channels_valid)
    {
        s_page_button_armed = true;
        return;
    }

    const uint16_t button_us = crsf->us[TASK_APP_UI_NEXT_PAGE_CHANNEL_INDEX];

    if (s_page_button_armed && (button_us >= TASK_APP_UI_BUTTON_HIGH_US))
    {
        app_ui_next_page();
        s_page_button_armed = false;
    }

    if (button_us <= TASK_APP_UI_BUTTON_LOW_US)
    {
        s_page_button_armed = true;
    }
}

static void app_ui_draw_page_title(
    task_oled_ui_screen_t screen,
    task_app_ui_page_t page)
{
    app_ui_screen_printf(
        screen,
        0u,
        "PAGE %u/%u",
        (unsigned int)((uint8_t)page + 1u),
        (unsigned int)TASK_APP_UI_PAGE_COUNT);
}

static void app_ui_draw_page_imu(task_oled_ui_screen_t screen)
{
    task_imu_snapshot_t imu;

    if (!task_imu_get_snapshot(&imu))
    {
        return;
    }

    if (!imu.initialized || !imu.data_valid)
    {
        app_ui_screen_printf(screen, 1u, "WHO:0x%02X", imu.who_am_i);

        app_ui_screen_printf(
            screen,
            2u,
            "INIT:%u READ:%u",
            (unsigned int)imu.init_result,
            (unsigned int)imu.last_read_result);

        app_ui_screen_printf(
            screen,
            3u,
            "ERR:%lu",
            (unsigned long)imu.error_count);

        return;
    }

    app_ui_screen_printf(screen, 1u, "WHO:0x%02X", imu.who_am_i);

    app_ui_screen_printf(
        screen,
        2u,
        "AX:%6d AY:%6d",
        imu.ax_raw,
        imu.ay_raw);

    app_ui_screen_printf(
        screen,
        3u,
        "AZ:%6d",
        imu.az_raw);

    app_ui_screen_printf(
        screen,
        4u,
        "GX:%6d GY:%6d",
        imu.gx_raw,
        imu.gy_raw);

    app_ui_screen_printf(
        screen,
        5u,
        "GZ:%6d",
        imu.gz_raw);

    app_ui_screen_printf(
        screen,
        6u,
        "N:%lu",
        (unsigned long)imu.sample_count);

    app_ui_screen_printf(
        screen,
        7u,
        "ERR:%lu",
        (unsigned long)imu.error_count);
}

static void app_ui_format_channel_pair(
    char *line,
    size_t line_len,
    uint8_t ch_a,
    uint16_t value_a,
    uint8_t ch_b,
    uint16_t value_b)
{
    if ((line == NULL) || (line_len == 0u))
    {
        return;
    }

    (void)snprintf(
        line,
        line_len,
        "C%u:%4u C%u:%4u",
        (unsigned int)(ch_a + 1u),
        value_a,
        (unsigned int)(ch_b + 1u),
        value_b);
}

static void app_ui_draw_page_crsf(
    task_oled_ui_screen_t screen,
    const task_crsf_snapshot_t *crsf)
{
    char line[24];

    if (crsf == NULL)
    {
        return;
    }

    if (!crsf->receiver_connected || !crsf->channels_valid)
    {
        app_ui_screen_set_line(screen, 1u, "C1:---- C2:----");
        app_ui_screen_set_line(screen, 2u, "C3:---- C4:----");
        app_ui_screen_set_line(screen, 3u, "C5:---- C6:----");
        app_ui_screen_set_line(screen, 4u, "C7:---- C8:----");
        app_ui_screen_set_line(screen, 5u, "C9:---- C10:---");

        return;
    }

    app_ui_format_channel_pair(line, sizeof(line), 0u, crsf->us[0], 1u, crsf->us[1]);
    app_ui_screen_set_line(screen, 1u, line);

    app_ui_format_channel_pair(line, sizeof(line), 2u, crsf->us[2], 3u, crsf->us[3]);
    app_ui_screen_set_line(screen, 2u, line);

    app_ui_format_channel_pair(line, sizeof(line), 4u, crsf->us[4], 5u, crsf->us[5]);
    app_ui_screen_set_line(screen, 3u, line);

    app_ui_format_channel_pair(line, sizeof(line), 6u, crsf->us[6], 7u, crsf->us[7]);
    app_ui_screen_set_line(screen, 4u, line);

    app_ui_format_channel_pair(line, sizeof(line), 8u, crsf->us[8], 9u, crsf->us[9]);
    app_ui_screen_set_line(screen, 5u, line);

    (void)task_log_printf(
        "C1=%u C2=%u C3=%u C4=%u C5=%u "
        "C6=%u C7=%u C8=%u C9=%u C10=%u\r\n",
        crsf->us[0],
        crsf->us[1],
        crsf->us[2],
        crsf->us[3],
        crsf->us[4],
        crsf->us[5],
        crsf->us[6],
        crsf->us[7],
        crsf->us[8],
        crsf->us[9]);
}

static void app_ui_draw_page_blank(task_oled_ui_screen_t screen)
{
    (void)screen;
}

static void app_ui_draw_active_page(
    task_oled_ui_screen_t screen,
    const task_crsf_snapshot_t *crsf)
{
    app_ui_draw_page_title(screen, s_active_page);

    switch (s_active_page)
    {
        case TASK_APP_UI_PAGE_1:
            app_ui_draw_page_imu(screen);
            break;

        case TASK_APP_UI_PAGE_2:
            app_ui_draw_page_crsf(screen, crsf);
            break;

        case TASK_APP_UI_PAGE_3:
        case TASK_APP_UI_PAGE_4:
        case TASK_APP_UI_PAGE_5:
            app_ui_draw_page_blank(screen);
            break;

        default:
            s_active_page = TASK_APP_UI_PAGE_1;
            break;
    }
}

static void task_app_ui_thread(void *argument)
{
    (void)argument;

    (void)task_oled_ui_select_page(TASK_OLED_UI_PAGE_LINES);

    for (;;)
    {
        task_crsf_snapshot_t crsf;
        task_oled_ui_screen_t screen;

        app_ui_screen_clear(screen);

        if (task_crsf_get_snapshot(&crsf))
        {
            app_ui_update_page_button(&crsf);
            app_ui_draw_active_page(screen, &crsf);
            (void)task_oled_ui_set_screen(screen);
        }

        osDelay(TASK_APP_UI_PERIOD_MS);
    }
}

void task_app_ui_start(void)
{
    if (s_task_app_ui_handle != NULL)
    {
        return;
    }

    static const osThreadAttr_t task_attributes =
    {
        .name = "app_ui",
        .attr_bits = 0u,
        .cb_mem = &s_task_app_ui_cb,
        .cb_size = sizeof(s_task_app_ui_cb),
        .stack_mem = s_task_app_ui_stack,
        .stack_size = sizeof(s_task_app_ui_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_task_app_ui_handle = osThreadNew(task_app_ui_thread, NULL, &task_attributes);
}