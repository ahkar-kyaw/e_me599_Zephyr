#include "task_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "usart.h"

#define TASK_LOG_STACK_BYTES          2048u
#define TASK_LOG_QUEUE_DEPTH          16u
#define TASK_LOG_MESSAGE_MAX_LEN      160u
#define TASK_LOG_UART_TIMEOUT_MS      50u

typedef struct
{
    char text[TASK_LOG_MESSAGE_MAX_LEN];
} task_log_msg_t;

static osThreadId_t s_task_log_handle = NULL;
static osMessageQueueId_t s_task_log_queue = NULL;

static StaticTask_t s_task_log_cb;
static uint64_t s_task_log_stack[TASK_LOG_STACK_BYTES / sizeof(uint64_t)];

static StaticQueue_t s_task_log_queue_cb;
static uint64_t s_task_log_queue_mem[
    ((TASK_LOG_QUEUE_DEPTH * sizeof(task_log_msg_t)) + sizeof(uint64_t) - 1u) / sizeof(uint64_t)
];

static void task_log_thread(void *argument)
{
    (void)argument;

    for (;;)
    {
        task_log_msg_t msg;

        if (osMessageQueueGet(s_task_log_queue, &msg, NULL, osWaitForever) == osOK)
        {
            const size_t len = strlen(msg.text);

            if (len > 0u)
            {
                /*
                 * This blocks only the low-priority logging task.
                 * Producer tasks never block on UART.
                 */
                (void)HAL_UART_Transmit(
                    &huart3,
                    (uint8_t *)msg.text,
                    (uint16_t)len,
                    TASK_LOG_UART_TIMEOUT_MS);
            }
        }
    }
}

bool task_log_start(void)
{
    if (s_task_log_handle != NULL)
    {
        return true;
    }

    static const osMessageQueueAttr_t queue_attr =
    {
        .name = "log_q",
        .attr_bits = 0u,
        .cb_mem = &s_task_log_queue_cb,
        .cb_size = sizeof(s_task_log_queue_cb),
        .mq_mem = s_task_log_queue_mem,
        .mq_size = sizeof(s_task_log_queue_mem)
    };

    s_task_log_queue = osMessageQueueNew(
        TASK_LOG_QUEUE_DEPTH,
        sizeof(task_log_msg_t),
        &queue_attr);

    if (s_task_log_queue == NULL)
    {
        return false;
    }

    static const osThreadAttr_t thread_attr =
    {
        .name = "log",
        .attr_bits = 0u,
        .cb_mem = &s_task_log_cb,
        .cb_size = sizeof(s_task_log_cb),
        .stack_mem = s_task_log_stack,
        .stack_size = sizeof(s_task_log_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_task_log_handle = osThreadNew(task_log_thread, NULL, &thread_attr);

    return (s_task_log_handle != NULL);
}

bool task_log_write(const char *text)
{
    task_log_msg_t msg;

    if ((text == NULL) || (s_task_log_queue == NULL))
    {
        return false;
    }

    memset(&msg, 0, sizeof(msg));
    strncpy(msg.text, text, sizeof(msg.text) - 1u);
    msg.text[sizeof(msg.text) - 1u] = '\0';

    /*
     * Timeout 0 means producer tasks do not block.
     * If the log queue is full, this message is dropped.
     */
    return (osMessageQueuePut(s_task_log_queue, &msg, 0u, 0u) == osOK);
}

bool task_log_printf(const char *format, ...)
{
    task_log_msg_t msg;
    va_list args;

    if ((format == NULL) || (s_task_log_queue == NULL))
    {
        return false;
    }

    memset(&msg, 0, sizeof(msg));

    va_start(args, format);
    (void)vsnprintf(msg.text, sizeof(msg.text), format, args);
    va_end(args);

    msg.text[sizeof(msg.text) - 1u] = '\0';

    return (osMessageQueuePut(s_task_log_queue, &msg, 0u, 0u) == osOK);
}