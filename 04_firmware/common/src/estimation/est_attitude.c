#include "estimation/est_attitude.h"

#include <math.h>
#include <string.h>

#define EST_ATTITUDE_DEFAULT_ALPHA 0.98f
#define EST_ATTITUDE_MIN_DT_S      0.0001f
#define EST_ATTITUDE_MAX_DT_S      0.1f

static float est_attitude_accel_roll_rad(const float accel_mps2[3])
{
    return atan2f(accel_mps2[1], accel_mps2[2]);
}

static float est_attitude_accel_pitch_rad(const float accel_mps2[3])
{
    float ay = accel_mps2[1];
    float az = accel_mps2[2];

    return atan2f(-accel_mps2[0], sqrtf((ay * ay) + (az * az)));
}

est_attitude_config_t est_attitude_default_config(void)
{
    est_attitude_config_t config;

    config.algorithm = EST_ATTITUDE_ALGORITHM_COMPLEMENTARY;
    config.complementary_alpha = EST_ATTITUDE_DEFAULT_ALPHA;

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
    est->complementary_alpha = config->complementary_alpha;

    if (est->complementary_alpha <= 0.0f || est->complementary_alpha >= 1.0f)
    {
        est->complementary_alpha = EST_ATTITUDE_DEFAULT_ALPHA;
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

    float accel_roll_rad = est_attitude_accel_roll_rad(input->accel_mps2);
    float accel_pitch_rad = est_attitude_accel_pitch_rad(input->accel_mps2);

    if (!est->initialized)
    {
        est->roll_rad = accel_roll_rad;
        est->pitch_rad = accel_pitch_rad;
        est->yaw_rad = 0.0f;
        est->initialized = true;
    }
    else
    {
        float alpha = est->complementary_alpha;
        float one_minus_alpha = 1.0f - alpha;

        float gyro_roll_rad = est->roll_rad +
                              (input->gyro_rps[0] * input->dt_s);

        float gyro_pitch_rad = est->pitch_rad +
                               (input->gyro_rps[1] * input->dt_s);

        est->roll_rad = (alpha * gyro_roll_rad) +
                        (one_minus_alpha * accel_roll_rad);

        est->pitch_rad = (alpha * gyro_pitch_rad) +
                         (one_minus_alpha * accel_pitch_rad);

        est->yaw_rad += input->gyro_rps[2] * input->dt_s;
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