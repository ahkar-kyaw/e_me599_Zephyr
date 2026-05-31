#include "test_crsf_oled.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "task_crsf.h"
#include "task_oled_ui.h"

#define TEST_CRSF_OLED_STACK_BYTES        2048u
#define TEST_CRSF_OLED_PERIOD_MS          200u

static osThreadId_t s_test_crsf_oled_handle = NULL;

static StaticTask_t s_test_crsf_oled_cb;
static uint64_t s_test_crsf_oled_stack[TEST_CRSF_OLED_STACK_BYTES / sizeof(uint64_t)];

static void test_crsf_oled_thread(void *argument)
{
    (void)argument;

    (void)task_oled_ui_select_page(TASK_OLED_UI_PAGE_LINES);
    (void)task_oled_ui_clear();

    for (;;)
    {
        task_crsf_snapshot_t crsf;
        char line[24];

        if (!task_crsf_get_snapshot(&crsf))
        {
            (void)task_oled_ui_set_line(0u, "CRSF SNAP FAIL");
            osDelay(TEST_CRSF_OLED_PERIOD_MS);
            continue;
        }

        if (!crsf.driver_started)
        {
            (void)task_oled_ui_set_line(0u, "CRSF DMA FAIL");
            (void)task_oled_ui_set_line(1u, "CHECK USART2");
            (void)task_oled_ui_set_line(2u, "PD5 TX PD6 RX");
            (void)task_oled_ui_set_line(3u, "");
            (void)task_oled_ui_set_line(4u, "");
            (void)task_oled_ui_set_line(5u, "");
            (void)task_oled_ui_set_line(6u, "");
            (void)task_oled_ui_set_line(7u, "");
            osDelay(TEST_CRSF_OLED_PERIOD_MS);
            continue;
        }

        if (crsf.receiver_connected && crsf.channels_valid)
        {
            (void)task_oled_ui_set_line(0u, "CRSF LINK OK");
        }
        else
        {
            (void)task_oled_ui_set_line(0u, "CRSF LINK LOST");
        }

        (void)snprintf(
            line,
            sizeof(line),
            "AGE %lu ms",
            (unsigned long)crsf.rc_age_ms);
        (void)task_oled_ui_set_line(1u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "CH1 %4u %4uus",
            crsf.raw[0],
            crsf.us[0]);
        (void)task_oled_ui_set_line(2u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "CH2 %4u %4uus",
            crsf.raw[1],
            crsf.us[1]);
        (void)task_oled_ui_set_line(3u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "CH3 %4u %4uus",
            crsf.raw[2],
            crsf.us[2]);
        (void)task_oled_ui_set_line(4u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "CH4 %4u %4uus",
            crsf.raw[3],
            crsf.us[3]);
        (void)task_oled_ui_set_line(5u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "RC%lu CRC%lu",
            (unsigned long)crsf.valid_rc_frame_count,
            (unsigned long)crsf.crc_error_count);
        (void)task_oled_ui_set_line(6u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "LEN%lu UART%lu",
            (unsigned long)crsf.length_error_count,
            (unsigned long)crsf.uart_error_count);
        (void)task_oled_ui_set_line(7u, line);

        osDelay(TEST_CRSF_OLED_PERIOD_MS);
    }
}

void test_crsf_oled_start(void)
{
    if (s_test_crsf_oled_handle != NULL)
    {
        return;
    }

    static const osThreadAttr_t task_attributes =
    {
        .name = "test_crsf_oled",
        .attr_bits = 0u,
        .cb_mem = &s_test_crsf_oled_cb,
        .cb_size = sizeof(s_test_crsf_oled_cb),
        .stack_mem = s_test_crsf_oled_stack,
        .stack_size = sizeof(s_test_crsf_oled_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_test_crsf_oled_handle =
        osThreadNew(test_crsf_oled_thread, NULL, &task_attributes);
}
