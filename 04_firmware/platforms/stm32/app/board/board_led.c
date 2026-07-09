#include "board_led.h"

#include "main.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} board_led_gpio_t;

#ifndef DBG_LED1_Pin
#error "DBG_LED1_Pin is not defined. Configure PB0 as GPIO_Output and label it DBG_LED1 in CubeMX."
#endif

#ifndef DBG_LED2_Pin
#error "DBG_LED2_Pin is not defined. Configure PE1 as GPIO_Output and label it DBG_LED2 in CubeMX."
#endif

#ifndef DBG_LED3_Pin
#error "DBG_LED3_Pin is not defined. Configure PB14 as GPIO_Output and label it DBG_LED3 in CubeMX."
#endif

static const board_led_gpio_t g_board_leds[BOARD_LED_COUNT] =
{
    [BOARD_LED_DEBUG1] = {DBG_LED1_GPIO_Port, DBG_LED1_Pin},
    [BOARD_LED_DEBUG2] = {DBG_LED2_GPIO_Port, DBG_LED2_Pin},
    [BOARD_LED_DEBUG3] = {DBG_LED3_GPIO_Port, DBG_LED3_Pin},
};

void board_led_init(void)
{
    board_led_all_off();
}

void board_led_write(board_led_id_t led, bool on)
{
    if (led >= BOARD_LED_COUNT)
    {
        return;
    }

    HAL_GPIO_WritePin(g_board_leds[led].port,
                      g_board_leds[led].pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void board_led_toggle(board_led_id_t led)
{
    if (led >= BOARD_LED_COUNT)
    {
        return;
    }

    HAL_GPIO_TogglePin(g_board_leds[led].port,
                       g_board_leds[led].pin);
}

void board_led_all_off(void)
{
    for (uint32_t i = 0u; i < (uint32_t)BOARD_LED_COUNT; i++)
    {
        board_led_write((board_led_id_t)i, false);
    }
}