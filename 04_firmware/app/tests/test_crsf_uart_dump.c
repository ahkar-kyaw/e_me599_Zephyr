#include "test_crsf_uart_dump.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "dma_buffers.h"
#include "usart.h"

#define TEST_CRSF_UART_DUMP_STACK_BYTES       2048u
#define TEST_CRSF_UART_DUMP_PERIOD_MS         100u
#define TEST_CRSF_UART_DUMP_STATUS_MS         1000u

#define TEST_CRSF_UART_DUMP_MAX_BYTES_PASS    96u
#define TEST_CRSF_UART_DUMP_BYTES_PER_LINE    16u

static osThreadId_t s_uart_dump_task_handle = NULL;

static StaticTask_t s_uart_dump_task_cb;
static uint64_t s_uart_dump_task_stack[TEST_CRSF_UART_DUMP_STACK_BYTES / sizeof(uint64_t)];

static uint16_t s_dma_read_index = 0u;
static uint32_t s_total_rx_bytes = 0u;
static uint32_t s_total_printed_bytes = 0u;
static uint32_t s_total_limited_bytes = 0u;

static void test_crsf_uart_dump_thread(void *argument);

static void debug_uart_write(const char *text)
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

static void debug_uart_printf(const char *format, ...)
{
    char buffer[160];
    va_list args;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    buffer[sizeof(buffer) - 1u] = '\0';

    debug_uart_write(buffer);
}

static uint16_t crsf_dma_write_index(void)
{
    if ((huart2.hdmarx == NULL) || (CRSF_RX_DMA_BUF_LEN == 0u))
    {
        return 0u;
    }

    const uint16_t remaining = (uint16_t)__HAL_DMA_GET_COUNTER(huart2.hdmarx);

    if (remaining > CRSF_RX_DMA_BUF_LEN)
    {
        return 0u;
    }

    return (uint16_t)(CRSF_RX_DMA_BUF_LEN - remaining);
}

static bool crsf_uart_dump_start_dma(void)
{
    (void)HAL_UART_AbortReceive(&huart2);

    s_dma_read_index = 0u;
    memset(crsf_rx_dma_buf, 0, CRSF_RX_DMA_BUF_LEN);

    if (HAL_UART_Receive_DMA(&huart2, crsf_rx_dma_buf, CRSF_RX_DMA_BUF_LEN) != HAL_OK)
    {
        return false;
    }

    if (huart2.hdmarx != NULL)
    {
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    }

    return true;
}

static void test_crsf_uart_dump_thread(void *argument)
{
    (void)argument;

    debug_uart_write("\r\nHELLO USART3 DEBUG UART\r\n");

    debug_uart_write("\r\n\r\n=== CRSF UART2 RAW DUMP TEST ===\r\n");
    debug_uart_write("USART2 RX DMA -> USART3 serial monitor\r\n");
    debug_uart_write("Do not run task_crsf during this test.\r\n\r\n");

    bool dma_ok = crsf_uart_dump_start_dma();

    if (!dma_ok)
    {
        debug_uart_write("ERROR: HAL_UART_Receive_DMA(&huart2) failed\r\n");
    }
    else
    {
        debug_uart_write("DMA started OK. Waiting for bytes...\r\n");
    }

    uint32_t last_status_tick = osKernelGetTickCount();

    for (;;)
    {
        if (!dma_ok)
        {
            osDelay(1000u);
            dma_ok = crsf_uart_dump_start_dma();

            if (dma_ok)
            {
                debug_uart_write("DMA restart OK\r\n");
            }

            continue;
        }

        if (huart2.RxState != HAL_UART_STATE_BUSY_RX)
        {
            debug_uart_write("WARN: USART2 RX not busy. Restarting DMA.\r\n");
            dma_ok = crsf_uart_dump_start_dma();
            osDelay(TEST_CRSF_UART_DUMP_PERIOD_MS);
            continue;
        }

        const uint16_t write_index = crsf_dma_write_index();
        uint32_t printed_this_pass = 0u;
        char line[96];
        size_t line_pos = 0u;
        uint8_t line_count = 0u;

        while ((s_dma_read_index != write_index) &&
               (printed_this_pass < TEST_CRSF_UART_DUMP_MAX_BYTES_PASS))
        {
            const uint8_t byte = crsf_rx_dma_buf[s_dma_read_index];

            s_dma_read_index++;

            if (s_dma_read_index >= CRSF_RX_DMA_BUF_LEN)
            {
                s_dma_read_index = 0u;
            }

            s_total_rx_bytes++;
            s_total_printed_bytes++;
            printed_this_pass++;

            if (line_count == 0u)
            {
                line_pos = 0u;
                line[0] = '\0';
            }

            if ((line_pos + 4u) < sizeof(line))
            {
                int written = snprintf(
                    &line[line_pos],
                    sizeof(line) - line_pos,
                    "%02X ",
                    byte);

                if (written > 0)
                {
                    line_pos += (size_t)written;
                }
            }

            line_count++;

            if (line_count >= TEST_CRSF_UART_DUMP_BYTES_PER_LINE)
            {
                debug_uart_printf("RX: %s\r\n", line);
                line_count = 0u;
            }
        }

        if (line_count > 0u)
        {
            debug_uart_printf("RX: %s\r\n", line);
        }

        while (s_dma_read_index != write_index)
        {
            s_dma_read_index++;

            if (s_dma_read_index >= CRSF_RX_DMA_BUF_LEN)
            {
                s_dma_read_index = 0u;
            }

            s_total_rx_bytes++;
            s_total_limited_bytes++;
        }

        const uint32_t now = osKernelGetTickCount();

        if ((now - last_status_tick) >= TEST_CRSF_UART_DUMP_STATUS_MS)
        {
            last_status_tick = now;

            debug_uart_printf(
                "STAT total=%lu printed=%lu limited=%lu rd=%u wr=%u rxstate=%lu err=0x%08lX\r\n",
                (unsigned long)s_total_rx_bytes,
                (unsigned long)s_total_printed_bytes,
                (unsigned long)s_total_limited_bytes,
                s_dma_read_index,
                write_index,
                (unsigned long)huart2.RxState,
                (unsigned long)HAL_UART_GetError(&huart2));
        }

        osDelay(TEST_CRSF_UART_DUMP_PERIOD_MS);
    }
}

void test_crsf_uart_dump_start(void)
{
    if (s_uart_dump_task_handle != NULL)
    {
        return;
    }

    static const osThreadAttr_t task_attributes =
    {
        .name = "crsf_uart_dump",
        .attr_bits = 0u,
        .cb_mem = &s_uart_dump_task_cb,
        .cb_size = sizeof(s_uart_dump_task_cb),
        .stack_mem = s_uart_dump_task_stack,
        .stack_size = sizeof(s_uart_dump_task_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_uart_dump_task_handle =
        osThreadNew(test_crsf_uart_dump_thread, NULL, &task_attributes);
}