#ifndef PROTO_CRSF_H
#define PROTO_CRSF_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRSF_FRAME_MAX_LEN              64u
#define CRSF_CHANNEL_COUNT              16u
#define CRSF_RC_CHANNEL_MIN             172u
#define CRSF_RC_CHANNEL_MID             992u
#define CRSF_RC_CHANNEL_MAX             1811u
#define CRSF_RC_CHANNEL_DEADBAND        8u

#define CRSF_FRAME_TYPE_LINK_STATISTICS 0x14u
#define CRSF_FRAME_TYPE_RC_CHANNELS     0x16u

typedef enum
{
    CRSF_PARSE_NO_FRAME = 0,
    CRSF_PARSE_FRAME_OK,
    CRSF_PARSE_FRAME_BAD_CRC,
    CRSF_PARSE_FRAME_BAD_LENGTH,
    CRSF_PARSE_FRAME_UNSUPPORTED
} crsf_parse_result_t;

typedef struct
{
    uint16_t raw[CRSF_CHANNEL_COUNT];
    bool valid;
} crsf_channels_t;

typedef struct
{
    uint8_t uplink_rssi_1;
    uint8_t uplink_rssi_2;
    uint8_t uplink_link_quality;
    int8_t uplink_snr;
    uint8_t active_antenna;
    uint8_t rf_mode;
    uint8_t uplink_tx_power;
    uint8_t downlink_rssi;
    uint8_t downlink_link_quality;
    int8_t downlink_snr;
    bool valid;
} crsf_link_statistics_t;

typedef struct
{
    uint8_t address;
    uint8_t length;
    uint8_t type;
    uint8_t payload[CRSF_FRAME_MAX_LEN];
    uint8_t payload_len;
} crsf_frame_t;

typedef struct
{
    uint8_t buffer[CRSF_FRAME_MAX_LEN];
    uint8_t index;
    uint8_t expected_total_len;
    uint32_t frames_ok;
    uint32_t crc_errors;
    uint32_t length_errors;
} crsf_parser_t;

void crsf_parser_init(crsf_parser_t *parser);
crsf_parse_result_t crsf_parser_push_byte(crsf_parser_t *parser, uint8_t byte, crsf_frame_t *out_frame);
bool crsf_decode_channels(const crsf_frame_t *frame, crsf_channels_t *out_channels);
bool crsf_decode_link_statistics(const crsf_frame_t *frame, crsf_link_statistics_t *out_stats);
int16_t crsf_channel_to_milli(uint16_t raw);
bool crsf_channel_is_switch_high(uint16_t raw);
bool crsf_channel_is_switch_low(uint16_t raw);

#ifdef __cplusplus
}
#endif

#endif
