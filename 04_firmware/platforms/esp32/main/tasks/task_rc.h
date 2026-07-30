#ifndef TASK_RC_H
#define TASK_RC_H

#include "app/app_rc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_rc_start(void);
void task_rc_get_snapshot(rc_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif
