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
    uint8_t axis_channel;
    uint8_t interact_channel;
    uint8_t enter_channel;

    uint16_t channel_min;
    uint16_t channel_max;

    uint16_t axis_left_threshold;
    uint16_t axis_neutral_low;
    uint16_t axis_neutral_high;
    uint16_t axis_right_threshold;

    uint16_t switch_off_threshold;
    uint16_t switch_on_threshold;

    bool interact_active_high;
    bool enter_active_high;
} ui_rc_input_config_t;

typedef struct
{
    ui_rc_input_config_t config;

    bool initialized;
    bool axis_armed;

    bool interact_raw_active;
    bool interact_armed;

    bool enter_raw_active;
    bool enter_armed;
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
