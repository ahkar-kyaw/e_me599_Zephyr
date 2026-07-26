#ifndef SAFETY_IMU_H
#define SAFETY_IMU_H

#include "app/app_imu_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SAFETY_IMU_FAULT_NONE           = 0u,
    SAFETY_IMU_FAULT_INVALID        = 1u << 0,
    SAFETY_IMU_FAULT_NOT_CALIBRATED = 1u << 1,
    SAFETY_IMU_FAULT_ATTITUDE       = 1u << 2,
    SAFETY_IMU_FAULT_STALE          = 1u << 3,
    SAFETY_IMU_FAULT_TIME           = 1u << 4,
    SAFETY_IMU_FAULT_TILT           = 1u << 5,
    SAFETY_IMU_FAULT_ACCEL_NORM     = 1u << 6
} safety_imu_fault_t;

typedef struct
{
    uint64_t max_age_us;

    float max_abs_roll_rad;
    float max_abs_pitch_rad;

    float min_accel_norm_mps2;
    float max_accel_norm_mps2;
} safety_imu_config_t;

typedef struct
{
    bool safe_for_balance;

    uint32_t fault_flags;
    uint64_t age_us;

    float accel_norm_mps2;
    float abs_roll_rad;
    float abs_pitch_rad;
} safety_imu_status_t;

safety_imu_config_t safety_imu_default_config(void);

bool safety_imu_check(const imu_snapshot_t *snapshot,
                      uint64_t now_us,
                      const safety_imu_config_t *config,
                      safety_imu_status_t *status);

#ifdef __cplusplus
}
#endif

#endif