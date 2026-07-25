#ifndef IMU_TYPES_H
#define IMU_TYPES_H

#include "drivers/drv_ism330dhcx.h"
#include "estimation/est_attitude.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint64_t timestamp_us;

    bool valid;
    bool calibrated;

    uint32_t sample_count;
    uint32_t read_error_count;

    float gyro_bias_rps[3];

    drv_ism330dhcx_raw_t raw;
    drv_ism330dhcx_data_t data;

    est_attitude_output_t attitude;
} imu_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif