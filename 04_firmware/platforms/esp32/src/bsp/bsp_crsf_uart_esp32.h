#ifndef BSP_CRSF_UART_ESP32_H
#define BSP_CRSF_UART_ESP32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t read_error_count;
    bool initialized;
} bsp_crsf_uart_esp32_t;

int bsp_crsf_uart_esp32_init(bsp_crsf_uart_esp32_t *uart);

int bsp_crsf_uart_esp32_read(bsp_crsf_uart_esp32_t *uart,
                             uint8_t *data,
                             size_t capacity,
                             uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
