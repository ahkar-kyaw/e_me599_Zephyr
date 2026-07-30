#ifndef BSP_DISPLAY_SPI_ESP32_H
#define BSP_DISPLAY_SPI_ESP32_H

#include "interfaces/if_display_io.h"

#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    spi_device_handle_t handle;
    if_display_io_t io;
} bsp_display_spi_esp32_t;

int bsp_display_spi_esp32_init(bsp_display_spi_esp32_t *display_io);

#ifdef __cplusplus
}
#endif

#endif
