#ifndef CTRL_BALANCE_TYPES_H
#define CTRL_BALANCE_TYPES_H

#include "app/imu_types.h"
#include "safety/safety_imu.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool valid;

    uint64_t timestamp_us;

    float pitch_rad;
    float pitch_rate_rps;

    float roll_rad;
    float roll_rate_rps;
} balance_state_t;

bool ctrl_balance_state_from_imu(const imu_snapshot_t *imu,
                                 const safety_imu_status_t *imu_status,
                                 balance_state_t *balance_state);

#ifdef __cplusplus
}
#endif

#endif