#include "ui/ui_rc_input.h"

#include <stddef.h>
#include <string.h>

static bool ui_rc_input_axis_is_neutral(const ui_rc_input_t *input,
                                        uint16_t value);
static bool ui_rc_input_switch_active(uint16_t value,
                                      bool current_active,
                                      bool active_high,
                                      uint16_t off_threshold,
                                      uint16_t on_threshold);

bool ui_rc_input_init(ui_rc_input_t *input,
                      const ui_rc_input_config_t *config)
{
    if ((input == NULL) || (config == NULL) ||
        (config->axis_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->interact_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->enter_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->channel_min >= config->channel_max) ||
        (config->axis_left_threshold <= config->channel_min) ||
        (config->axis_left_threshold >= config->axis_neutral_low) ||
        (config->axis_neutral_low > config->axis_neutral_high) ||
        (config->axis_neutral_high >= config->axis_right_threshold) ||
        (config->axis_right_threshold >= config->channel_max) ||
        (config->switch_off_threshold <= config->channel_min) ||
        (config->switch_off_threshold >= config->switch_on_threshold) ||
        (config->switch_on_threshold >= config->channel_max))
    {
        return false;
    }

    memset(input, 0, sizeof(*input));
    input->config = *config;

    return true;
}

ui_event_flags_t ui_rc_input_update(ui_rc_input_t *input,
                                    const rc_snapshot_t *snapshot,
                                    bool link_usable)
{
    if ((input == NULL) || (snapshot == NULL) || !link_usable)
    {
        if (input != NULL)
        {
            input->initialized = false;
            input->axis_armed = false;
            input->interact_armed = false;
            input->enter_armed = false;
        }

        return UI_EVENT_LINK_LOST;
    }

    const uint16_t axis_value =
        snapshot->channel[input->config.axis_channel];
    const uint16_t interact_value =
        snapshot->channel[input->config.interact_channel];
    const uint16_t enter_value =
        snapshot->channel[input->config.enter_channel];

    if ((axis_value < input->config.channel_min) ||
        (axis_value > input->config.channel_max) ||
        (interact_value < input->config.channel_min) ||
        (interact_value > input->config.channel_max) ||
        (enter_value < input->config.channel_min) ||
        (enter_value > input->config.channel_max))
    {
        input->initialized = false;
        input->axis_armed = false;
        input->interact_armed = false;
        input->enter_armed = false;
        return UI_EVENT_LINK_LOST;
    }

    if (!input->initialized)
    {
        input->axis_armed =
            ui_rc_input_axis_is_neutral(input, axis_value);

        input->interact_raw_active =
            ui_rc_input_switch_active(
                interact_value,
                false,
                input->config.interact_active_high,
                input->config.switch_off_threshold,
                input->config.switch_on_threshold);
        input->interact_armed = !input->interact_raw_active;

        input->enter_raw_active =
            ui_rc_input_switch_active(
                enter_value,
                false,
                input->config.enter_active_high,
                input->config.switch_off_threshold,
                input->config.switch_on_threshold);
        input->enter_armed = !input->enter_raw_active;

        input->initialized = true;
        return UI_EVENT_NONE;
    }

    ui_event_flags_t events = UI_EVENT_NONE;

    if (ui_rc_input_axis_is_neutral(input, axis_value))
    {
        input->axis_armed = true;
    }
    else if (input->axis_armed &&
             (axis_value <= input->config.axis_left_threshold))
    {
        events |= UI_EVENT_LEFT;
        input->axis_armed = false;
    }
    else if (input->axis_armed &&
             (axis_value >= input->config.axis_right_threshold))
    {
        events |= UI_EVENT_RIGHT;
        input->axis_armed = false;
    }

    const bool interact_active =
        ui_rc_input_switch_active(
            interact_value,
            input->interact_raw_active,
            input->config.interact_active_high,
            input->config.switch_off_threshold,
            input->config.switch_on_threshold);

    if (interact_active != input->interact_raw_active)
    {
        input->interact_raw_active = interact_active;

        if (!interact_active)
        {
            input->interact_armed = true;
            events |= UI_EVENT_INTERACT_OFF;
        }
        else if (input->interact_armed)
        {
            input->interact_armed = false;
            events |= UI_EVENT_INTERACT_ON;
        }
    }

    const bool enter_active =
        ui_rc_input_switch_active(
            enter_value,
            input->enter_raw_active,
            input->config.enter_active_high,
            input->config.switch_off_threshold,
            input->config.switch_on_threshold);

    if (enter_active != input->enter_raw_active)
    {
        input->enter_raw_active = enter_active;

        if (!enter_active)
        {
            input->enter_armed = true;
        }
        else if (input->enter_armed)
        {
            input->enter_armed = false;
            events |= UI_EVENT_ENTER;
        }
    }

    return events;
}

static bool ui_rc_input_axis_is_neutral(const ui_rc_input_t *input,
                                        uint16_t value)
{
    return (value >= input->config.axis_neutral_low) &&
           (value <= input->config.axis_neutral_high);
}

static bool ui_rc_input_switch_active(uint16_t value,
                                      bool current_active,
                                      bool active_high,
                                      uint16_t off_threshold,
                                      uint16_t on_threshold)
{
    if (active_high)
    {
        if (!current_active && (value >= on_threshold))
        {
            return true;
        }

        if (current_active && (value <= off_threshold))
        {
            return false;
        }
    }
    else
    {
        if (!current_active && (value <= off_threshold))
        {
            return true;
        }

        if (current_active && (value >= on_threshold))
        {
            return false;
        }
    }

    return current_active;
}
