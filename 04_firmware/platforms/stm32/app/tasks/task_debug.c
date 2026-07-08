#include "task_debug.h"

#include "cmsis_os2.h"
#include "config_app.h"
#include "usart.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static osThreadId_t task_debug_handle;

static void task_debug_entry(void *argument);
static void debug_uart_write(const char *text);

void task_debug_start(void)
{
    static const osThreadAttr_t task_debug_attributes =
    {
        .name = "task_debug",
        .stack_size = APP_DEBUG_TASK_STACK_BYTES,
        .priority = APP_DEBUG_TASK_PRIORITY,
    };

    task_debug_handle = osThreadNew(task_debug_entry, NULL, &task_debug_attributes);

    (void)task_debug_handle;
}

static void task_debug_entry(void *argument)
{
    (void)argument;

    uint32_t counter = 0u;
    char line[96];

    debug_uart_write("\r\n[boot] task_debug started\r\n");

    for (;;)
    {
        counter++;

        int length = snprintf(line,
                              sizeof(line),
                              "[debug] tick=%06lu\r\n",
                              (unsigned long)counter);

        if ((length > 0) && (length < (int)sizeof(line)))
        {
            debug_uart_write(line);
        }

        osDelay(APP_DEBUG_TASK_PERIOD_MS);
    }
}

static void debug_uart_write(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    size_t length = strlen(text);

    if (length == 0u)
    {
        return;
    }

    HAL_UART_Transmit(&huart3,
                      (uint8_t *)text,
                      (uint16_t)length,
                      100u);
}