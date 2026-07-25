#ifndef EST_IMU_CALIBRATION_H
#define EST_IMU_CALIBRATION_H

#include "drivers/drv_ism330dhcx.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t sample_count;
    uint32_t target_sample_count;

    float gyro_sum_rps[3];
    float gyro_bias_rps[3];

    bool complete;
} est_imu_calibration_t;

void est_imu_calibration_reset(est_imu_calibration_t *cal,
                               uint32_t target_sample_count);

bool est_imu_calibration_update(est_imu_calibration_t *cal,
                                const drv_ism330dhcx_data_t *data);

bool est_imu_calibration_is_complete(const est_imu_calibration_t *cal);

void est_imu_calibration_apply(const est_imu_calibration_t *cal,
                               drv_ism330dhcx_data_t *data);

#ifdef __cplusplus
}
#endif

#endif