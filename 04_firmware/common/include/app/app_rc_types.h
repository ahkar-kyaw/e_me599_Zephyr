#ifndef APP_RC_TYPES_H
#define APP_RC_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_RC_CHANNEL_COUNT 16u

typedef struct
{
    uint16_t channel[APP_RC_CHANNEL_COUNT];
    uint64_t timestamp_us;
    uint32_t frame_count;
    uint32_t crc_error_count;
    uint32_t parse_error_count;
    uint32_t uart_error_count;
    bool valid;
} rc_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif
