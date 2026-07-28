#ifndef BSP_OLED_I2C_ESP32_H
#define BSP_OLED_I2C_ESP32_H

#include "interfaces/if_i2c.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    if_i2c_t i2c;
    bool initialized;
} bsp_oled_i2c_esp32_t;

int bsp_oled_i2c_esp32_init(bsp_oled_i2c_esp32_t *bus);

#ifdef __cplusplus
}
#endif

#endif
