#include "bsp_oled_i2c_esp32.h"

#if defined(BOARD_ESP32_NODEMCU_V1)
#include "board_esp32_nodemcu_v1.h"
#else
#error "No ESP32 board selected."
#endif

#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include <stddef.h>
#include <string.h>

static int bsp_oled_i2c_esp32_write(void *context,
                                    uint8_t address,
                                    const uint8_t *data,
                                    size_t length);

int bsp_oled_i2c_esp32_init(bsp_oled_i2c_esp32_t *bus)
{
    if (bus == NULL)
    {
        return IF_I2C_ERR;
    }

    memset(bus, 0, sizeof(*bus));

    const i2c_config_t config =
    {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_OLED_I2C_SDA_GPIO,
        .scl_io_num = BOARD_OLED_I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BOARD_OLED_I2C_CLOCK_HZ,
        .clk_flags = 0,
    };

    esp_err_t error = i2c_param_config(BOARD_OLED_I2C_PORT, &config);

    if (error != ESP_OK)
    {
        return IF_I2C_ERR;
    }

    error = i2c_driver_install(BOARD_OLED_I2C_PORT,
                               config.mode,
                               0u,
                               0u,
                               0);

    if ((error != ESP_OK) && (error != ESP_ERR_INVALID_STATE))
    {
        return IF_I2C_ERR;
    }

    bus->i2c.context = bus;
    bus->i2c.write = bsp_oled_i2c_esp32_write;
    bus->initialized = true;

    return IF_I2C_OK;
}

static int bsp_oled_i2c_esp32_write(void *context,
                                    uint8_t address,
                                    const uint8_t *data,
                                    size_t length)
{
    bsp_oled_i2c_esp32_t *bus = (bsp_oled_i2c_esp32_t *)context;

    if ((bus == NULL) || !bus->initialized ||
        (data == NULL) || (length == 0u))
    {
        return IF_I2C_ERR;
    }

    const esp_err_t error =
        i2c_master_write_to_device(BOARD_OLED_I2C_PORT,
                                   address,
                                   data,
                                   length,
                                   pdMS_TO_TICKS(
                                       BOARD_OLED_I2C_TIMEOUT_MS));

    return (error == ESP_OK) ? IF_I2C_OK : IF_I2C_ERR;
}
