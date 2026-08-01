#include "safety/safety_manual_drive.h"

#include <stddef.h>
#include <string.h>

static bool safety_manual_drive_config_valid(
    const safety_manual_drive_config_t *config);
static bool safety_manual_drive_request_fresh(
    const app_manual_drive_request_t *request,
    uint64_t now_us,
    uint64_t max_age_us);
static bool safety_manual_drive_stick_neutral(
    const safety_manual_drive_config_t *config,
    uint16_t value);
static float safety_manual_drive_velocity(
    const safety_manual_drive_config_t *config,
    uint16_t value);
static uint32_t safety_manual_drive_inhibits(
    const safety_manual_drive_t *safety,
    const app_manual_drive_request_t *request,
    bool request_fresh,
    const rc_snapshot_t *rc_snapshot,
    const safety_rc_status_t *rc_status,
    const actuator_snapshot_t *actuator_snapshot,
    bool stick_neutral);
static void safety_manual_drive_begin_stop(
    safety_manual_drive_t *safety,
    uint64_t now_us,
    bool require_request_cycle);

bool safety_manual_drive_init(
    safety_manual_drive_t *safety,
    const safety_manual_drive_config_t *config)
{
    if ((safety == NULL) || !safety_manual_drive_config_valid(config))
    {
        return false;
    }

    memset(safety, 0, sizeof(*safety));
    safety->config = *config;
    safety->state = APP_MANUAL_DRIVE_DISABLED;
    safety->initialized = true;
    return true;
}

void safety_manual_drive_update(
    safety_manual_drive_t *safety,
    const app_manual_drive_request_t *request,
    const rc_snapshot_t *rc_snapshot,
    const safety_rc_status_t *rc_status,
    const actuator_snapshot_t *actuator_snapshot,
    uint64_t now_us,
    app_manual_drive_snapshot_t *snapshot)
{
    if ((safety == NULL) || !safety->initialized ||
        (request == NULL) || (rc_snapshot == NULL) ||
        (rc_status == NULL) || (actuator_snapshot == NULL) ||
        (snapshot == NULL))
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->timestamp_us = now_us;
    snapshot->actuator_index = request->actuator_index;

    const bool request_fresh = safety_manual_drive_request_fresh(
        request,
        now_us,
        safety->config.request_max_age_us);
    const uint16_t stick_value =
        rc_snapshot->channel[safety->config.velocity_channel];
    const bool stick_neutral = safety_manual_drive_stick_neutral(
        &safety->config,
        stick_value);
    const uint32_t base_inhibits = safety_manual_drive_inhibits(
        safety,
        request,
        request_fresh,
        rc_snapshot,
        rc_status,
        actuator_snapshot,
        stick_neutral);
    const uint32_t hard_inhibits = base_inhibits &
        ~(APP_MANUAL_DRIVE_INHIBIT_STICK |
          APP_MANUAL_DRIVE_INHIBIT_REARM);

    if (!request_fresh || !request->enabled)
    {
        safety->request_cycle_required = false;

        if (safety->state == APP_MANUAL_DRIVE_ARMED)
        {
            safety_manual_drive_begin_stop(safety, now_us, false);
        }
        else if ((safety->state == APP_MANUAL_DRIVE_STOPPING) &&
                 (now_us < safety->stop_until_us))
        {
            /* Hold the zero command until the stop window expires. */
        }
        else
        {
            safety->state = APP_MANUAL_DRIVE_DISABLED;
        }
    }
    else if (hard_inhibits != APP_MANUAL_DRIVE_INHIBIT_NONE)
    {
        if (safety->state == APP_MANUAL_DRIVE_ARMED)
        {
            safety_manual_drive_begin_stop(safety, now_us, true);
        }
        else if ((safety->state == APP_MANUAL_DRIVE_STOPPING) &&
                 (now_us < safety->stop_until_us))
        {
            /* Hold the zero command until the stop window expires. */
        }
        else
        {
            safety->state = APP_MANUAL_DRIVE_WAIT_SAFE;
        }
    }
    else if (safety->state == APP_MANUAL_DRIVE_STOPPING)
    {
        if (now_us >= safety->stop_until_us)
        {
            safety->state = APP_MANUAL_DRIVE_WAIT_NEUTRAL;
        }
    }
    else if (safety->state == APP_MANUAL_DRIVE_ARMED)
    {
        if (request->actuator_index != safety->actuator_index)
        {
            safety_manual_drive_begin_stop(safety, now_us, true);
        }
    }
    else if (safety->request_cycle_required)
    {
        safety->state = APP_MANUAL_DRIVE_WAIT_NEUTRAL;
    }
    else if (!stick_neutral)
    {
        safety->state = APP_MANUAL_DRIVE_WAIT_NEUTRAL;
    }
    else
    {
        safety->state = APP_MANUAL_DRIVE_ARMED;
        safety->actuator_index = request->actuator_index;
    }

    snapshot->state = safety->state;
    snapshot->inhibit_flags = base_inhibits;

    if (safety->request_cycle_required && request->enabled)
    {
        snapshot->inhibit_flags |= APP_MANUAL_DRIVE_INHIBIT_REARM;
    }

    if ((safety->state == APP_MANUAL_DRIVE_ARMED) &&
        (safety->actuator_index == request->actuator_index))
    {
        snapshot->actuator_index = safety->actuator_index;
        snapshot->velocity_erpm = safety_manual_drive_velocity(
            &safety->config,
            stick_value);
        snapshot->transmit_enabled = true;
        snapshot->motion_allowed = true;
        snapshot->inhibit_flags = APP_MANUAL_DRIVE_INHIBIT_NONE;
    }
    else if (safety->state == APP_MANUAL_DRIVE_STOPPING)
    {
        snapshot->actuator_index = safety->actuator_index;
        snapshot->velocity_erpm = 0.0f;
        snapshot->transmit_enabled = true;
        snapshot->motion_allowed = false;
    }
}

