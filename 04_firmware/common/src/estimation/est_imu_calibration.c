#include "estimation/est_imu_calibration.h"

#include <string.h>

void est_imu_calibration_reset(est_imu_calibration_t *cal,
                               uint32_t target_sample_count)
{
    if (cal == 0)
    {
        return;
    }

    memset(cal, 0, sizeof(*cal));

    if (target_sample_count == 0u)
    {
        target_sample_count = 1u;
    }

    cal->target_sample_count = target_sample_count;
}

bool est_imu_calibration_update(est_imu_calibration_t *cal,
                                const drv_ism330dhcx_data_t *data)
{
    if ((cal == 0) || (data == 0))
    {
        return false;
    }

    if (cal->complete)
    {
        return true;
    }

    for (uint32_t i = 0u; i < 3u; i++)
    {
        cal->gyro_sum_rps[i] += data->gyro_rps[i];
    }

    cal->sample_count++;

    if (cal->sample_count >= cal->target_sample_count)
    {
        for (uint32_t i = 0u; i < 3u; i++)
        {
            cal->gyro_bias_rps[i] =
                cal->gyro_sum_rps[i] / (float)cal->sample_count;
        }

        cal->complete = true;
    }

    return cal->complete;
}

bool est_imu_calibration_is_complete(const est_imu_calibration_t *cal)
{
    if (cal == 0)
    {
        return false;
    }

    return cal->complete;
}

void est_imu_calibration_apply(const est_imu_calibration_t *cal,
                               drv_ism330dhcx_data_t *data)
{
    if ((cal == 0) || (data == 0) || !cal->complete)
    {
        return;
    }

    for (uint32_t i = 0u; i < 3u; i++)
    {
        data->gyro_rps[i] -= cal->gyro_bias_rps[i];
    }
}