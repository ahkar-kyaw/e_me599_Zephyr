#ifndef CTRL_BALANCE_TYPES_H
#define CTRL_BALANCE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool valid;

    uint64_t timestamp_us;

    float pitch_rad;
    float pitch_rate_rps;

    float roll_rad;
    float roll_rate_rps;
} balance_state_t;

#ifdef __cplusplus
}
#endif

#endif