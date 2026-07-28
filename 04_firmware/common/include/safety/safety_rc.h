#ifndef SAFETY_RC_H
#define SAFETY_RC_H

#include "app/app_rc_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SAFETY_RC_FAULT_NONE = 0u,
    SAFETY_RC_FAULT_NO_DATA = 1u << 0,
    SAFETY_RC_FAULT_STALE = 1u << 1,
    SAFETY_RC_FAULT_CHANNEL_RANGE = 1u << 2
} safety_rc_fault_t;

typedef struct
{
    uint64_t max_age_us;
    uint16_t channel_min;
    uint16_t channel_max;
} safety_rc_config_t;

typedef struct
{
    uint32_t fault_flags;
    uint64_t age_us;
    bool safe_for_control;
} safety_rc_status_t;

safety_rc_config_t safety_rc_default_config(void);

void safety_rc_check(const rc_snapshot_t *snapshot,
                     uint64_t now_us,
                     const safety_rc_config_t *config,
                     safety_rc_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
