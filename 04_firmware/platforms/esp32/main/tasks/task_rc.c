#include "task_rc.h"

#include "bsp_crsf_uart_esp32.h"
#include "config_rc.h"
#include "protocols/proto_crsf.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stddef.h>
#include <string.h>

#define TASK_RC_STACK_BYTES  4096u
#define TASK_RC_PRIORITY     (tskIDLE_PRIORITY + 2)
#define TASK_RC_READ_BYTES   64u

static const char *TAG = "task_rc";

static rc_snapshot_t g_rc_snapshot;
static portMUX_TYPE g_rc_snapshot_lock = portMUX_INITIALIZER_UNLOCKED;

static void task_rc_entry(void *argument);
static void task_rc_publish_snapshot(const rc_snapshot_t *snapshot);
static void task_rc_publish_invalid(uint32_t uart_error_count);

void task_rc_start(void)
{
    const BaseType_t ok = xTaskCreate(
        task_rc_entry,
        "task_rc",
        TASK_RC_STACK_BYTES,
        NULL,
        TASK_RC_PRIORITY,
        NULL);

    if (ok != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create task");
    }
}

void task_rc_get_snapshot(rc_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    portENTER_CRITICAL(&g_rc_snapshot_lock);
    *snapshot = g_rc_snapshot;
    portEXIT_CRITICAL(&g_rc_snapshot_lock);
}

static void task_rc_publish_snapshot(const rc_snapshot_t *snapshot)
{
    portENTER_CRITICAL(&g_rc_snapshot_lock);
    g_rc_snapshot = *snapshot;
    portEXIT_CRITICAL(&g_rc_snapshot_lock);
}

static void task_rc_publish_invalid(uint32_t uart_error_count)
{
    rc_snapshot_t snapshot;

    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.timestamp_us = (uint64_t)esp_timer_get_time();
    snapshot.uart_error_count = uart_error_count;
    snapshot.valid = false;

    task_rc_publish_snapshot(&snapshot);
}

static void task_rc_entry(void *argument)
{
    (void)argument;

    _Static_assert(APP_RC_CHANNEL_COUNT == PROTO_CRSF_CHANNEL_COUNT,
                   "RC channel counts must match");

    bsp_crsf_uart_esp32_t uart;
    proto_crsf_parser_t parser;
    uint32_t uart_init_error_count = 0u;
    uint8_t rx_buffer[TASK_RC_READ_BYTES];

    task_rc_publish_invalid(0u);

    while (bsp_crsf_uart_esp32_init(&uart) != 0)
    {
        uart_init_error_count++;
        task_rc_publish_invalid(uart_init_error_count);
        ESP_LOGE(TAG, "CRSF UART init failed");
        vTaskDelay(pdMS_TO_TICKS(APP_RC_RETRY_MS));
    }

    proto_crsf_init(&parser);

    ESP_LOGI(TAG, "CRSF receiver task started");

    for (;;)
    {
        const int bytes_read =
            bsp_crsf_uart_esp32_read(&uart,
                                     rx_buffer,
                                     sizeof(rx_buffer),
                                     APP_RC_UART_READ_TIMEOUT_MS);

        if (bytes_read < 0)
        {
            task_rc_publish_invalid(
                uart_init_error_count + uart.read_error_count);
            vTaskDelay(pdMS_TO_TICKS(APP_RC_RETRY_MS));
            continue;
        }

        for (int i = 0; i < bytes_read; i++)
        {
            proto_crsf_channels_t channels;

            if (proto_crsf_parse_byte(&parser,
                                      rx_buffer[i],
                                      &channels))
            {
                rc_snapshot_t snapshot;

                memset(&snapshot, 0, sizeof(snapshot));
                memcpy(snapshot.channel,
                       channels.channel,
                       sizeof(snapshot.channel));

                snapshot.timestamp_us =
                    (uint64_t)esp_timer_get_time();
                snapshot.frame_count = parser.rc_frame_count;
                snapshot.crc_error_count = parser.crc_error_count;
                snapshot.parse_error_count = parser.parse_error_count;
                snapshot.uart_error_count =
                    uart_init_error_count + uart.read_error_count;
                snapshot.valid = true;

                task_rc_publish_snapshot(&snapshot);
            }
        }
    }
}
