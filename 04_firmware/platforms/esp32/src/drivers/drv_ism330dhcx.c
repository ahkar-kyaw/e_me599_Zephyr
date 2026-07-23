#include "drv_ism330dhcx.h"

#include "board_esp32_nodemcu_v1.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stddef.h>
#include <string.h>

#define ISM330DHCX_REG_WHO_AM_I       0x0Fu
#define ISM330DHCX_REG_CTRL1_XL       0x10u
#define ISM330DHCX_REG_CTRL2_G        0x11u
#define ISM330DHCX_REG_CTRL3_C        0x12u
#define ISM330DHCX_REG_STATUS_REG     0x1Eu
#define ISM330DHCX_REG_OUT_TEMP_L     0x20u

#define ISM330DHCX_SPI_READ_BIT       0x80u

#define ISM330DHCX_CTRL3_SW_RESET     0x01u
#define ISM330DHCX_CTRL3_IF_INC       0x04u
#define ISM330DHCX_CTRL3_BDU          0x40u

#define ISM330DHCX_STATUS_XLDA        0x01u
#define ISM330DHCX_STATUS_GDA         0x02u
#define ISM330DHCX_STATUS_TDA         0x04u

#define ISM330DHCX_SPI_MAX_TRANSFER   32u

#define STANDARD_GRAVITY_MPS2         9.80665f

static const char *TAG = "drv_ism330dhcx";

