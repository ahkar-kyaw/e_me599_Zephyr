#include "bsp_crsf_uart_esp32.h"

#if defined(BOARD_ESP32_NODEMCU_V1)
#include "board_esp32_nodemcu_v1.h"
#else
#error "No ESP32 board selected."
#endif

#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>

int bsp_crsf_uart_esp32_init(bsp_crsf_uart_esp32_t *uart)
{
    if (uart == NULL)
    {
        return -1;
    }

    memset(uart, 0, sizeof(*uart));

    const uart_config_t config =
    {
        .baud_rate = BOARD_CRSF_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0u,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t error = uart_param_config(BOARD_CRSF_UART_PORT, &config);

    if (error != ESP_OK)
    {
        return -1;
    }

    error = uart_set_pin(BOARD_CRSF_UART_PORT,
                         BOARD_CRSF_UART_TX_GPIO,
                         BOARD_CRSF_UART_RX_GPIO,
                         UART_PIN_NO_CHANGE,
                         UART_PIN_NO_CHANGE);

    if (error != ESP_OK)
    {
        return -1;
    }

    error = uart_driver_install(BOARD_CRSF_UART_PORT,
                                BOARD_CRSF_UART_RX_BUFFER_BYTES,
                                0u,
                                0u,
                                NULL,
                                0);

    if (error != ESP_OK)
    {
        return -1;
    }

    uart->initialized = true;
    return 0;
}

int bsp_crsf_uart_esp32_read(bsp_crsf_uart_esp32_t *uart,
                             uint8_t *data,
                             size_t capacity,
                             uint32_t timeout_ms)
{
    if ((uart == NULL) || !uart->initialized ||
        (data == NULL) || (capacity == 0u))
    {
        return -1;
    }

    if (capacity > INT_MAX)
    {
        capacity = INT_MAX;
    }

    const int bytes_read =
        uart_read_bytes(BOARD_CRSF_UART_PORT,
                        data,
                        (uint32_t)capacity,
                        pdMS_TO_TICKS(timeout_ms));

    if (bytes_read < 0)
    {
        uart->read_error_count++;
    }

    return bytes_read;
}
