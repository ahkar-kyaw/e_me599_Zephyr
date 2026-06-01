#include "drv_ism330dhcx.h"

#include <string.h>

#define ISM330DHCX_REG_FUNC_CFG_ACCESS      0x01u
#define ISM330DHCX_REG_WHO_AM_I             0x0Fu
#define ISM330DHCX_REG_CTRL1_XL             0x10u
#define ISM330DHCX_REG_CTRL2_G              0x11u
#define ISM330DHCX_REG_CTRL3_C              0x12u
#define ISM330DHCX_REG_STATUS_REG           0x1Eu

#define ISM330DHCX_REG_OUTX_L_G             0x22u
#define ISM330DHCX_REG_OUTX_H_G             0x23u
#define ISM330DHCX_REG_OUTY_L_G             0x24u
#define ISM330DHCX_REG_OUTY_H_G             0x25u
#define ISM330DHCX_REG_OUTZ_L_G             0x26u
#define ISM330DHCX_REG_OUTZ_H_G             0x27u
#define ISM330DHCX_REG_OUTX_L_A             0x28u
#define ISM330DHCX_REG_OUTX_H_A             0x29u
#define ISM330DHCX_REG_OUTY_L_A             0x2Au
#define ISM330DHCX_REG_OUTY_H_A             0x2Bu
#define ISM330DHCX_REG_OUTZ_L_A             0x2Cu
#define ISM330DHCX_REG_OUTZ_H_A             0x2Du

#define ISM330DHCX_SPI_READ_BIT             0x80u
#define ISM330DHCX_SPI_WRITE_MASK           0x7Fu

#define ISM330DHCX_CTRL3_C_BDU              0x40u
#define ISM330DHCX_CTRL3_C_IF_INC           0x04u
#define ISM330DHCX_CTRL3_C_SW_RESET         0x01u

/*
 * CTRL1_XL:
 * ODR_XL = 104 Hz
 * FS_XL  = +/-2 g
 *
 * CTRL2_G:
 * ODR_G = 104 Hz
 * FS_G  = +/-250 dps
 */
#define ISM330DHCX_CTRL1_XL_104HZ_2G        0x40u
#define ISM330DHCX_CTRL2_G_104HZ_250DPS     0x40u

#define ISM330DHCX_DEFAULT_TIMEOUT_MS       10u
#define ISM330DHCX_SPI_MAX_TRANSFER_LEN     16u

static bool drv_ism330dhcx_valid_dev(const drv_ism330dhcx_t *dev)
{
    return (dev != NULL) &&
           (dev->hspi != NULL) &&
           (dev->cs_gpio != NULL) &&
           (dev->cs_pin != 0u);
}

static void drv_ism330dhcx_cs_low(drv_ism330dhcx_t *dev)
{
    HAL_GPIO_WritePin(dev->cs_gpio, dev->cs_pin, GPIO_PIN_RESET);
}

static void drv_ism330dhcx_cs_high(drv_ism330dhcx_t *dev)
{
    HAL_GPIO_WritePin(dev->cs_gpio, dev->cs_pin, GPIO_PIN_SET);
}

static void drv_ism330dhcx_cs_delay(void)
{
    for (volatile uint32_t i = 0u; i < 100u; i++)
    {
        __NOP();
    }
}

