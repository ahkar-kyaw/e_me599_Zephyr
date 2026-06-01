#ifndef TASK_IMU_H
#define TASK_IMU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TASK_IMU_STATE_INIT = 0,
    TASK_IMU_STATE_CALIBRATING,
    TASK_IMU_STATE_RUNNING,
    TASK_IMU_STATE_ERROR
} task_imu_state_t;

typedef struct
{
    bool initialized;
    bool data_valid;
    bool gyro_calibrated;

    task_imu_state_t state;

    uint8_t who_am_i;
    uint8_t init_result;
    uint8_t last_read_result;

    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;

    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;

    int32_t ax_mg;
    int32_t ay_mg;
    int32_t az_mg;
    
    uint32_t accel_norm_mg;

    int32_t gx_mdps;
    int32_t gy_mdps;
    int32_t gz_mdps;

    int32_t gx_bias_raw;
    int32_t gy_bias_raw;
    int32_t gz_bias_raw;

    int16_t pitch_cdeg;
    int16_t roll_cdeg;

    uint32_t calibration_count;
    uint32_t calibration_target;

    uint32_t sample_count;
    uint32_t error_count;
    uint32_t last_update_ms;
} task_imu_snapshot_t;

void task_imu_start(void);
bool task_imu_get_snapshot(task_imu_snapshot_t *out_snapshot);

#ifdef __cplusplus
}
#endif

#endif