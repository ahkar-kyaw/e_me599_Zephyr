#include "safety/safety_rc.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SAFETY_RC_DEFAULT_MAX_AGE_US 100000u
#define SAFETY_RC_DEFAULT_RAW_MIN    172u
#define SAFETY_RC_DEFAULT_RAW_MAX    1811u

safety_rc_config_t safety_rc_default_config(void)
{
    const safety_rc_config_t config =
    {
        .max_age_us = SAFETY_RC_DEFAULT_MAX_AGE_US,
        .channel_min = SAFETY_RC_DEFAULT_RAW_MIN,
        .channel_max = SAFETY_RC_DEFAULT_RAW_MAX,
    };

    return config;
}

void safety_rc_check(const rc_snapshot_t *snapshot,
                     uint64_t now_us,
                     const safety_rc_config_t *config,
                     safety_rc_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    memset(status, 0, sizeof(*status));
    status->age_us = UINT64_MAX;

    if ((snapshot == NULL) || (config == NULL) || !snapshot->valid)
    {
        status->fault_flags |= SAFETY_RC_FAULT_NO_DATA;
        return;
    }

    if (now_us >= snapshot->timestamp_us)
    {
        status->age_us = now_us - snapshot->timestamp_us;
    }

    if (status->age_us > config->max_age_us)
    {
        status->fault_flags |= SAFETY_RC_FAULT_STALE;
    }

    for (uint32_t i = 0u; i < APP_RC_CHANNEL_COUNT; i++)
    {
        if ((snapshot->channel[i] < config->channel_min) ||
            (snapshot->channel[i] > config->channel_max))
        {
            status->fault_flags |= SAFETY_RC_FAULT_CHANNEL_RANGE;
            break;
        }
    }

    status->safe_for_control =
        (status->fault_flags == SAFETY_RC_FAULT_NONE);
}
