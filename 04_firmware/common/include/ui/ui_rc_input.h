#ifndef UI_RC_INPUT_H
#define UI_RC_INPUT_H

#include "app/app_rc_types.h"
#include "ui/ui_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t page_axis_channel;
    uint8_t vertical_axis_channel;
    uint8_t interact_channel;
    uint8_t enter_channel;
    uint8_t input_enable_channel;

    uint16_t channel_min;
    uint16_t channel_max;

    uint16_t axis_low_threshold;
    uint16_t axis_neutral_low;
    uint16_t axis_neutral_high;
    uint16_t axis_high_threshold;

    uint16_t action_off_threshold;
    uint16_t action_on_threshold;
    uint16_t enable_off_threshold;
    uint16_t enable_on_threshold;

    bool page_right_high;
    bool vertical_up_high;
    bool interact_active_high;
    bool enter_active_high;
    bool input_enable_active_high;
} ui_rc_input_config_t;

typedef struct
{
    ui_rc_input_config_t config;

    bool initialized;
    bool page_axis_armed;
    bool vertical_axis_armed;

    bool interact_raw_active;
    bool interact_armed;

    bool enter_raw_active;
    bool enter_armed;

    bool input_enable_raw_active;
    bool input_enable_armed;
} ui_rc_input_t;

bool ui_rc_input_init(ui_rc_input_t *input,
                      const ui_rc_input_config_t *config);

ui_event_flags_t ui_rc_input_update(ui_rc_input_t *input,
                                    const rc_snapshot_t *snapshot,
                                    bool link_usable);

#ifdef __cplusplus
}
#endif

#endif
