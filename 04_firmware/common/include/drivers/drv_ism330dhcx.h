#ifndef DRV_ISM330DHCX_H
#define DRV_ISM330DHCX_H

#include "interfaces/if_spi.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_ISM330DHCX_OK       0
#define DRV_ISM330DHCX_ERR_ARG -1
#define DRV_ISM330DHCX_ERR_IO  -2
#define DRV_ISM330DHCX_ERR_ID  -3

#define DRV_ISM330DHCX_WHO_AM_I_EXPECTED 0x6B

typedef enum
{
    DRV_ISM330DHCX_ODR_104_HZ = 0x04,
    DRV_ISM330DHCX_ODR_208_HZ = 0x05,
    DRV_ISM330DHCX_ODR_416_HZ = 0x06
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
} drv_ism330dhcx_raw_t;

typedef struct
{
    float accel_mps2[3];
    float gyro_rps[3];
    float temp_c;
} drv_ism330dhcx_data_t;

typedef struct
{
    if_spi_t *spi;

    drv_ism330dhcx_config_t config;

    float accel_mps2_per_lsb;
    float gyro_rps_per_lsb;
} drv_ism330dhcx_t;

drv_ism330dhcx_config_t drv_ism330dhcx_default_config(void);

int drv_ism330dhcx_init(drv_ism330dhcx_t *dev, if_spi_t *spi);

int drv_ism330dhcx_init_config(drv_ism330dhcx_t *dev,
                               if_spi_t *spi,
                               const drv_ism330dhcx_config_t *config);

int drv_ism330dhcx_read_who_am_i(drv_ism330dhcx_t *dev, uint8_t *who_am_i);

int drv_ism330dhcx_read_raw(drv_ism330dhcx_t *dev,
                            drv_ism330dhcx_raw_t *raw);

void drv_ism330dhcx_convert_raw(const drv_ism330dhcx_t *dev,
                                const drv_ism330dhcx_raw_t *raw,
                                drv_ism330dhcx_data_t *data);

#ifdef __cplusplus
}
#endif

#endif