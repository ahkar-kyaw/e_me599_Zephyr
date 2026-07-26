#include "safety/safety_imu.h"

#include <math.h>
#include <string.h>

#define SAFETY_IMU_DEFAULT_MAX_AGE_US             50000u
#define SAFETY_IMU_DEFAULT_MAX_ABS_ROLL_RAD       1.0471976f
#define SAFETY_IMU_DEFAULT_MAX_ABS_PITCH_RAD      1.0471976f
#define SAFETY_IMU_DEFAULT_MIN_ACCEL_NORM_MPS2    3.0f
#define SAFETY_IMU_DEFAULT_MAX_ACCEL_NORM_MPS2    20.0f

static float safety_abs_f32(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float safety_accel_norm_mps2(const float accel_mps2[3])
{
    float x = accel_mps2[0];
    float y = accel_mps2[1];
    float z = accel_mps2[2];

    return sqrtf((x * x) + (y * y) + (z * z));
}

safety_imu_config_t safety_imu_default_config(void)
{
    safety_imu_config_t config;

    config.max_age_us = SAFETY_IMU_DEFAULT_MAX_AGE_US;
    config.max_abs_roll_rad = SAFETY_IMU_DEFAULT_MAX_ABS_ROLL_RAD;
    config.max_abs_pitch_rad = SAFETY_IMU_DEFAULT_MAX_ABS_PITCH_RAD;
    config.min_accel_norm_mps2 = SAFETY_IMU_DEFAULT_MIN_ACCEL_NORM_MPS2;
    config.max_accel_norm_mps2 = SAFETY_IMU_DEFAULT_MAX_ACCEL_NORM_MPS2;

    return config;
}

bool safety_imu_check(const imu_snapshot_t *snapshot,
                      uint64_t now_us,
                      const safety_imu_config_t *config,
                      safety_imu_status_t *status)
{
    if ((snapshot == 0) || (config == 0) || (status == 0))
    {
        return false;
    }

    memset(status, 0, sizeof(*status));

    if (!snapshot->valid)
    {
        status->fault_flags |= SAFETY_IMU_FAULT_INVALID;
    }

    if (!snapshot->calibrated)
    {
        status->fault_flags |= SAFETY_IMU_FAULT_NOT_CALIBRATED;
    }

    if (!snapshot->attitude.valid)
    {
        status->fault_flags |= SAFETY_IMU_FAULT_ATTITUDE;
    }

    if (now_us < snapshot->timestamp_us)
    {
        status->fault_flags |= SAFETY_IMU_FAULT_TIME;
    }
    else
    {
        status->age_us = now_us - snapshot->timestamp_us;

        if (status->age_us > config->max_age_us)
        {
            status->fault_flags |= SAFETY_IMU_FAULT_STALE;
        }
    }

    status->abs_roll_rad = safety_abs_f32(snapshot->attitude.roll_rad);
    status->abs_pitch_rad = safety_abs_f32(snapshot->attitude.pitch_rad);

    if ((status->abs_roll_rad > config->max_abs_roll_rad) ||
        (status->abs_pitch_rad > config->max_abs_pitch_rad))
    {
        status->fault_flags |= SAFETY_IMU_FAULT_TILT;
    }

    status->accel_norm_mps2 =
        safety_accel_norm_mps2(snapshot->data.accel_mps2);

    if ((status->accel_norm_mps2 < config->min_accel_norm_mps2) ||
        (status->accel_norm_mps2 > config->max_accel_norm_mps2))
    {
        status->fault_flags |= SAFETY_IMU_FAULT_ACCEL_NORM;
    }

    status->safe_for_balance =
        (status->fault_flags == SAFETY_IMU_FAULT_NONE);

    return status->safe_for_balance;
}