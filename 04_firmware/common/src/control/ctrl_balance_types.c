#include "control/ctrl_balance_types.h"

#include <string.h>

bool ctrl_balance_state_from_imu(const imu_snapshot_t *imu,
                                 const safety_imu_status_t *imu_status,
                                 balance_state_t *balance_state)
{
    if ((imu == 0) || (imu_status == 0) || (balance_state == 0))
    {
        return false;
    }

    memset(balance_state, 0, sizeof(*balance_state));

    balance_state->timestamp_us = imu->timestamp_us;

    if (!imu_status->safe_for_balance)
    {
        return false;
    }

    if (!imu->attitude.valid)
    {
        return false;
    }

    balance_state->valid = true;

    balance_state->pitch_rad = imu->attitude.pitch_rad;
    balance_state->pitch_rate_rps = imu->attitude.pitch_rate_rps;

    balance_state->roll_rad = imu->attitude.roll_rad;
    balance_state->roll_rate_rps = imu->attitude.roll_rate_rps;

    return true;
}