#ifndef DRV_ISM330DHCX_H
#define DRV_ISM330DHCX_H

#include "driver/spi_master.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_ISM330DHCX_WHO_AM_I_EXPECTED    0x6Bu

typedef enum
{
    DRV_ISM330DHCX_ODR_POWER_DOWN = 0x00,
    DRV_ISM330DHCX_ODR_12_5_HZ    = 0x01,
    DRV_ISM330DHCX_ODR_26_HZ      = 0x02,
    DRV_ISM330DHCX_ODR_52_HZ      = 0x03,
    DRV_ISM330DHCX_ODR_104_HZ     = 0x04,
    DRV_ISM330DHCX_ODR_208_HZ     = 0x05,
    DRV_ISM330DHCX_ODR_416_HZ     = 0x06,
    DRV_ISM330DHCX_ODR_833_HZ     = 0x07,
    DRV_ISM330DHCX_ODR_1660_HZ    = 0x08,
    DRV_ISM330DHCX_ODR_3330_HZ    = 0x09,
    DRV_ISM330DHCX_ODR_6660_HZ    = 0x0A
} drv_ism330dhcx_odr_t;

typedef enum
{
    DRV_ISM330DHCX_ACCEL_FS_2G  = 0x00,
    DRV_ISM330DHCX_ACCEL_FS_16G = 0x01,
    DRV_ISM330DHCX_ACCEL_FS_4G  = 0x02,
    DRV_ISM330DHCX_ACCEL_FS_8G  = 0x03
} drv_ism330dhcx_accel_fs_t;

typedef enum
{
    DRV_ISM330DHCX_GYRO_FS_250_DPS  = 0x00,
    DRV_ISM330DHCX_GYRO_FS_500_DPS  = 0x01,
    DRV_ISM330DHCX_GYRO_FS_1000_DPS = 0x02,
    DRV_ISM330DHCX_GYRO_FS_2000_DPS = 0x03
} drv_ism330dhcx_gyro_fs_t;

typedef struct
{
    drv_ism330dhcx_odr_t accel_odr;
    drv_ism330dhcx_odr_t gyro_odr;
    drv_ism330dhcx_accel_fs_t accel_fs;
    drv_ism330dhcx_gyro_fs_t gyro_fs;
} drv_ism330dhcx_config_t;

typedef struct
{
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temp_raw;

    float accel_mps2[3];
    float gyro_dps[3];
    float temp_c;

    uint8_t status_reg;
    bool accel_data_ready;
    bool gyro_data_ready;
    bool temp_data_ready;
} drv_ism330dhcx_sample_t;

typedef struct
{
    spi_device_handle_t spi;

    drv_ism330dhcx_config_t config;

    float accel_lsb_to_mps2;
    float gyro_lsb_to_dps;
} drv_ism330dhcx_t;

drv_ism330dhcx_config_t drv_ism330dhcx_default_config(void);

esp_err_t drv_ism330dhcx_bus_init(void);
esp_err_t drv_ism330dhcx_add_device(drv_ism330dhcx_t *dev);
esp_err_t drv_ism330dhcx_init(drv_ism330dhcx_t *dev,
                              const drv_ism330dhcx_config_t *config);

esp_err_t drv_ism330dhcx_read_who_am_i(drv_ism330dhcx_t *dev,
                                        uint8_t *who_am_i);

esp_err_t drv_ism330dhcx_read_status(drv_ism330dhcx_t *dev,
                                      uint8_t *status_reg);

esp_err_t drv_ism330dhcx_read_sample(drv_ism330dhcx_t *dev,
                                      drv_ism330dhcx_sample_t *sample);

esp_err_t drv_ism330dhcx_soft_reset(drv_ism330dhcx_t *dev);

#ifdef __cplusplus
}
#endif

#endif