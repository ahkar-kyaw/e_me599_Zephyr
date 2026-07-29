#ifndef PROTO_CRSF_H
#define PROTO_CRSF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROTO_CRSF_CHANNEL_COUNT  16u
#define PROTO_CRSF_MAX_FRAME_SIZE 64u

#define PROTO_CRSF_CHANNEL_VALUE_MIN   172u
#define PROTO_CRSF_CHANNEL_VALUE_1000  191u
#define PROTO_CRSF_CHANNEL_VALUE_MID   992u
#define PROTO_CRSF_CHANNEL_VALUE_2000 1792u
#define PROTO_CRSF_CHANNEL_VALUE_MAX  1811u

typedef struct
{
    uint16_t channel[PROTO_CRSF_CHANNEL_COUNT];
} proto_crsf_channels_t;

typedef struct
{
    uint8_t frame[PROTO_CRSF_MAX_FRAME_SIZE];
    uint8_t index;
    uint8_t expected_size;
    uint32_t frame_count;
    uint32_t rc_frame_count;
    uint32_t crc_error_count;
    uint32_t parse_error_count;
} proto_crsf_parser_t;

void proto_crsf_init(proto_crsf_parser_t *parser);

bool proto_crsf_parse_byte(proto_crsf_parser_t *parser,
                           uint8_t byte,
                           proto_crsf_channels_t *channels);

#ifdef __cplusplus
}
#endif

#endif
