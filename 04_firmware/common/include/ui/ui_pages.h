#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "app/app_imu_types.h"
#include "app/app_rc_types.h"
#include "drivers/drv_ssd1306.h"
#include "safety/safety_imu.h"
#include "safety/safety_rc.h"
#include "ui/ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const rc_snapshot_t *rc_snapshot;
    const safety_rc_status_t *rc_status;
    const imu_snapshot_t *imu_snapshot;
    const safety_imu_status_t *imu_status;
} ui_page_model_t;

void ui_pages_render(drv_ssd1306_t *display,
                     const ui_state_t *state,
                     const ui_page_model_t *model);

#ifdef __cplusplus
}
#endif

#endif
