#include "test_crsf.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "dma_buffers.h"
#include "drv_crsf.h"
#include "test_debug_uart.h"
#include "usart.h"

#define TEST_CRSF_TASK_STACK_BYTES 2048u
#define TEST_CRSF_TASK_PERIOD_MS   2u
#define TEST_CRSF_PRINT_PERIOD_MS  500u

static osThreadId_t s_test_crsf_task_handle = NULL;

static void test_crsf_task(void *argument)
{
    (void)argument;

    test_debug_uart_init(&huart3);

    printf("\r\nCRSF test task starting\r\n");

    if (!drv_crsf_init(&huart2, crsf_rx_dma_buf, CRSF_RX_DMA_BUF_LEN))
    {
        printf("CRSF init failed\r\n");
    }
    else if (!drv_crsf_start())
    {
        printf("CRSF DMA start failed\r\n");
    }
    else
    {
        printf("CRSF DMA started\r\n");
    }

    uint32_t last_print_tick = osKernelGetTickCount();

    for (;;)
    {
        drv_crsf_process();

        const uint32_t now = osKernelGetTickCount();
        if ((now - last_print_tick) >= TEST_CRSF_PRINT_PERIOD_MS)
        {
            last_print_tick = now;

            drv_crsf_state_t state;
            drv_crsf_get_state(&state);

            printf("CRSF link=%u frames=%lu rc=%lu crc_err=%lu len_err=%lu uart_err=%lu ch1=%u ch2=%u ch3=%u ch4=%u\r\n",
                   state.receiver_connected ? 1u : 0u,
                   (unsigned long)state.valid_frame_count,
                   (unsigned long)state.valid_rc_frame_count,
                   (unsigned long)state.crc_error_count,
                   (unsigned long)state.length_error_count,
                   (unsigned long)state.uart_error_count,
                   state.channels.raw[0],
                   state.channels.raw[1],
                   state.channels.raw[2],
                   state.channels.raw[3]);
        }

        osDelay(TEST_CRSF_TASK_PERIOD_MS);
    }
}

void test_crsf_start(void)
{
    static const osThreadAttr_t task_attributes = {
        .name = "test_crsf",
        .stack_size = TEST_CRSF_TASK_STACK_BYTES,
        .priority = (osPriority_t)osPriorityLow
    };

    if (s_test_crsf_task_handle == NULL)
    {
        s_test_crsf_task_handle = osThreadNew(test_crsf_task, NULL, &task_attributes);
    }
}
