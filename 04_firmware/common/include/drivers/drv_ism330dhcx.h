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
} drv_ism330dhcx_t;

int drv_ism330dhcx_init(drv_ism330dhcx_t *dev, if_spi_t *spi);
int drv_ism330dhcx_read_who_am_i(drv_ism330dhcx_t *dev, uint8_t *who_am_i);
int drv_ism330dhcx_read_raw(drv_ism330dhcx_t *dev, drv_ism330dhcx_raw_t *raw);

void drv_ism330dhcx_convert_raw(const drv_ism330dhcx_raw_t *raw,
                                drv_ism330dhcx_data_t *data);

#ifdef __cplusplus
}
#endif

#endif