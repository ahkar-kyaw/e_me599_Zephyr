#ifndef TASK_SAFETY_H
#define TASK_SAFETY_H

#include "app/app_manual_drive_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_safety_start(void);
void task_safety_get_manual_drive_snapshot(
    app_manual_drive_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
