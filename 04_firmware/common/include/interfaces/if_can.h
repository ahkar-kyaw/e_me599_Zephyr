#ifndef IF_CAN_H
#define IF_CAN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_CAN_CLASSIC_MAX_DATA_LENGTH 8u

typedef struct
{
    uint32_t id;
    uint8_t data[IF_CAN_CLASSIC_MAX_DATA_LENGTH];
    uint8_t data_length;
    bool is_extended;
    bool is_remote;
} if_can_frame_t;

#ifdef __cplusplus
}
#endif

#endif
