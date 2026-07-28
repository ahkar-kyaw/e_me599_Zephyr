#ifndef BOARD_ESP32_NODEMCU_V1_H
#define BOARD_ESP32_NODEMCU_V1_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

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

/* IMU interrupt pins. Not in use yet. */
#define BOARD_IMU_INT1_GPIO              GPIO_NUM_34
#define BOARD_IMU_INT2_GPIO              GPIO_NUM_35

/* SSD1306 OLED I2C bus. */
#define BOARD_OLED_I2C_PORT              I2C_NUM_0
#define BOARD_OLED_I2C_SDA_GPIO          GPIO_NUM_21
#define BOARD_OLED_I2C_SCL_GPIO          GPIO_NUM_22
#define BOARD_OLED_I2C_CLOCK_HZ          400000u
#define BOARD_OLED_I2C_TIMEOUT_MS        100u

/* ExpressLRS receiver CRSF UART. */
#define BOARD_CRSF_UART_PORT             UART_NUM_2
#define BOARD_CRSF_UART_RX_GPIO          GPIO_NUM_16
#define BOARD_CRSF_UART_TX_GPIO          GPIO_NUM_17
#define BOARD_CRSF_UART_BAUDRATE         420000u
#define BOARD_CRSF_UART_RX_BUFFER_BYTES  512u

#endif
