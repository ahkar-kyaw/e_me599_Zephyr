#include "estimation/est_attitude.h"

#include <math.h>
#include <string.h>

#define EST_ATTITUDE_DEFAULT_ALPHA      0.98f
#define EST_ATTITUDE_DEFAULT_MAG_ALPHA  0.98f
#define EST_ATTITUDE_MIN_DT_S           0.0001f
#define EST_ATTITUDE_MAX_DT_S           0.1f
#define EST_ATTITUDE_PI                 3.1415926536f
#define EST_ATTITUDE_TWO_PI             6.2831853072f

static float wrap_pi(float angle_rad)
{
    while (angle_rad > EST_ATTITUDE_PI)
    {
        angle_rad -= EST_ATTITUDE_TWO_PI;
    }

    while (angle_rad < -EST_ATTITUDE_PI)
    {
        angle_rad += EST_ATTITUDE_TWO_PI;
    }

    return angle_rad;
}

static float accel_roll_rad(const float accel_mps2[3])
{
    return atan2f(accel_mps2[1], accel_mps2[2]);
}

static float accel_pitch_rad(const float accel_mps2[3])
{
    float ay = accel_mps2[1];
    float az = accel_mps2[2];

    return atan2f(-accel_mps2[0], sqrtf((ay * ay) + (az * az)));
}

static bool mag_yaw_rad(const float mag_uT[3],
                        float roll_rad,
                        float pitch_rad,
                        float *yaw_rad)
{
    if ((mag_uT == 0) || (yaw_rad == 0))
    {
        return false;
    }

    float mx = mag_uT[0];
    float my = mag_uT[1];
    float mz = mag_uT[2];

    float mag_norm = sqrtf((mx * mx) + (my * my) + (mz * mz));

    if (mag_norm < 0.0001f)
    {
        return false;
    }

    float sr = sinf(roll_rad);
    float cr = cosf(roll_rad);
    float sp = sinf(pitch_rad);
    float cp = cosf(pitch_rad);

    float xh = (mx * cp) + (mz * sp);
    float yh = (mx * sr * sp) + (my * cr) - (mz * sr * cp);

    *yaw_rad = wrap_pi(atan2f(-yh, xh));

    return true;
}

static float blend_angle_rad(float predicted_rad,
                             float measured_rad,
                             float alpha)
{
    float error = wrap_pi(measured_rad - predicted_rad);

    return wrap_pi(predicted_rad + ((1.0f - alpha) * error));
}

est_attitude_config_t est_attitude_default_config(void)
{
    est_attitude_config_t config;

    config.algorithm = EST_ATTITUDE_ALGORITHM_COMPLEMENTARY;
    config.sensor_mode = EST_ATTITUDE_SENSOR_MODE_6DOF;
    config.complementary_alpha = EST_ATTITUDE_DEFAULT_ALPHA;
    config.complementary_mag_alpha = EST_ATTITUDE_DEFAULT_MAG_ALPHA;

    return config;
}

void est_attitude_init(est_attitude_t *est,
                       const est_attitude_config_t *config)
{
    if ((est == 0) || (config == 0))
    {
        return;
    }

    memset(est, 0, sizeof(*est));

    est->algorithm = config->algorithm;
    est->sensor_mode = config->sensor_mode;

    est->complementary_alpha = config->complementary_alpha;
    est->complementary_mag_alpha = config->complementary_mag_alpha;

    if ((est->complementary_alpha <= 0.0f) ||
        (est->complementary_alpha >= 1.0f))
    {
        est->complementary_alpha = EST_ATTITUDE_DEFAULT_ALPHA;
    }

    if ((est->complementary_mag_alpha <= 0.0f) ||
        (est->complementary_mag_alpha >= 1.0f))
    {
        est->complementary_mag_alpha = EST_ATTITUDE_DEFAULT_MAG_ALPHA;
    }
}

void est_attitude_reset(est_attitude_t *est)
{
    if (est == 0)
    {
        return;
    }

    est->initialized = false;
    est->roll_rad = 0.0f;
    est->pitch_rad = 0.0f;
    est->yaw_rad = 0.0f;
}

bool est_attitude_update(est_attitude_t *est,
                         const est_attitude_input_t *input,
                         est_attitude_output_t *output)
{
    if ((est == 0) || (input == 0) || (output == 0))
    {
        return false;
    }

    memset(output, 0, sizeof(*output));

    if ((input->dt_s < EST_ATTITUDE_MIN_DT_S) ||
        (input->dt_s > EST_ATTITUDE_MAX_DT_S))
    {
        return false;
    }

    if (est->algorithm != EST_ATTITUDE_ALGORITHM_COMPLEMENTARY)
    {
        return false;
    }

    float accel_roll = accel_roll_rad(input->accel_mps2);
    float accel_pitch = accel_pitch_rad(input->accel_mps2);

    if (!est->initialized)
    {
        est->roll_rad = accel_roll;
        est->pitch_rad = accel_pitch;
        est->yaw_rad = 0.0f;

        if ((est->sensor_mode == EST_ATTITUDE_SENSOR_MODE_9DOF) &&
            input->mag_valid)
        {
            float yaw = 0.0f;

            if (mag_yaw_rad(input->mag_uT,
                            est->roll_rad,
                            est->pitch_rad,
                            &yaw))
            {
                est->yaw_rad = yaw;
            }
        }

        est->initialized = true;
    }
    else
    {
        float alpha = est->complementary_alpha;
        float one_minus_alpha = 1.0f - alpha;

        float gyro_roll = est->roll_rad +
                          (input->gyro_rps[0] * input->dt_s);

        float gyro_pitch = est->pitch_rad +
                           (input->gyro_rps[1] * input->dt_s);

        float gyro_yaw = est->yaw_rad +
                         (input->gyro_rps[2] * input->dt_s);

        est->roll_rad = (alpha * gyro_roll) +
                        (one_minus_alpha * accel_roll);

        est->pitch_rad = (alpha * gyro_pitch) +
                         (one_minus_alpha * accel_pitch);

        est->yaw_rad = wrap_pi(gyro_yaw);

        if ((est->sensor_mode == EST_ATTITUDE_SENSOR_MODE_9DOF) &&
            input->mag_valid)
        {
            float yaw = 0.0f;

            if (mag_yaw_rad(input->mag_uT,
                            est->roll_rad,
                            est->pitch_rad,
                            &yaw))
            {
                est->yaw_rad = blend_angle_rad(est->yaw_rad,
                                               yaw,
                                               est->complementary_mag_alpha);
            }
        }
    }

    output->valid = true;

    output->roll_rad = est->roll_rad;
    output->pitch_rad = est->pitch_rad;
    output->yaw_rad = est->yaw_rad;

    output->roll_rate_rps = input->gyro_rps[0];
    output->pitch_rate_rps = input->gyro_rps[1];
    output->yaw_rate_rps = input->gyro_rps[2];

    return true;
}