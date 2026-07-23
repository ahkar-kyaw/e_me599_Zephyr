#include "bsp_imu_spi_esp32.h"

#if defined(BOARD_ESP32_NODEMCU_V1)
#include "board_esp32_nodemcu_v1.h"
#elif defined(BOARD_ESP32S3_DEVKITC_V1)
#include "board_esp32s3_devkitc_v1.h"
#else
#error "No ESP32 board selected."
#endif

#include "driver/spi_master.h"
#include "esp_err.h"

#include <string.h>

#define BSP_IMU_SPI_MAX_TRANSFER_SIZE 32

static int bsp_imu_spi_esp32_transfer(void *context,
                                      const uint8_t *tx,
                                      uint8_t *rx,
                                      size_t length)
{
    bsp_imu_spi_esp32_t *bus = (bsp_imu_spi_esp32_t *)context;

    if ((bus == 0) || (bus->handle == 0) || (tx == 0) || (rx == 0) ||
        (length == 0))
    {
        return IF_SPI_ERR;
    }

    spi_transaction_t transaction =
    {
        .length = length * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    esp_err_t err = spi_device_transmit(bus->handle, &transaction);

    return (err == ESP_OK) ? IF_SPI_OK : IF_SPI_ERR;
}

int bsp_imu_spi_esp32_init(bsp_imu_spi_esp32_t *bus)
{
    if (bus == 0)
    {
        return IF_SPI_ERR;
    }

    memset(bus, 0, sizeof(*bus));

    spi_bus_config_t bus_config =
    {
        .mosi_io_num = BOARD_IMU_SPI_MOSI_GPIO,
        .miso_io_num = BOARD_IMU_SPI_MISO_GPIO,
        .sclk_io_num = BOARD_IMU_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BSP_IMU_SPI_MAX_TRANSFER_SIZE,
    };

    esp_err_t err = spi_bus_initialize(BOARD_IMU_SPI_HOST,
                                       &bus_config,
                                       SPI_DMA_CH_AUTO);

    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE))
    {
        return IF_SPI_ERR;
    }

    spi_device_interface_config_t device_config =
    {
        .clock_speed_hz = BOARD_IMU_SPI_CLOCK_HZ,
        .mode = BOARD_IMU_SPI_MODE,
        .spics_io_num = BOARD_IMU_SPI_CS_GPIO,
        .queue_size = 1,
    };

    err = spi_bus_add_device(BOARD_IMU_SPI_HOST,
                             &device_config,
                             &bus->handle);

    if (err != ESP_OK)
    {
        return IF_SPI_ERR;
    }

    bus->spi.context = bus;
    bus->spi.transfer = bsp_imu_spi_esp32_transfer;

    return IF_SPI_OK;
}