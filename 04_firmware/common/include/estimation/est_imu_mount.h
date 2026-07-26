#ifndef EST_IMU_MOUNT_H
#define EST_IMU_MOUNT_H

#include "drivers/drv_ism330dhcx.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EST_IMU_AXIS_SENSOR_X_POS = 0,
    EST_IMU_AXIS_SENSOR_X_NEG,
    EST_IMU_AXIS_SENSOR_Y_POS,
    EST_IMU_AXIS_SENSOR_Y_NEG,
    EST_IMU_AXIS_SENSOR_Z_POS,
    EST_IMU_AXIS_SENSOR_Z_NEG
} est_imu_axis_t;

typedef struct
{
    est_imu_axis_t body_forward;
    est_imu_axis_t body_up;
} est_imu_mount_config_t;

typedef struct
{
    bool valid;

    float body_forward_in_sensor[3];
    float body_left_in_sensor[3];
    float body_up_in_sensor[3];
} est_imu_mount_t;

est_imu_mount_config_t est_imu_mount_default_config(void);

bool est_imu_mount_init(est_imu_mount_t *mount,
                        const est_imu_mount_config_t *config);

bool est_imu_mount_apply_data(const est_imu_mount_t *mount,
                              const drv_ism330dhcx_data_t *sensor_data,
                              drv_ism330dhcx_data_t *body_data);

#ifdef __cplusplus
}
#endif

#endif