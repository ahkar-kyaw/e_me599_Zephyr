#include "bsp_display_spi_esp32.h"

#if defined(BOARD_ESP32_NODEMCU_V1)
#include "board_esp32_nodemcu_v1.h"
#elif defined(BOARD_ESP32S3_DEVKITC_V1)
#include "board_esp32s3_devkitc_v1.h"
#else
#error "No ESP32 board selected."
#endif

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#define BSP_DISPLAY_SPI_TRANSFER_CHUNK 4096u

static int bsp_display_spi_write_command(void *context,
                                         uint8_t command);
static int bsp_display_spi_write_data(void *context,
                                      const uint8_t *data,
                                      size_t length);
static void bsp_display_spi_set_reset(void *context, bool high);
static void bsp_display_spi_delay_ms(void *context, uint32_t delay_ms);
static int bsp_display_spi_transmit(bsp_display_spi_esp32_t *display_io,
                                    const uint8_t *data,
                                    size_t length);

int bsp_display_spi_esp32_init(bsp_display_spi_esp32_t *display_io)
{
    if (display_io == NULL)
    {
        return IF_DISPLAY_IO_ERR;
    }

    memset(display_io, 0, sizeof(*display_io));

    const gpio_config_t control_gpio_config =
    {
        .pin_bit_mask =
            (1ULL << BOARD_DISPLAY_DC_GPIO) |
            (1ULL << BOARD_DISPLAY_RESET_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&control_gpio_config) != ESP_OK)
    {
        return IF_DISPLAY_IO_ERR;
    }

    gpio_set_level(BOARD_DISPLAY_DC_GPIO, 0);
    gpio_set_level(BOARD_DISPLAY_RESET_GPIO, 1);

    const spi_bus_config_t bus_config =
    {
        .mosi_io_num = BOARD_DISPLAY_SPI_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = BOARD_DISPLAY_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BSP_DISPLAY_SPI_TRANSFER_CHUNK,
    };

    esp_err_t error = spi_bus_initialize(BOARD_DISPLAY_SPI_HOST,
                                         &bus_config,
                                         SPI_DMA_CH_AUTO);

    if ((error != ESP_OK) && (error != ESP_ERR_INVALID_STATE))
    {
        return IF_DISPLAY_IO_ERR;
    }

    const spi_device_interface_config_t device_config =
    {
        .clock_speed_hz = BOARD_DISPLAY_SPI_CLOCK_HZ,
        .mode = BOARD_DISPLAY_SPI_MODE,
        .spics_io_num = BOARD_DISPLAY_SPI_CS_GPIO,
        .queue_size = 1,
    };

    error = spi_bus_add_device(BOARD_DISPLAY_SPI_HOST,
                               &device_config,
                               &display_io->handle);

    if (error != ESP_OK)
    {
        return IF_DISPLAY_IO_ERR;
    }

    display_io->io.context = display_io;
    display_io->io.write_command = bsp_display_spi_write_command;
    display_io->io.write_data = bsp_display_spi_write_data;
    display_io->io.set_reset = bsp_display_spi_set_reset;
    display_io->io.delay_ms = bsp_display_spi_delay_ms;

    return IF_DISPLAY_IO_OK;
}

static int bsp_display_spi_write_command(void *context,
                                         uint8_t command)
{
    bsp_display_spi_esp32_t *display_io =
        (bsp_display_spi_esp32_t *)context;

    if (display_io == NULL)
    {
        return IF_DISPLAY_IO_ERR;
    }

    gpio_set_level(BOARD_DISPLAY_DC_GPIO, 0);
    return bsp_display_spi_transmit(display_io, &command, 1u);
}

static int bsp_display_spi_write_data(void *context,
                                      const uint8_t *data,
                                      size_t length)
{
    bsp_display_spi_esp32_t *display_io =
        (bsp_display_spi_esp32_t *)context;

    if ((display_io == NULL) || (data == NULL) || (length == 0u))
    {
        return IF_DISPLAY_IO_ERR;
    }

    gpio_set_level(BOARD_DISPLAY_DC_GPIO, 1);
    return bsp_display_spi_transmit(display_io, data, length);
}

static void bsp_display_spi_set_reset(void *context, bool high)
{
    (void)context;
    gpio_set_level(BOARD_DISPLAY_RESET_GPIO, high ? 1 : 0);
}

static void bsp_display_spi_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}

static int bsp_display_spi_transmit(bsp_display_spi_esp32_t *display_io,
                                    const uint8_t *data,
                                    size_t length)
{
    if ((display_io == NULL) || (display_io->handle == NULL) ||
        (data == NULL) || (length == 0u))
    {
        return IF_DISPLAY_IO_ERR;
    }

    size_t offset = 0u;

    while (offset < length)
    {
        size_t chunk = length - offset;

        if (chunk > BSP_DISPLAY_SPI_TRANSFER_CHUNK)
        {
            chunk = BSP_DISPLAY_SPI_TRANSFER_CHUNK;
        }

        spi_transaction_t transaction =
        {
            .length = chunk * 8u,
        };

        if (chunk <= sizeof(transaction.tx_data))
        {
            transaction.flags = SPI_TRANS_USE_TXDATA;
            memcpy(transaction.tx_data, &data[offset], chunk);
        }
        else
        {
            transaction.tx_buffer = &data[offset];
        }

        if (spi_device_transmit(display_io->handle, &transaction) != ESP_OK)
        {
            return IF_DISPLAY_IO_ERR;
        }

        offset += chunk;
    }

    return IF_DISPLAY_IO_OK;
}
