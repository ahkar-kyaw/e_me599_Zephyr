#include "board_led.h"

#include "board_esp32_nodemcu_v1.h"

#include "driver/gpio.h"
#include "esp_err.h"

typedef struct
{
    gpio_num_t gpio;
    uint8_t active_level;
    uint8_t inactive_level;
} board_led_gpio_t;

static const board_led_gpio_t g_board_leds[BOARD_LED_COUNT] =
{
    [BOARD_LED_DEBUG1] =
    {
        .gpio = BOARD_DEBUG_LED1_GPIO,
        .active_level = BOARD_DEBUG_LED_ACTIVE_LEVEL,
        .inactive_level = BOARD_DEBUG_LED_INACTIVE_LEVEL,
    },
};

void board_led_init(void)
{
    gpio_config_t io_conf =
    {
        .pin_bit_mask = (1ULL << BOARD_DEBUG_LED1_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    board_led_all_off();
}

void board_led_write(board_led_id_t led, bool on)
{
    if (led >= BOARD_LED_COUNT)
    {
        return;
    }

    const board_led_gpio_t *led_gpio = &g_board_leds[led];

    uint8_t level = on ? led_gpio->active_level : led_gpio->inactive_level;

    ESP_ERROR_CHECK(gpio_set_level(led_gpio->gpio, level));
}

void board_led_toggle(board_led_id_t led)
{
    static bool led_state[BOARD_LED_COUNT];

    if (led >= BOARD_LED_COUNT)
    {
        return;
    }

    led_state[led] = !led_state[led];

    board_led_write(led, led_state[led]);
}

void board_led_all_off(void)
{
    for (uint32_t i = 0u; i < (uint32_t)BOARD_LED_COUNT; i++)
    {
        board_led_write((board_led_id_t)i, false);
    }
}