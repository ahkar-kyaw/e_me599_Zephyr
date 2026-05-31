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

static void app_ui_format_cdeg(char *out, size_t out_len, int16_t cdeg)
{
    int32_t value = cdeg;
    char sign = '+';

    if ((out == NULL) || (out_len == 0u))
    {
        return;
    }

    if (value < 0)
    {
        sign = '-';
        value = -value;
    }

    (void)snprintf(
        out,
        out_len,
        "%c%ld.%02ld",
        sign,
        (long)(value / 100),
        (long)(value % 100));
}

static void app_ui_format_mdps_1dp(char *out, size_t out_len, int32_t mdps)
{
    int32_t value = mdps;
    char sign = '+';

    if ((out == NULL) || (out_len == 0u))
    {
        return;
    }

    if (value < 0)
    {
        sign = '-';
        value = -value;
    }

    (void)snprintf(
        out,
        out_len,
        "%c%ld.%01ld",
        sign,
        (long)(value / 1000),
        (long)((value % 1000) / 100));
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

static void app_ui_on_enter_page(task_app_ui_page_t page)
{
    (void)page;
    app_ui_clear_all_oled_lines();
}

static void app_ui_draw_page_title(task_app_ui_page_t page)
{
    char line[24];

    (void)snprintf(
        line,
        sizeof(line),
        "PAGE %u/%u",
        (unsigned int)((uint8_t)page + 1u),
        (unsigned int)TASK_APP_UI_PAGE_COUNT);

    (void)task_oled_ui_set_line(0u, line);
}

static void app_ui_draw_page_1(void)
{
    /* Reserved for future main status page. */
}

static void app_ui_draw_page_2(void)
{
    /* Reserved for future power page. */
}

static void app_ui_draw_page_3(void)
{
    /* Reserved for future motor or safety page. */
}

static void app_ui_draw_page_imu(void)
{
    task_imu_snapshot_t imu;
    char line[24];
    char pitch[12];
    char roll[12];
    char gx[12];
    char gy[12];

    if (!task_imu_get_snapshot(&imu))
    {
        return;
    }

    if (imu.state == TASK_IMU_STATE_CALIBRATING)
    {
        (void)task_oled_ui_set_line(1u, "IMU CAL");

        const uint32_t calibration_target =
            (imu.calibration_target > 0u) ? imu.calibration_target : 1u;

        (void)snprintf(
            line,
            sizeof(line),
            "%lu/%lu",
            (unsigned long)imu.calibration_count,
            (unsigned long)calibration_target);
        (void)task_oled_ui_set_line(2u, line);

        (void)task_oled_ui_clear_line(3u);
        (void)task_oled_ui_clear_line(4u);
        (void)task_oled_ui_clear_line(5u);
        (void)task_oled_ui_clear_line(6u);
        (void)task_oled_ui_clear_line(7u);

        return;
    }

    if (!imu.initialized || !imu.data_valid)
    {
        (void)snprintf(line, sizeof(line), "WHO:0x%02X", imu.who_am_i);
        (void)task_oled_ui_set_line(1u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "INIT:%u READ:%u",
            (unsigned int)imu.init_result,
            (unsigned int)imu.last_read_result);
        (void)task_oled_ui_set_line(2u, line);

        (void)snprintf(line, sizeof(line), "ERR:%lu", (unsigned long)imu.error_count);
        (void)task_oled_ui_set_line(3u, line);

        (void)task_oled_ui_clear_line(4u);
        (void)task_oled_ui_clear_line(5u);
        (void)task_oled_ui_clear_line(6u);
        (void)task_oled_ui_clear_line(7u);

        serial_printf(
            "IMU FAIL WHO=0x%02X INIT=%u READ=%u ERR=%lu\r\n",
            imu.who_am_i,
            (unsigned int)imu.init_result,
            (unsigned int)imu.last_read_result,
            (unsigned long)imu.error_count);

        return;
    }

    app_ui_format_cdeg(pitch, sizeof(pitch), imu.pitch_cdeg);
    app_ui_format_cdeg(roll, sizeof(roll), imu.roll_cdeg);
    app_ui_format_mdps_1dp(gx, sizeof(gx), imu.gx_mdps);
    app_ui_format_mdps_1dp(gy, sizeof(gy), imu.gy_mdps);

    (void)snprintf(line, sizeof(line), "P:%s R:%s", pitch, roll);
    (void)task_oled_ui_set_line(1u, line);

    (void)snprintf(line, sizeof(line), "AX:%ldmg", (long)imu.ax_mg);
    (void)task_oled_ui_set_line(2u, line);

    (void)snprintf(line, sizeof(line), "AY:%ldmg", (long)imu.ay_mg);
    (void)task_oled_ui_set_line(3u, line);

    (void)snprintf(line, sizeof(line), "AZ:%ldmg", (long)imu.az_mg);
    (void)task_oled_ui_set_line(4u, line);

    (void)snprintf(line, sizeof(line), "GX:%sdps", gx);
    (void)task_oled_ui_set_line(5u, line);

    (void)snprintf(line, sizeof(line), "GY:%sdps", gy);
    (void)task_oled_ui_set_line(6u, line);

    (void)snprintf(line, sizeof(line), "N:%lu", (unsigned long)imu.sample_count);
    (void)task_oled_ui_set_line(7u, line);

    serial_printf(
        "IMU P=%d R=%d AX=%ld AY=%ld AZ=%ld GX=%ld GY=%ld GZ=%ld N=%lu\r\n",
        imu.pitch_cdeg,
        imu.roll_cdeg,
        (long)imu.ax_mg,
        (long)imu.ay_mg,
        (long)imu.az_mg,
        (long)imu.gx_mdps,
        (long)imu.gy_mdps,
        (long)imu.gz_mdps,
        (unsigned long)imu.sample_count);
}

static void app_ui_draw_page_crsf(const task_crsf_snapshot_t *crsf)
{
    char line[24];

    if (crsf == NULL)
    {
        return;
    }

    if (!crsf->receiver_connected || !crsf->channels_valid)
    {
        (void)task_oled_ui_set_line(1u, "C1:---- C2:----");
        (void)task_oled_ui_set_line(2u, "C3:---- C4:----");
        (void)task_oled_ui_set_line(3u, "C5:---- C6:----");
        (void)task_oled_ui_set_line(4u, "C7:---- C8:----");

        (void)task_oled_ui_clear_line(5u);
        (void)task_oled_ui_clear_line(6u);
        (void)task_oled_ui_clear_line(7u);

        serial_write("C1=---- C2=---- C3=---- C4=---- C5=---- C6=---- C7=---- C8=----\r\n");
        return;
    }

    app_ui_format_channel_pair(line, sizeof(line), 0u, crsf->us[0], 1u, crsf->us[1]);
    (void)task_oled_ui_set_line(1u, line);

    app_ui_format_channel_pair(line, sizeof(line), 2u, crsf->us[2], 3u, crsf->us[3]);
    (void)task_oled_ui_set_line(2u, line);

    app_ui_format_channel_pair(line, sizeof(line), 4u, crsf->us[4], 5u, crsf->us[5]);
    (void)task_oled_ui_set_line(3u, line);

    app_ui_format_channel_pair(line, sizeof(line), 6u, crsf->us[6], 7u, crsf->us[7]);
    (void)task_oled_ui_set_line(4u, line);

    (void)task_oled_ui_clear_line(5u);
    (void)task_oled_ui_clear_line(6u);
    (void)task_oled_ui_clear_line(7u);

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
        app_ui_on_enter_page(s_active_page);
        s_last_drawn_page = s_active_page;
    }

    app_ui_draw_page_title(s_active_page);

    switch (s_active_page)
    {
        case TASK_APP_UI_PAGE_1:
            app_ui_draw_page_1();
            break;

        case TASK_APP_UI_PAGE_2:
            app_ui_draw_page_2();
            break;

        case TASK_APP_UI_PAGE_3:
            app_ui_draw_page_3();
            break;

        case TASK_APP_UI_PAGE_4:
            app_ui_draw_page_imu();
            break;

        case TASK_APP_UI_PAGE_5:
            app_ui_draw_page_crsf(crsf);
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