static drv_ism330dhcx_result_t drv_ism330dhcx_read_regs(
    drv_ism330dhcx_t *dev,
    uint8_t start_reg,
    uint8_t *rx,
    uint16_t len)
{
    uint8_t tx_buf[1u + ISM330DHCX_SPI_MAX_TRANSFER_LEN];
    uint8_t rx_buf[1u + ISM330DHCX_SPI_MAX_TRANSFER_LEN];

    if (!drv_ism330dhcx_valid_dev(dev) || (rx == NULL) || (len == 0u))
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    if (len > ISM330DHCX_SPI_MAX_TRANSFER_LEN)
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    memset(tx_buf, 0, sizeof(tx_buf));
    memset(rx_buf, 0, sizeof(rx_buf));

    tx_buf[0] = (uint8_t)(start_reg | ISM330DHCX_SPI_READ_BIT);

    drv_ism330dhcx_cs_low(dev);
    drv_ism330dhcx_cs_delay();

    const HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(
            dev->hspi,
            tx_buf,
            rx_buf,
            (uint16_t)(len + 1u),
            dev->timeout_ms);

    drv_ism330dhcx_cs_delay();
    drv_ism330dhcx_cs_high(dev);

    if (status != HAL_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    memcpy(rx, &rx_buf[1], len);

    return DRV_ISM330DHCX_OK;
}

static drv_ism330dhcx_result_t drv_ism330dhcx_write_reg(
    drv_ism330dhcx_t *dev,
    uint8_t reg,
    uint8_t value)
{
    uint8_t tx[2];

    if (!drv_ism330dhcx_valid_dev(dev))
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    tx[0] = (uint8_t)(reg & ISM330DHCX_SPI_WRITE_MASK);
    tx[1] = value;

    drv_ism330dhcx_cs_low(dev);
    drv_ism330dhcx_cs_delay();

    const HAL_StatusTypeDef status =
        HAL_SPI_Transmit(dev->hspi, tx, sizeof(tx), dev->timeout_ms);

    drv_ism330dhcx_cs_delay();
    drv_ism330dhcx_cs_high(dev);

    if (status != HAL_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    return DRV_ISM330DHCX_OK;
}

drv_ism330dhcx_result_t drv_ism330dhcx_read_who_am_i(
    drv_ism330dhcx_t *dev,
    uint8_t *out_who_am_i)
{
    if (out_who_am_i == NULL)
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    return drv_ism330dhcx_read_regs(
        dev,
        ISM330DHCX_REG_WHO_AM_I,
        out_who_am_i,
        1u);
}

static drv_ism330dhcx_result_t drv_ism330dhcx_verify_reg_bits(
    drv_ism330dhcx_t *dev,
    uint8_t reg,
    uint8_t expected_mask)
{
    uint8_t value = 0u;

    const drv_ism330dhcx_result_t result =
        drv_ism330dhcx_read_regs(dev, reg, &value, 1u);

    if (result != DRV_ISM330DHCX_OK)
    {
        return result;
    }

    if ((value & expected_mask) != expected_mask)
    {
        return DRV_ISM330DHCX_ERROR_WHO_AM_I;
    }

    return DRV_ISM330DHCX_OK;
}

drv_ism330dhcx_result_t drv_ism330dhcx_init(
    drv_ism330dhcx_t *dev,
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_gpio,
    uint16_t cs_pin)
{
    uint8_t who_am_i = 0u;
    bool who_ok = false;

    if ((dev == NULL) || (hspi == NULL) || (cs_gpio == NULL) || (cs_pin == 0u))
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    memset(dev, 0, sizeof(*dev));

    dev->hspi = hspi;
    dev->cs_gpio = cs_gpio;
    dev->cs_pin = cs_pin;
    dev->timeout_ms = ISM330DHCX_DEFAULT_TIMEOUT_MS;

    /*
     * Safe deselected startup.
     */
    drv_ism330dhcx_cs_high(dev);
    HAL_Delay(50u);

    /*
     * Create a clean CS edge before the first real SPI transaction.
     */
    drv_ism330dhcx_cs_low(dev);
    drv_ism330dhcx_cs_delay();
    drv_ism330dhcx_cs_high(dev);
    HAL_Delay(10u);

    for (uint8_t attempt = 0u; attempt < 5u; attempt++)
    {
        if (drv_ism330dhcx_read_who_am_i(dev, &who_am_i) != DRV_ISM330DHCX_OK)
        {
            return DRV_ISM330DHCX_ERROR_SPI;
        }

        if (who_am_i == DRV_ISM330DHCX_WHO_AM_I_EXPECTED)
        {
            who_ok = true;
            break;
        }

        HAL_Delay(5u);
    }

    if (!who_ok)
    {
        return DRV_ISM330DHCX_ERROR_WHO_AM_I;
    }

    /*
     * Enable block data update and register auto-increment.
     *
     * BDU prevents mixed high/low bytes during data updates.
     * IF_INC is required for reliable multi-byte burst reads.
     */
    if (drv_ism330dhcx_write_reg(
            dev,
            ISM330DHCX_REG_CTRL3_C,
            (uint8_t)(ISM330DHCX_CTRL3_C_BDU | ISM330DHCX_CTRL3_C_IF_INC)) != DRV_ISM330DHCX_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    /*
     * Verify IF_INC and BDU really stuck before using burst reads.
     */
    if (drv_ism330dhcx_verify_reg_bits(
            dev,
            ISM330DHCX_REG_CTRL3_C,
            (uint8_t)(ISM330DHCX_CTRL3_C_BDU | ISM330DHCX_CTRL3_C_IF_INC)) != DRV_ISM330DHCX_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    if (drv_ism330dhcx_write_reg(
            dev,
            ISM330DHCX_REG_CTRL1_XL,
            ISM330DHCX_CTRL1_XL_104HZ_2G) != DRV_ISM330DHCX_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    if (drv_ism330dhcx_write_reg(
            dev,
            ISM330DHCX_REG_CTRL2_G,
            ISM330DHCX_CTRL2_G_104HZ_250DPS) != DRV_ISM330DHCX_OK)
    {
        return DRV_ISM330DHCX_ERROR_SPI;
    }

    HAL_Delay(50u);

    return DRV_ISM330DHCX_OK;
}

drv_ism330dhcx_result_t drv_ism330dhcx_read_raw(
    drv_ism330dhcx_t *dev,
    drv_ism330dhcx_raw_t *out_raw)
{
    uint8_t data[12];

    if (out_raw == NULL)
    {
        return DRV_ISM330DHCX_ERROR_NULL;
    }

    /*
     * Burst read from OUTX_L_G through OUTZ_H_A.
     *
     * This requires CTRL3_C.IF_INC = 1, which drv_ism330dhcx_init()
     * writes and verifies before returning OK.
     */
    const drv_ism330dhcx_result_t result =
        drv_ism330dhcx_read_regs(
            dev,
            ISM330DHCX_REG_OUTX_L_G,
            data,
            sizeof(data));

    if (result != DRV_ISM330DHCX_OK)
    {
        return result;
    }

    out_raw->gx_raw = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    out_raw->gy_raw = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    out_raw->gz_raw = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));

    out_raw->ax_raw = (int16_t)((uint16_t)data[6] | ((uint16_t)data[7] << 8));
    out_raw->ay_raw = (int16_t)((uint16_t)data[8] | ((uint16_t)data[9] << 8));
    out_raw->az_raw = (int16_t)((uint16_t)data[10] | ((uint16_t)data[11] << 8));

    return DRV_ISM330DHCX_OK;
}