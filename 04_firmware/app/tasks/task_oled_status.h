#ifndef TASK_OLED_STATUS_H
#define TASK_OLED_STATUS_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool imu_valid;
    bool crsf_link_valid;
    bool can_motor_valid;
    bool battery_low;
    bool estop_active;
    bool armed;

    uint16_t battery_mv;
    int16_t pitch_cdeg;
    int16_t roll_cdeg;

    uint32_t safety_fault_flags;
    uint32_t heartbeat;
} task_oled_status_t;

bool task_oled_status_start(I2C_HandleTypeDef *hi2c);
bool task_oled_status_publish(const task_oled_status_t *status);
bool task_oled_status_set_user_line(const char *text);

#ifdef __cplusplus
}
#endif

#endif