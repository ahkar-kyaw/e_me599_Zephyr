#ifndef BOARD_LED_H
#define BOARD_LED_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BOARD_LED_DEBUG1 = 0,
    BOARD_LED_COUNT
} board_led_id_t;

void board_led_init(void);
void board_led_write(board_led_id_t led, bool on);
void board_led_toggle(board_led_id_t led);
void board_led_all_off(void);

#ifdef __cplusplus
}
#endif

#endif