#ifndef TASK_SUPERVISOR_H
#define TASK_SUPERVISOR_H

#include "app/app_supervisor_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_supervisor_start(void);
void task_supervisor_get_snapshot(
    app_supervisor_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
