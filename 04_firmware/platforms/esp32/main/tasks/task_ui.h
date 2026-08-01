#ifndef TASK_UI_H
#define TASK_UI_H

#include "app/app_manual_drive_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_ui_start(void);
void task_ui_get_manual_drive_request(
    app_manual_drive_request_t *request);

#ifdef __cplusplus
}
#endif

#endif
