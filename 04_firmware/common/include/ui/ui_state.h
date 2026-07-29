#ifndef UI_STATE_H
#define UI_STATE_H

#include "ui/ui_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_state_init(ui_state_t *state);

bool ui_state_update(ui_state_t *state, ui_event_flags_t events);

uint8_t ui_state_subpage_count(ui_page_t page);

#ifdef __cplusplus
}
#endif

#endif
