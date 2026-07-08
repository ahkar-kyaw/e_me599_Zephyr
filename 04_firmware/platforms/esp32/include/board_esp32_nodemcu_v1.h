#ifndef BOARD_ESP32_NODEMCU_V1_H
#define BOARD_ESP32_NODEMCU_V1_H

#include "driver/gpio.h"

/*
 * Many ESP32 NodeMCU and DevKit boards connect the onboard LED to GPIO2.
 * If your board uses a different LED pin, change only this file.
 * If the LED is inverted, swap ACTIVE_LEVEL and INACTIVE_LEVEL.
 */
#define BOARD_LED_GPIO             GPIO_NUM_2
#define BOARD_LED_ACTIVE_LEVEL     1
#define BOARD_LED_INACTIVE_LEVEL   0
#define BOARD_LED_PERIOD_MS        500u

#endif