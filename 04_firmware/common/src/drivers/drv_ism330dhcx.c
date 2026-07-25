#include "drivers/drv_ism330dhcx.h"

#include <stddef.h>
#include <stdint.h>

#define ISM330DHCX_REG_WHO_AM_I       0x0F
#define ISM330DHCX_REG_CTRL1_XL       0x10
#define ISM330DHCX_REG_CTRL2_G        0x11
#define ISM330DHCX_REG_CTRL3_C        0x12
#define ISM330DHCX_REG_OUT_TEMP_L     0x20

#define ISM330DHCX_SPI_READ_BIT       0x80

#define ISM330DHCX_CTRL3_IF_INC       0x04
#define ISM330DHCX_CTRL3_BDU          0x40

#define ISM330DHCX_CTRL1_XL_208HZ_4G  0x58
#define ISM330DHCX_CTRL2_G_208HZ_500  0x54

#define ISM330DHCX_RAW_BLOCK_LEN      14

#define ISM330DHCX_ACCEL_MPS2_PER_LSB 0.0011964113f
#define ISM330DHCX_GYRO_RPS_PER_LSB   0.0003054326f
#define ISM330DHCX_TEMP_C_PER_LSB     0.00390625f
#define ISM330DHCX_TEMP_C_OFFSET      25.0f

static int16_t drv_i16_from_le(uint8_t lo, uint8_t hi)
{
    return (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
}

static int drv_read_regs(drv_ism330dhcx_t *dev,
                         uint8_t reg,
                         uint8_t *data,
                         size_t length)
{
    if ((dev == 0) ||
        (dev->spi == 0) ||
        (dev->spi->transfer == 0) ||
        (data == 0) ||
        (length == 0) ||
        (length > ISM330DHCX_RAW_BLOCK_LEN))
    {
        return DRV_ISM330DHCX_ERR_ARG;
    }

    uint8_t tx[ISM330DHCX_RAW_BLOCK_LEN + 1] = {0};
    uint8_t rx[ISM330DHCX_RAW_BLOCK_LEN + 1] = {0};

    tx[0] = reg | ISM330DHCX_SPI_READ_BIT;

    if (dev->spi->transfer(dev->spi->context, tx, rx, length + 1) != IF_SPI_OK)
    {
        return DRV_ISM330DHCX_ERR_IO;
    }

    for (size_t i = 0; i < length; i++)
    {
        data[i] = rx[i + 1];
    }

    return DRV_ISM330DHCX_OK;
}

static int drv_write_reg(drv_ism330dhcx_t *dev, uint8_t reg, uint8_t value)
{
    if ((dev == 0) ||
        (dev->spi == 0) ||
        (dev->spi->transfer == 0))
    {
        return DRV_ISM330DHCX_ERR_ARG;
    }

    uint8_t tx[2] = {reg, value};
    uint8_t rx[2] = {0};

    if (dev->spi->transfer(dev->spi->context, tx, rx, sizeof(tx)) != IF_SPI_OK)
    {
        return DRV_ISM330DHCX_ERR_IO;
    }

    return DRV_ISM330DHCX_OK;
}

int drv_ism330dhcx_read_who_am_i(drv_ism330dhcx_t *dev, uint8_t *who_am_i)
{
    if (who_am_i == 0)
    {
        return DRV_ISM330DHCX_ERR_ARG;
    }

    return drv_read_regs(dev, ISM330DHCX_REG_WHO_AM_I, who_am_i, 1);
}

int drv_ism330dhcx_init(drv_ism330dhcx_t *dev, if_spi_t *spi)
{
    if ((dev == 0) || (spi == 0))
    {
        return DRV_ISM330DHCX_ERR_ARG;
    }

    dev->spi = spi;

    uint8_t who_am_i = 0;
    int err = drv_ism330dhcx_read_who_am_i(dev, &who_am_i);

    if (err != DRV_ISM330DHCX_OK)
    {
        return err;
    }

    if (who_am_i != DRV_ISM330DHCX_WHO_AM_I_EXPECTED)
    {
        return DRV_ISM330DHCX_ERR_ID;
    }

    err = drv_write_reg(dev,
                        ISM330DHCX_REG_CTRL3_C,
                        ISM330DHCX_CTRL3_BDU | ISM330DHCX_CTRL3_IF_INC);
    if (err != DRV_ISM330DHCX_OK)
    {
        return err;
    }

    err = drv_write_reg(dev,
                        ISM330DHCX_REG_CTRL1_XL,
                        ISM330DHCX_CTRL1_XL_208HZ_4G);
    if (err != DRV_ISM330DHCX_OK)
    {
        return err;
    }

    err = drv_write_reg(dev,
                        ISM330DHCX_REG_CTRL2_G,
                        ISM330DHCX_CTRL2_G_208HZ_500);
    if (err != DRV_ISM330DHCX_OK)
    {
        return err;
    }

    return DRV_ISM330DHCX_OK;
}

int drv_ism330dhcx_read_raw(drv_ism330dhcx_t *dev, drv_ism330dhcx_raw_t *raw)
{
    if (raw == 0)
    {
        return DRV_ISM330DHCX_ERR_ARG;
    }

    uint8_t data[ISM330DHCX_RAW_BLOCK_LEN] = {0};

    int err = drv_read_regs(dev,
                            ISM330DHCX_REG_OUT_TEMP_L,
                            data,
                            sizeof(data));
    if (err != DRV_ISM330DHCX_OK)
    {
        return err;
    }

    raw->temp_raw = drv_i16_from_le(data[0], data[1]);

    raw->gyro_raw[0] = drv_i16_from_le(data[2], data[3]);
    raw->gyro_raw[1] = drv_i16_from_le(data[4], data[5]);
    raw->gyro_raw[2] = drv_i16_from_le(data[6], data[7]);

    raw->accel_raw[0] = drv_i16_from_le(data[8], data[9]);
    raw->accel_raw[1] = drv_i16_from_le(data[10], data[11]);
    raw->accel_raw[2] = drv_i16_from_le(data[12], data[13]);

    return DRV_ISM330DHCX_OK;
}

void drv_ism330dhcx_convert_raw(const drv_ism330dhcx_raw_t *raw,
                                drv_ism330dhcx_data_t *data)
{
    if ((raw == 0) || (data == 0))
    {
        return;
    }

    for (uint32_t i = 0; i < 3; i++)
    {
        data->accel_mps2[i] = (float)raw->accel_raw[i] *
                              ISM330DHCX_ACCEL_MPS2_PER_LSB;

        data->gyro_rps[i] = (float)raw->gyro_raw[i] *
                            ISM330DHCX_GYRO_RPS_PER_LSB;
    }

    data->temp_c = ISM330DHCX_TEMP_C_OFFSET +
                   ((float)raw->temp_raw * ISM330DHCX_TEMP_C_PER_LSB);
}