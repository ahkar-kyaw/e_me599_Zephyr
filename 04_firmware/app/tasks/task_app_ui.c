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
#include "usart.h"

#define TASK_APP_UI_STACK_BYTES              2048u
#define TASK_APP_UI_PERIOD_MS                200u

#define TASK_APP_UI_NEXT_PAGE_CHANNEL_INDEX  6u
#define TASK_APP_UI_BUTTON_HIGH_US           1700u
#define TASK_APP_UI_BUTTON_LOW_US            1300u

typedef enum
{
    TASK_APP_UI_PAGE_1_BLANK = 0,
    TASK_APP_UI_PAGE_2_BLANK,
    TASK_APP_UI_PAGE_3_BLANK,
    TASK_APP_UI_PAGE_4_IMU,
    TASK_APP_UI_PAGE_5_CRSF,
    TASK_APP_UI_PAGE_COUNT
} task_app_ui_page_t;

static osThreadId_t s_task_app_ui_handle = NULL;

static StaticTask_t s_task_app_ui_cb;
static uint64_t s_task_app_ui_stack[TASK_APP_UI_STACK_BYTES / sizeof(uint64_t)];

static task_app_ui_page_t s_active_page = TASK_APP_UI_PAGE_1_BLANK;
static task_app_ui_page_t s_last_drawn_page = TASK_APP_UI_PAGE_COUNT;
static bool s_page_button_armed = true;

static void task_app_ui_thread(void *argument);

static void serial_write(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    const size_t len = strlen(text);

    if (len == 0u)
    {
        return;
    }

    (void)HAL_UART_Transmit(
        &huart3,
        (uint8_t *)text,
        (uint16_t)len,
        100u);
}

static void serial_printf(const char *format, ...)
{
    char buffer[192];
    va_list args;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    buffer[sizeof(buffer) - 1u] = '\0';

    serial_write(buffer);
}

static void app_ui_clear_all_oled_lines(void)
{
    for (uint8_t line = 0u; line < TASK_OLED_UI_LINE_COUNT; line++)
    {
        (void)task_oled_ui_clear_line(line);
    }
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
    else if (button_us <= TASK_APP_UI_BUTTON_LOW_US)
    {
        s_page_button_armed = true;
    }
    else
    {
        /* Hold current armed state inside hysteresis band. */
    }
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

static void app_ui_draw_blank_page(void)
{
    /* Intentionally blank. */
}

static void app_ui_draw_imu_raw(void)
{
    task_imu_snapshot_t imu;
    char line[24];

    if (!task_imu_get_snapshot(&imu))
    {
        return;
    }

    if (!imu.initialized || !imu.data_valid)
    {
        (void)task_oled_ui_set_line(0u, "AX:------");
        (void)task_oled_ui_set_line(1u, "AY:------");
        (void)task_oled_ui_set_line(2u, "AZ:------");
        (void)task_oled_ui_set_line(3u, "GX:------");
        (void)task_oled_ui_set_line(4u, "GY:------");
        (void)task_oled_ui_set_line(5u, "GZ:------");

        serial_write("IMU AX=---- AY=---- AZ=---- GX=---- GY=---- GZ=----\r\n");
        return;
    }

    (void)snprintf(line, sizeof(line), "AX:%6d", imu.ax_raw);
    (void)task_oled_ui_set_line(0u, line);

    (void)snprintf(line, sizeof(line), "AY:%6d", imu.ay_raw);
    (void)task_oled_ui_set_line(1u, line);

    (void)snprintf(line, sizeof(line), "AZ:%6d", imu.az_raw);
    (void)task_oled_ui_set_line(2u, line);

    (void)snprintf(line, sizeof(line), "GX:%6d", imu.gx_raw);
    (void)task_oled_ui_set_line(3u, line);

    (void)snprintf(line, sizeof(line), "GY:%6d", imu.gy_raw);
    (void)task_oled_ui_set_line(4u, line);

    (void)snprintf(line, sizeof(line), "GZ:%6d", imu.gz_raw);
    (void)task_oled_ui_set_line(5u, line);

    serial_printf(
        "IMU AX=%d AY=%d AZ=%d GX=%d GY=%d GZ=%d\r\n",
        imu.ax_raw,
        imu.ay_raw,
        imu.az_raw,
        imu.gx_raw,
        imu.gy_raw,
        imu.gz_raw);
}

static void app_ui_draw_crsf_channels(const task_crsf_snapshot_t *crsf)
{
    char line[24];

    if (crsf == NULL)
    {
        return;
    }

    if (!crsf->receiver_connected || !crsf->channels_valid)
    {
        (void)task_oled_ui_set_line(0u, "C1:---- C2:----");
        (void)task_oled_ui_set_line(1u, "C3:---- C4:----");
        (void)task_oled_ui_set_line(2u, "C5:---- C6:----");
        (void)task_oled_ui_set_line(3u, "C7:---- C8:----");

        serial_write("C1=---- C2=---- C3=---- C4=---- C5=---- C6=---- C7=---- C8=----\r\n");
        return;
    }

    app_ui_format_channel_pair(line, sizeof(line), 0u, crsf->us[0], 1u, crsf->us[1]);
    (void)task_oled_ui_set_line(0u, line);

    app_ui_format_channel_pair(line, sizeof(line), 2u, crsf->us[2], 3u, crsf->us[3]);
    (void)task_oled_ui_set_line(1u, line);

    app_ui_format_channel_pair(line, sizeof(line), 4u, crsf->us[4], 5u, crsf->us[5]);
    (void)task_oled_ui_set_line(2u, line);

    app_ui_format_channel_pair(line, sizeof(line), 6u, crsf->us[6], 7u, crsf->us[7]);
    (void)task_oled_ui_set_line(3u, line);

    serial_printf(
        "C1=%u C2=%u C3=%u C4=%u C5=%u C6=%u C7=%u C8=%u\r\n",
        crsf->us[0],
        crsf->us[1],
        crsf->us[2],
        crsf->us[3],
        crsf->us[4],
        crsf->us[5],
        crsf->us[6],
        crsf->us[7]);
}

static void app_ui_draw_active_page(const task_crsf_snapshot_t *crsf)
{
    if (s_active_page != s_last_drawn_page)
    {
        app_ui_clear_all_oled_lines();
        s_last_drawn_page = s_active_page;
    }

    switch (s_active_page)
    {
        case TASK_APP_UI_PAGE_1_BLANK:
        case TASK_APP_UI_PAGE_2_BLANK:
        case TASK_APP_UI_PAGE_3_BLANK:
            app_ui_draw_blank_page();
            break;

        case TASK_APP_UI_PAGE_4_IMU:
            app_ui_draw_imu_raw();
            break;

        case TASK_APP_UI_PAGE_5_CRSF:
            app_ui_draw_crsf_channels(crsf);
            break;

        default:
            s_active_page = TASK_APP_UI_PAGE_1_BLANK;
            break;
    }
}

static void task_app_ui_thread(void *argument)
{
    (void)argument;

    (void)task_oled_ui_select_page(TASK_OLED_UI_PAGE_LINES);
    app_ui_clear_all_oled_lines();

    for (;;)
    {
        task_crsf_snapshot_t crsf;

        if (task_crsf_get_snapshot(&crsf))
        {
            app_ui_update_page_button(&crsf);
            app_ui_draw_active_page(&crsf);
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