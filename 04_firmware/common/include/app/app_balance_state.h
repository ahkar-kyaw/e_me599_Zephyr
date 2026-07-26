#ifndef APP_BALANCE_STATE_H
#define APP_BALANCE_STATE_H

#include "app/app_imu_types.h"
#include "control/ctrl_balance_types.h"
#include "safety/safety_imu.h"

#ifdef __cplusplus
extern "C" {
#endif

bool app_balance_state_from_imu(const imu_snapshot_t *imu,
                                const safety_imu_status_t *imu_status,
                                balance_state_t *balance_state);

#ifdef __cplusplus
}
#endif

#endif