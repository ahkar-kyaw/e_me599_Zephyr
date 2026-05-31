#ifndef DRV_ISM330DHCX_H
#define DRV_ISM330DHCX_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_ISM330DHCX_WHO_AM_I_EXPECTED    0x6Bu

typedef enum
{
    DRV_ISM330DHCX_OK = 0,
    DRV_ISM330DHCX_ERROR_NULL,
    DRV_ISM330DHCX_ERROR_SPI,
    DRV_ISM330DHCX_ERROR_WHO_AM_I,
    DRV_ISM330DHCX_ERROR_TIMEOUT
} drv_ism330dhcx_result_t;

typedef struct
{
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;

    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;
} drv_ism330dhcx_raw_t;

typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_gpio;
    uint16_t cs_pin;
    uint32_t timeout_ms;
} drv_ism330dhcx_t;

drv_ism330dhcx_result_t drv_ism330dhcx_init(
    drv_ism330dhcx_t *dev,
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_gpio,
    uint16_t cs_pin);

drv_ism330dhcx_result_t drv_ism330dhcx_read_who_am_i(
    drv_ism330dhcx_t *dev,
    uint8_t *out_who_am_i);

drv_ism330dhcx_result_t drv_ism330dhcx_read_raw(
    drv_ism330dhcx_t *dev,
    drv_ism330dhcx_raw_t *out_raw);

#ifdef __cplusplus
}
#endif

#endif