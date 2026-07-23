#ifndef BOARD_ESP32_NODEMCU_V1_H
#define BOARD_ESP32_NODEMCU_V1_H

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define BOARD_NAME          "ESP32 NodeMCU"
#define BOARD_MCU_NAME      "ESP32"
#define BOARD_PLATFORM_NAME "esp32"

/* Board debug LED. */
#define BOARD_DEBUG_LED1_GPIO            GPIO_NUM_2
#define BOARD_DEBUG_LED_ACTIVE_LEVEL     1
#define BOARD_DEBUG_LED_INACTIVE_LEVEL   0
#define BOARD_DEBUG_LED_PERIOD_MS        500u

/* ISM330DHCX IMU SPI bus. */
#define BOARD_IMU_SPI_HOST               SPI2_HOST
#define BOARD_IMU_SPI_SCLK_GPIO          GPIO_NUM_18
#define BOARD_IMU_SPI_MISO_GPIO          GPIO_NUM_19
#define BOARD_IMU_SPI_MOSI_GPIO          GPIO_NUM_23
#define BOARD_IMU_SPI_CS_GPIO            GPIO_NUM_27
#define BOARD_IMU_SPI_CLOCK_HZ           5000000
#define BOARD_IMU_SPI_MODE               3

/* Optional IMU interrupt pins. Not used by the polling test yet. */
#define BOARD_IMU_INT1_GPIO              GPIO_NUM_34
#define BOARD_IMU_INT2_GPIO              GPIO_NUM_35

#endif