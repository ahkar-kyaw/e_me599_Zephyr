#ifndef SAFETY_MANUAL_DRIVE_H
#define SAFETY_MANUAL_DRIVE_H

#include "app/app_actuator_types.h"
#include "app/app_manual_drive_types.h"
#include "app/app_rc_types.h"
#include "safety/safety_rc.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint64_t request_max_age_us;
    uint64_t stop_hold_us;
    uint16_t channel_min;
    uint16_t channel_max;
    uint16_t stick_neutral_low;
    uint16_t stick_neutral_high;
    float max_velocity_erpm;
    uint8_t velocity_channel;
    bool velocity_positive_high;
} safety_manual_drive_config_t;

typedef struct
{
    safety_manual_drive_config_t config;
    app_manual_drive_state_t state;
    uint64_t stop_until_us;
    uint8_t actuator_index;
    bool request_cycle_required;
    bool initialized;
} safety_manual_drive_t;

bool safety_manual_drive_init(
    safety_manual_drive_t *safety,
    const safety_manual_drive_config_t *config);

void safety_manual_drive_update(
    safety_manual_drive_t *safety,
    const app_manual_drive_request_t *request,
    const rc_snapshot_t *rc_snapshot,
    const safety_rc_status_t *rc_status,
    const actuator_snapshot_t *actuator_snapshot,
    uint64_t now_us,
    app_manual_drive_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
