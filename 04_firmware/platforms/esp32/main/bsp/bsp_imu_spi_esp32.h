#ifndef BSP_IMU_SPI_ESP32_H
#define BSP_IMU_SPI_ESP32_H

#include "interfaces/if_spi.h"

#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    spi_device_handle_t handle;
    if_spi_t spi;
} bsp_imu_spi_esp32_t;

int bsp_imu_spi_esp32_init(bsp_imu_spi_esp32_t *bus);

#ifdef __cplusplus
}
#endif

#endif