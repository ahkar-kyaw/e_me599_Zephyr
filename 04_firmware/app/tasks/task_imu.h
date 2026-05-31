#ifndef TASK_IMU_H
#define TASK_IMU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool initialized;
    bool data_valid;

    uint8_t who_am_i;

    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;

    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;

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