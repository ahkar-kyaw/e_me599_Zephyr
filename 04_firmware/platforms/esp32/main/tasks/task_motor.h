#ifndef TASK_MOTOR_H
#define TASK_MOTOR_H

#include "app/app_actuator_types.h"
#include "app/app_manual_drive_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void task_motor_start(void);
void task_motor_get_snapshot(actuator_snapshot_t *snapshot);
void task_motor_set_approved_manual_command(
    const app_manual_drive_snapshot_t *command);

#ifdef __cplusplus
}
#endif

#endif