static bool safety_manual_drive_config_valid(
    const safety_manual_drive_config_t *config)
{
    return (config != NULL) &&
           (config->velocity_channel < APP_RC_CHANNEL_COUNT) &&
           (config->channel_min < config->stick_neutral_low) &&
           (config->stick_neutral_low <= config->stick_neutral_high) &&
           (config->stick_neutral_high < config->channel_max) &&
           (config->request_max_age_us > 0u) &&
           (config->stop_hold_us > 0u) &&
           (config->max_velocity_erpm > 0.0f);
}

static bool safety_manual_drive_request_fresh(
    const app_manual_drive_request_t *request,
    uint64_t now_us,
    uint64_t max_age_us)
{
    return (request->timestamp_us != 0u) &&
           (now_us >= request->timestamp_us) &&
           ((now_us - request->timestamp_us) <= max_age_us);
}

static bool safety_manual_drive_stick_neutral(
    const safety_manual_drive_config_t *config,
    uint16_t value)
{
    return (value >= config->stick_neutral_low) &&
           (value <= config->stick_neutral_high);
}

static float safety_manual_drive_velocity(
    const safety_manual_drive_config_t *config,
    uint16_t value)
{
    float normalized = 0.0f;

    if (value < config->stick_neutral_low)
    {
        normalized = -((float)config->stick_neutral_low - (float)value) /
            ((float)config->stick_neutral_low -
             (float)config->channel_min);
    }
    else if (value > config->stick_neutral_high)
    {
        normalized = ((float)value - (float)config->stick_neutral_high) /
            ((float)config->channel_max -
             (float)config->stick_neutral_high);
    }

    if (normalized < -1.0f)
    {
        normalized = -1.0f;
    }
    else if (normalized > 1.0f)
    {
        normalized = 1.0f;
    }

    if (!config->velocity_positive_high)
    {
        normalized = -normalized;
    }

    return normalized * config->max_velocity_erpm;
}

static uint32_t safety_manual_drive_inhibits(
    const safety_manual_drive_t *safety,
    const app_manual_drive_request_t *request,
    bool request_fresh,
    const rc_snapshot_t *rc_snapshot,
    const safety_rc_status_t *rc_status,
    const actuator_snapshot_t *actuator_snapshot,
    bool stick_neutral)
{
    uint32_t flags = APP_MANUAL_DRIVE_INHIBIT_NONE;

    if (!request->enabled)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_REQUEST;
    }

    if (!request_fresh)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_REQUEST_STALE;
    }

    if (!rc_status->safe_for_control || !rc_snapshot->valid)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_RC;
    }

    if (request->actuator_index >= APP_ACTUATOR_COUNT)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_ACTUATOR;
        return flags;
    }

    const actuator_feedback_snapshot_t *feedback =
        &actuator_snapshot->actuator[request->actuator_index];

    if (!feedback->configured)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_ACTUATOR;
    }

    if (!actuator_snapshot->bus_initialized ||
        (actuator_snapshot->bus_state != APP_ACTUATOR_BUS_ACTIVE))
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_BUS;
    }

    if (!feedback->valid)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_FEEDBACK;
    }

    if (feedback->fault_code != 0u)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_MOTOR_FAULT;
    }

    if (!stick_neutral &&
        (safety->state != APP_MANUAL_DRIVE_ARMED))
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_STICK;
    }

    if (safety->request_cycle_required)
    {
        flags |= APP_MANUAL_DRIVE_INHIBIT_REARM;
    }

    return flags;
}

static void safety_manual_drive_begin_stop(
    safety_manual_drive_t *safety,
    uint64_t now_us,
    bool require_request_cycle)
{
    safety->state = APP_MANUAL_DRIVE_STOPPING;
    safety->stop_until_us = now_us + safety->config.stop_hold_us;
    safety->request_cycle_required = require_request_cycle;
}
