#ifndef TASK_IMU_H
#define TASK_IMU_H

#include "app/imu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_imu_start(void);
void task_imu_get_snapshot(imu_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif