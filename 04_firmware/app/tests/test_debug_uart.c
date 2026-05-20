#include "test_debug_uart.h"

static UART_HandleTypeDef *s_debug_huart = NULL;

void test_debug_uart_init(UART_HandleTypeDef *huart)
{
    s_debug_huart = huart;
}

int __io_putchar(int ch)
{
    if (s_debug_huart != NULL)
    {
        const uint8_t byte = (uint8_t)ch;
        (void)HAL_UART_Transmit(s_debug_huart, (uint8_t *)&byte, 1u, 10u);
    }

    return ch;
}
