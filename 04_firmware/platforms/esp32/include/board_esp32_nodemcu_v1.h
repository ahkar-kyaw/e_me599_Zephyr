#ifndef BOARD_ESP32_NODEMCU_V1_H
#define BOARD_ESP32_NODEMCU_V1_H

#include "driver/gpio.h"

#define BOARD_NAME          "ESP32 NodeMCU"
#define BOARD_MCU_NAME      "ESP32"
#define BOARD_PLATFORM_NAME "esp32"

/* Board debug LED. */
#define BOARD_DEBUG_LED1_GPIO            GPIO_NUM_2
#define BOARD_DEBUG_LED_ACTIVE_LEVEL     1
#define BOARD_DEBUG_LED_INACTIVE_LEVEL   0
#define BOARD_DEBUG_LED_PERIOD_MS        500u

#endif