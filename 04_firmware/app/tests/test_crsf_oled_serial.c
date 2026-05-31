#include "test_crsf_oled_serial.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "usart.h"

#include "main.h"
#include "task_crsf.h"
#include "task_oled_ui.h"

#define TEST_CRSF_OLED_SERIAL_STACK_BYTES     2048u
#define TEST_CRSF_OLED_SERIAL_PERIOD_MS       200u

static osThreadId_t s_test_task_handle = NULL;

static StaticTask_t s_test_task_cb;
static uint64_t s_test_task_stack[TEST_CRSF_OLED_SERIAL_STACK_BYTES / sizeof(uint64_t)];

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

static const char *ok_lost(bool value)
{
    return value ? "OK" : "LOST";
}

static void test_crsf_oled_serial_thread(void *argument)
{
    (void)argument;

    (void)task_oled_ui_select_page(TASK_OLED_UI_PAGE_LINES);
    (void)task_oled_ui_clear();

    serial_write("\r\n=== CRSF OLED SERIAL TEST START ===\r\n");

    for (;;)
    {
        task_crsf_snapshot_t crsf;
        char line[24];

        if (!task_crsf_get_snapshot(&crsf))
        {
            (void)task_oled_ui_set_line(0u, "CRSF SNAP FAIL");
            serial_write("CRSF snapshot read failed\r\n");
            osDelay(TEST_CRSF_OLED_SERIAL_PERIOD_MS);
            continue;
        }

        if (!crsf.driver_started)
        {
            (void)task_oled_ui_set_line(0u, "CRSF DMA FAIL");
            (void)task_oled_ui_set_line(1u, "CHECK USART2");
            (void)task_oled_ui_set_line(2u, "PD6 RX");
            (void)task_oled_ui_set_line(3u, "420000 8N1");
            (void)task_oled_ui_set_line(4u, "");
            (void)task_oled_ui_set_line(5u, "");
            (void)task_oled_ui_set_line(6u, "");
            (void)task_oled_ui_set_line(7u, "");

            serial_write("CRSF driver failed to start. Check USART2 DMA.\r\n");

            osDelay(TEST_CRSF_OLED_SERIAL_PERIOD_MS);
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
            "C1 %4u %4uus",
            crsf.raw[0],
            crsf.us[0]);
        (void)task_oled_ui_set_line(2u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "C2 %4u %4uus",
            crsf.raw[1],
            crsf.us[1]);
        (void)task_oled_ui_set_line(3u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "C3 %4u %4uus",
            crsf.raw[2],
            crsf.us[2]);
        (void)task_oled_ui_set_line(4u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "C4 %4u %4uus",
            crsf.raw[3],
            crsf.us[3]);
        (void)task_oled_ui_set_line(5u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "BY%lu LB%02X",
            (unsigned long)crsf.rx_byte_count,
            crsf.last_rx_byte);
        (void)task_oled_ui_set_line(6u, line);

        (void)snprintf(
            line,
            sizeof(line),
            "RC%lu CR%lu",
            (unsigned long)crsf.valid_rc_frame_count,
            (unsigned long)crsf.crc_error_count);
        (void)task_oled_ui_set_line(7u, line);

        serial_printf(
            "CRSF %s valid=%u age=%lu "
            "ch1=%u/%uus ch2=%u/%uus ch3=%u/%uus ch4=%u/%uus "
            "bytes=%lu last=0x%02X frames=%lu rc=%lu crc=%lu len=%lu uart=%lu dma=%lu rd=%u wr=%u\r\n",
            ok_lost(crsf.receiver_connected && crsf.channels_valid),
            crsf.channels_valid ? 1u : 0u,
            (unsigned long)crsf.rc_age_ms,
            crsf.raw[0],
            crsf.us[0],
            crsf.raw[1],
            crsf.us[1],
            crsf.raw[2],
            crsf.us[2],
            crsf.raw[3],
            crsf.us[3],
            (unsigned long)crsf.rx_byte_count,
            crsf.last_rx_byte,
            (unsigned long)crsf.valid_frame_count,
            (unsigned long)crsf.valid_rc_frame_count,
            (unsigned long)crsf.crc_error_count,
            (unsigned long)crsf.length_error_count,
            (unsigned long)crsf.uart_error_count,
            (unsigned long)crsf.dma_restart_count,
            crsf.dma_read_index,
            crsf.dma_write_index);

        osDelay(TEST_CRSF_OLED_SERIAL_PERIOD_MS);
    }
}

void test_crsf_oled_serial_start(void)
{
    if (s_test_task_handle != NULL)
    {
        return;
    }

    static const osThreadAttr_t task_attributes =
    {
        .name = "crsf_oled_serial",
        .attr_bits = 0u,
        .cb_mem = &s_test_task_cb,
        .cb_size = sizeof(s_test_task_cb),
        .stack_mem = s_test_task_stack,
        .stack_size = sizeof(s_test_task_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_test_task_handle =
        osThreadNew(test_crsf_oled_serial_thread, NULL, &task_attributes);
}