static int16_t le_i16(uint8_t lo, uint8_t hi)
{
    return (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
}

static float accel_mg_per_lsb(drv_ism330dhcx_accel_fs_t fs)
{
    switch (fs)
    {
        case DRV_ISM330DHCX_ACCEL_FS_2G:
            return 0.061f;

        case DRV_ISM330DHCX_ACCEL_FS_4G:
            return 0.122f;

        case DRV_ISM330DHCX_ACCEL_FS_8G:
            return 0.244f;

        case DRV_ISM330DHCX_ACCEL_FS_16G:
            return 0.488f;

        default:
            return 0.061f;
    }
}

static float gyro_mdps_per_lsb(drv_ism330dhcx_gyro_fs_t fs)
{
    switch (fs)
    {
        case DRV_ISM330DHCX_GYRO_FS_250_DPS:
            return 8.75f;

        case DRV_ISM330DHCX_GYRO_FS_500_DPS:
            return 17.50f;

        case DRV_ISM330DHCX_GYRO_FS_1000_DPS:
            return 35.00f;

        case DRV_ISM330DHCX_GYRO_FS_2000_DPS:
            return 70.00f;

        default:
            return 8.75f;
    }
}

static esp_err_t drv_ism330dhcx_read_regs(drv_ism330dhcx_t *dev,
                                          uint8_t reg,
                                          uint8_t *data,
                                          size_t length)
{
    if ((dev == NULL) || (dev->spi == NULL) || (data == NULL) ||
        (length == 0u) || (length > (ISM330DHCX_SPI_MAX_TRANSFER - 1u)))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx_buf[ISM330DHCX_SPI_MAX_TRANSFER] = {0};
    uint8_t rx_buf[ISM330DHCX_SPI_MAX_TRANSFER] = {0};

    tx_buf[0] = reg | ISM330DHCX_SPI_READ_BIT;

    spi_transaction_t transaction =
    {
        .length = 8u * (length + 1u),
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t err = spi_device_transmit(dev->spi, &transaction);
    if (err != ESP_OK)
    {
        return err;
    }

    memcpy(data, &rx_buf[1], length);

    return ESP_OK;
}

static esp_err_t drv_ism330dhcx_write_reg(drv_ism330dhcx_t *dev,
                                          uint8_t reg,
                                          uint8_t value)
{
    if ((dev == NULL) || (dev->spi == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t tx_buf[2] =
    {
        (uint8_t)(reg & ~ISM330DHCX_SPI_READ_BIT),
        value,
    };

    spi_transaction_t transaction =
    {
        .length = 16u,
        .tx_buffer = tx_buf,
    };

    return spi_device_transmit(dev->spi, &transaction);
}

drv_ism330dhcx_config_t drv_ism330dhcx_default_config(void)
{
    drv_ism330dhcx_config_t config =
    {
        .accel_odr = DRV_ISM330DHCX_ODR_208_HZ,
        .gyro_odr = DRV_ISM330DHCX_ODR_208_HZ,
        .accel_fs = DRV_ISM330DHCX_ACCEL_FS_4G,
        .gyro_fs = DRV_ISM330DHCX_GYRO_FS_500_DPS,
    };

    return config;
}

esp_err_t drv_ism330dhcx_bus_init(void)
{
    spi_bus_config_t bus_config =
    {
        .mosi_io_num = BOARD_IMU_SPI_MOSI_GPIO,
        .miso_io_num = BOARD_IMU_SPI_MISO_GPIO,
        .sclk_io_num = BOARD_IMU_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ISM330DHCX_SPI_MAX_TRANSFER,
    };

    esp_err_t err = spi_bus_initialize(BOARD_IMU_SPI_HOST,
                                       &bus_config,
                                       SPI_DMA_CH_AUTO);

    if (err == ESP_ERR_INVALID_STATE)
    {
        return ESP_OK;
    }

    return err;
}

esp_err_t drv_ism330dhcx_add_device(drv_ism330dhcx_t *dev)
{
    if (dev == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));

    spi_device_interface_config_t device_config =
    {
        .clock_speed_hz = BOARD_IMU_SPI_CLOCK_HZ,
        .mode = BOARD_IMU_SPI_MODE,
        .spics_io_num = BOARD_IMU_SPI_CS_GPIO,
        .queue_size = 1,
    };

    return spi_bus_add_device(BOARD_IMU_SPI_HOST,
                              &device_config,
                              &dev->spi);
}

esp_err_t drv_ism330dhcx_read_who_am_i(drv_ism330dhcx_t *dev,
                                        uint8_t *who_am_i)
{
    if (who_am_i == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return drv_ism330dhcx_read_regs(dev,
                                    ISM330DHCX_REG_WHO_AM_I,
                                    who_am_i,
                                    1u);
}

esp_err_t drv_ism330dhcx_read_status(drv_ism330dhcx_t *dev,
                                      uint8_t *status_reg)
{
    if (status_reg == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    return drv_ism330dhcx_read_regs(dev,
                                    ISM330DHCX_REG_STATUS_REG,
                                    status_reg,
                                    1u);
}

esp_err_t drv_ism330dhcx_soft_reset(drv_ism330dhcx_t *dev)
{
    esp_err_t err = drv_ism330dhcx_write_reg(dev,
                                             ISM330DHCX_REG_CTRL3_C,
                                             ISM330DHCX_CTRL3_SW_RESET);
    if (err != ESP_OK)
    {
        return err;
    }

    for (uint32_t i = 0u; i < 50u; i++)
    {
        uint8_t ctrl3 = 0u;

        err = drv_ism330dhcx_read_regs(dev,
                                       ISM330DHCX_REG_CTRL3_C,
                                       &ctrl3,
                                       1u);
        if (err != ESP_OK)
        {
            return err;
        }

        if ((ctrl3 & ISM330DHCX_CTRL3_SW_RESET) == 0u)
        {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t drv_ism330dhcx_init(drv_ism330dhcx_t *dev,
                              const drv_ism330dhcx_config_t *config)
{
    if ((dev == NULL) || (config == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t who_am_i = 0u;

    esp_err_t err = drv_ism330dhcx_read_who_am_i(dev, &who_am_i);
    if (err != ESP_OK)
    {
        return err;
    }

    if (who_am_i != DRV_ISM330DHCX_WHO_AM_I_EXPECTED)
    {
        ESP_LOGE(TAG,
                 "unexpected WHO_AM_I=0x%02X expected=0x%02X",
                 who_am_i,
                 DRV_ISM330DHCX_WHO_AM_I_EXPECTED);
        return ESP_ERR_NOT_FOUND;
    }

    err = drv_ism330dhcx_soft_reset(dev);
    if (err != ESP_OK)
    {
        return err;
    }

    dev->config = *config;

    dev->accel_lsb_to_mps2 =
        (accel_mg_per_lsb(config->accel_fs) / 1000.0f) * STANDARD_GRAVITY_MPS2;

    dev->gyro_lsb_to_dps =
        gyro_mdps_per_lsb(config->gyro_fs) / 1000.0f;

    err = drv_ism330dhcx_write_reg(dev,
                                   ISM330DHCX_REG_CTRL3_C,
                                   ISM330DHCX_CTRL3_BDU |
                                   ISM330DHCX_CTRL3_IF_INC);
    if (err != ESP_OK)
    {
        return err;
    }

    uint8_t ctrl1_xl =
        (uint8_t)(((uint8_t)config->accel_odr << 4) |
                  ((uint8_t)config->accel_fs << 2));

    uint8_t ctrl2_g =
        (uint8_t)(((uint8_t)config->gyro_odr << 4) |
                  ((uint8_t)config->gyro_fs << 2));

    err = drv_ism330dhcx_write_reg(dev,
                                   ISM330DHCX_REG_CTRL1_XL,
                                   ctrl1_xl);
    if (err != ESP_OK)
    {
        return err;
    }

    err = drv_ism330dhcx_write_reg(dev,
                                   ISM330DHCX_REG_CTRL2_G,
                                   ctrl2_g);
    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(TAG,
             "ISM330DHCX initialized WHO_AM_I=0x%02X accel_odr=%u gyro_odr=%u",
             who_am_i,
             (unsigned int)config->accel_odr,
             (unsigned int)config->gyro_odr);

    return ESP_OK;
}

esp_err_t drv_ism330dhcx_read_sample(drv_ism330dhcx_t *dev,
                                      drv_ism330dhcx_sample_t *sample)
{
    if ((dev == NULL) || (sample == NULL))
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t status = 0u;
    esp_err_t err = drv_ism330dhcx_read_status(dev, &status);
    if (err != ESP_OK)
    {
        return err;
    }

    uint8_t raw[14] = {0};

    err = drv_ism330dhcx_read_regs(dev,
                                   ISM330DHCX_REG_OUT_TEMP_L,
                                   raw,
                                   sizeof(raw));
    if (err != ESP_OK)
    {
        return err;
    }

    memset(sample, 0, sizeof(*sample));

    sample->status_reg = status;
    sample->accel_data_ready = ((status & ISM330DHCX_STATUS_XLDA) != 0u);
    sample->gyro_data_ready = ((status & ISM330DHCX_STATUS_GDA) != 0u);
    sample->temp_data_ready = ((status & ISM330DHCX_STATUS_TDA) != 0u);

    sample->temp_raw = le_i16(raw[0], raw[1]);

    sample->gyro_raw[0] = le_i16(raw[2], raw[3]);
    sample->gyro_raw[1] = le_i16(raw[4], raw[5]);
    sample->gyro_raw[2] = le_i16(raw[6], raw[7]);

    sample->accel_raw[0] = le_i16(raw[8], raw[9]);
    sample->accel_raw[1] = le_i16(raw[10], raw[11]);
    sample->accel_raw[2] = le_i16(raw[12], raw[13]);

    for (uint32_t i = 0u; i < 3u; i++)
    {
        sample->accel_mps2[i] =
            (float)sample->accel_raw[i] * dev->accel_lsb_to_mps2;

        sample->gyro_dps[i] =
            (float)sample->gyro_raw[i] * dev->gyro_lsb_to_dps;
    }

    sample->temp_c = 25.0f + ((float)sample->temp_raw / 256.0f);

    return ESP_OK;
}