#include "estimation/est_imu_mount.h"

#include <string.h>

static void axis_to_vector(est_imu_axis_t axis, float vector[3])
{
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    vector[2] = 0.0f;

    switch (axis)
    {
        case EST_IMU_AXIS_SENSOR_X_POS:
            vector[0] = 1.0f;
            break;

        case EST_IMU_AXIS_SENSOR_X_NEG:
            vector[0] = -1.0f;
            break;

        case EST_IMU_AXIS_SENSOR_Y_POS:
            vector[1] = 1.0f;
            break;

        case EST_IMU_AXIS_SENSOR_Y_NEG:
            vector[1] = -1.0f;
            break;

        case EST_IMU_AXIS_SENSOR_Z_POS:
            vector[2] = 1.0f;
            break;

        case EST_IMU_AXIS_SENSOR_Z_NEG:
            vector[2] = -1.0f;
            break;

        default:
            break;
    }
}

static float dot3(const float a[3], const float b[3])
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

static void cross3(const float a[3], const float b[3], float out[3])
{
    out[0] = (a[1] * b[2]) - (a[2] * b[1]);
    out[1] = (a[2] * b[0]) - (a[0] * b[2]);
    out[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

static void map_vector_to_body(const est_imu_mount_t *mount,
                               const float sensor[3],
                               float body[3])
{
    body[0] = dot3(sensor, mount->body_forward_in_sensor);
    body[1] = dot3(sensor, mount->body_left_in_sensor);
    body[2] = dot3(sensor, mount->body_up_in_sensor);
}

est_imu_mount_config_t est_imu_mount_default_config(void)
{
    est_imu_mount_config_t config;

    config.body_forward = EST_IMU_AXIS_SENSOR_X_POS;
    config.body_up = EST_IMU_AXIS_SENSOR_Z_POS;

    return config;
}

bool est_imu_mount_init(est_imu_mount_t *mount,
                        const est_imu_mount_config_t *config)
{
    if ((mount == 0) || (config == 0))
    {
        return false;
    }

    memset(mount, 0, sizeof(*mount));

    axis_to_vector(config->body_forward, mount->body_forward_in_sensor);
    axis_to_vector(config->body_up, mount->body_up_in_sensor);

    if (dot3(mount->body_forward_in_sensor,
             mount->body_up_in_sensor) != 0.0f)
    {
        return false;
    }

    cross3(mount->body_up_in_sensor,
           mount->body_forward_in_sensor,
           mount->body_left_in_sensor);

    mount->valid = true;

    return true;
}

bool est_imu_mount_apply_data(const est_imu_mount_t *mount,
                              const drv_ism330dhcx_data_t *sensor_data,
                              drv_ism330dhcx_data_t *body_data)
{
    if ((mount == 0) ||
        (sensor_data == 0) ||
        (body_data == 0) ||
        (!mount->valid))
    {
        return false;
    }

    map_vector_to_body(mount,
                       sensor_data->accel_mps2,
                       body_data->accel_mps2);

    map_vector_to_body(mount,
                       sensor_data->gyro_rps,
                       body_data->gyro_rps);

    body_data->temp_c = sensor_data->temp_c;

    return true;
}