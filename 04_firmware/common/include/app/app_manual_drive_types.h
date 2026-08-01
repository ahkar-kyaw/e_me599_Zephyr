#ifndef APP_MANUAL_DRIVE_TYPES_H
#define APP_MANUAL_DRIVE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    APP_MANUAL_DRIVE_DISABLED = 0,
    APP_MANUAL_DRIVE_WAIT_SAFE,
    APP_MANUAL_DRIVE_WAIT_NEUTRAL,
    APP_MANUAL_DRIVE_ARMED,
    APP_MANUAL_DRIVE_STOPPING
} app_manual_drive_state_t;

typedef enum
{
    APP_MANUAL_DRIVE_INHIBIT_NONE = 0u,
    APP_MANUAL_DRIVE_INHIBIT_REQUEST = 1u << 0,
    APP_MANUAL_DRIVE_INHIBIT_REQUEST_STALE = 1u << 1,
    APP_MANUAL_DRIVE_INHIBIT_RC = 1u << 2,
    APP_MANUAL_DRIVE_INHIBIT_ACTUATOR = 1u << 3,
    APP_MANUAL_DRIVE_INHIBIT_BUS = 1u << 4,
    APP_MANUAL_DRIVE_INHIBIT_FEEDBACK = 1u << 5,
    APP_MANUAL_DRIVE_INHIBIT_MOTOR_FAULT = 1u << 6,
    APP_MANUAL_DRIVE_INHIBIT_STICK = 1u << 7,
    APP_MANUAL_DRIVE_INHIBIT_REARM = 1u << 8
} app_manual_drive_inhibit_t;

typedef struct
{
    uint64_t timestamp_us;
    uint8_t actuator_index;
    bool enabled;
} app_manual_drive_request_t;

typedef struct
{
    uint64_t timestamp_us;
    float velocity_erpm;
    uint32_t inhibit_flags;
    app_manual_drive_state_t state;
    uint8_t actuator_index;
    bool transmit_enabled;
    bool motion_allowed;
} app_manual_drive_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif
