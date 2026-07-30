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
static bool ui_rc_input_channels_valid(const ui_rc_input_t *input,
                                       const rc_snapshot_t *snapshot);
static void ui_rc_input_lock(ui_rc_input_t *input);
static void ui_rc_input_reset_state(ui_rc_input_t *input);
static bool ui_rc_input_config_valid(
    const ui_rc_input_config_t *config);

bool ui_rc_input_init(ui_rc_input_t *input,
                      const ui_rc_input_config_t *config)
{
    if ((input == NULL) || !ui_rc_input_config_valid(config))
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
    if ((input == NULL) || (snapshot == NULL) || !link_usable ||
        !ui_rc_input_channels_valid(input, snapshot))
    {
        if (input != NULL)
        {
            ui_rc_input_reset_state(input);
        }

        return UI_EVENT_LINK_LOST;
    }

    const uint16_t enable_value =
        snapshot->channel[input->config.input_enable_channel];

    if (!input->initialized)
    {
        input->input_enable_raw_active =
            ui_rc_input_switch_active(
                enable_value,
                false,
                input->config.input_enable_active_high,
                input->config.enable_off_threshold,
                input->config.enable_on_threshold);
        input->input_enable_armed =
            !input->input_enable_raw_active;
        input->initialized = true;
        return UI_EVENT_NONE;
    }

    const bool enable_active =
        ui_rc_input_switch_active(
            enable_value,
            input->input_enable_raw_active,
            input->config.input_enable_active_high,
            input->config.enable_off_threshold,
            input->config.enable_on_threshold);

    if (enable_active != input->input_enable_raw_active)
    {
        input->input_enable_raw_active = enable_active;

        if (!enable_active)
        {
            input->input_enable_armed = true;
            ui_rc_input_lock(input);
            return UI_EVENT_INPUT_DISABLED;
        }

        if (input->input_enable_armed)
        {
            const uint16_t page_value =
                snapshot->channel[input->config.page_axis_channel];
            const uint16_t vertical_value =
                snapshot->channel[input->config.vertical_axis_channel];
            const uint16_t interact_value =
                snapshot->channel[input->config.interact_channel];
            const uint16_t enter_value =
                snapshot->channel[input->config.enter_channel];

            input->input_enable_armed = false;
            input->page_axis_armed =
                ui_rc_input_axis_is_neutral(input, page_value);
            input->vertical_axis_armed =
                ui_rc_input_axis_is_neutral(input, vertical_value);
            input->interact_raw_active =
                ui_rc_input_switch_active(
                    interact_value,
                    false,
                    input->config.interact_active_high,
                    input->config.action_off_threshold,
                    input->config.action_on_threshold);
            input->interact_armed = !input->interact_raw_active;
            input->enter_raw_active =
                ui_rc_input_switch_active(
                    enter_value,
                    false,
                    input->config.enter_active_high,
                    input->config.action_off_threshold,
                    input->config.action_on_threshold);
            input->enter_armed = !input->enter_raw_active;
            return UI_EVENT_INPUT_ENABLED;
        }
    }

    if (!input->input_enable_raw_active)
    {
        return UI_EVENT_NONE;
    }

    const uint16_t page_value =
        snapshot->channel[input->config.page_axis_channel];
    const uint16_t vertical_value =
        snapshot->channel[input->config.vertical_axis_channel];
    const uint16_t interact_value =
        snapshot->channel[input->config.interact_channel];
    const uint16_t enter_value =
        snapshot->channel[input->config.enter_channel];
    ui_event_flags_t events = UI_EVENT_NONE;

    if (ui_rc_input_axis_is_neutral(input, page_value))
    {
        input->page_axis_armed = true;
    }
    else if (input->page_axis_armed &&
             (page_value <= input->config.axis_low_threshold))
    {
        events |= input->config.page_right_high
            ? UI_EVENT_LEFT
            : UI_EVENT_RIGHT;
        input->page_axis_armed = false;
    }
    else if (input->page_axis_armed &&
             (page_value >= input->config.axis_high_threshold))
    {
        events |= input->config.page_right_high
            ? UI_EVENT_RIGHT
            : UI_EVENT_LEFT;
        input->page_axis_armed = false;
    }

    if (ui_rc_input_axis_is_neutral(input, vertical_value))
    {
        input->vertical_axis_armed = true;
    }
    else if (input->vertical_axis_armed &&
             (vertical_value <= input->config.axis_low_threshold))
    {
        events |= input->config.vertical_up_high
            ? UI_EVENT_DOWN
            : UI_EVENT_UP;
        input->vertical_axis_armed = false;
    }
    else if (input->vertical_axis_armed &&
             (vertical_value >= input->config.axis_high_threshold))
    {
        events |= input->config.vertical_up_high
            ? UI_EVENT_UP
            : UI_EVENT_DOWN;
        input->vertical_axis_armed = false;
    }

    const bool interact_active =
        ui_rc_input_switch_active(
            interact_value,
            input->interact_raw_active,
            input->config.interact_active_high,
            input->config.action_off_threshold,
            input->config.action_on_threshold);

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
            input->config.action_off_threshold,
            input->config.action_on_threshold);

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

    if ((events &
         (UI_EVENT_INTERACT_ON | UI_EVENT_INTERACT_OFF)) != 0u)
    {
        events &= (UI_EVENT_INTERACT_ON | UI_EVENT_INTERACT_OFF);
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

static bool ui_rc_input_channels_valid(const ui_rc_input_t *input,
                                       const rc_snapshot_t *snapshot)
{
    const uint8_t channel[] =
    {
        input->config.page_axis_channel,
        input->config.vertical_axis_channel,
        input->config.interact_channel,
        input->config.enter_channel,
        input->config.input_enable_channel,
    };

    for (size_t i = 0u; i < (sizeof(channel) / sizeof(channel[0])); i++)
    {
        const uint16_t value = snapshot->channel[channel[i]];

        if ((value < input->config.channel_min) ||
            (value > input->config.channel_max))
        {
            return false;
        }
    }

    return true;
}

static void ui_rc_input_lock(ui_rc_input_t *input)
{
    input->page_axis_armed = false;
    input->vertical_axis_armed = false;
    input->interact_raw_active = false;
    input->interact_armed = false;
    input->enter_raw_active = false;
    input->enter_armed = false;
}

static void ui_rc_input_reset_state(ui_rc_input_t *input)
{
    const ui_rc_input_config_t config = input->config;

    memset(input, 0, sizeof(*input));
    input->config = config;
}

static bool ui_rc_input_config_valid(
    const ui_rc_input_config_t *config)
{
    if ((config == NULL) ||
        (config->page_axis_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->vertical_axis_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->interact_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->enter_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->input_enable_channel >= APP_RC_CHANNEL_COUNT) ||
        (config->channel_min >= config->channel_max) ||
        (config->axis_low_threshold <= config->channel_min) ||
        (config->axis_low_threshold >= config->axis_neutral_low) ||
        (config->axis_neutral_low > config->axis_neutral_high) ||
        (config->axis_neutral_high >= config->axis_high_threshold) ||
        (config->axis_high_threshold >= config->channel_max) ||
        (config->action_off_threshold <= config->channel_min) ||
        (config->action_off_threshold >=
         config->action_on_threshold) ||
        (config->action_on_threshold >= config->channel_max) ||
        (config->enable_off_threshold <= config->channel_min) ||
        (config->enable_off_threshold >=
         config->enable_on_threshold) ||
        (config->enable_on_threshold >= config->channel_max))
    {
        return false;
    }

    const uint8_t channel[] =
    {
        config->page_axis_channel,
        config->vertical_axis_channel,
        config->interact_channel,
        config->enter_channel,
        config->input_enable_channel,
    };

    for (size_t i = 0u; i < (sizeof(channel) / sizeof(channel[0])); i++)
    {
        for (size_t j = i + 1u;
             j < (sizeof(channel) / sizeof(channel[0]));
             j++)
        {
            if (channel[i] == channel[j])
            {
                return false;
            }
        }
    }

    return true;
}
