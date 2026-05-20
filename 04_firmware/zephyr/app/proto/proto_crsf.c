#include "proto_crsf.h"

#include <string.h>

#define CRSF_CRC_POLY               0xD5u
#define CRSF_LENGTH_MIN             2u
#define CRSF_LENGTH_MAX             62u
#define CRSF_RC_PAYLOAD_LEN         22u
#define CRSF_LINK_STATS_PAYLOAD_LEN 10u

static uint8_t crsf_crc8_dvb_s2_byte(uint8_t crc, uint8_t byte)
{
    crc ^= byte;

    for (uint8_t bit = 0u; bit < 8u; bit++)
    {
        if ((crc & 0x80u) != 0u)
        {
            crc = (uint8_t)((crc << 1u) ^ CRSF_CRC_POLY);
        }
        else
        {
            crc = (uint8_t)(crc << 1u);
        }
    }

    return crc;
}

static uint8_t crsf_crc8_dvb_s2(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0u;

    for (uint8_t i = 0u; i < len; i++)
    {
        crc = crsf_crc8_dvb_s2_byte(crc, data[i]);
    }

    return crc;
}

static bool crsf_is_plausible_length(uint8_t length)
{
    return (length >= CRSF_LENGTH_MIN) && (length <= CRSF_LENGTH_MAX);
}

void crsf_parser_init(crsf_parser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    memset(parser, 0, sizeof(*parser));
}

crsf_parse_result_t crsf_parser_push_byte(crsf_parser_t *parser, uint8_t byte, crsf_frame_t *out_frame)
{
    if ((parser == NULL) || (out_frame == NULL))
    {
        return CRSF_PARSE_NO_FRAME;
    }

    if (parser->index == 0u)
    {
        parser->buffer[parser->index++] = byte;
        return CRSF_PARSE_NO_FRAME;
    }

    if (parser->index == 1u)
    {
        if (!crsf_is_plausible_length(byte))
        {
            parser->index = 0u;
            parser->expected_total_len = 0u;
            parser->length_errors++;
            return CRSF_PARSE_FRAME_BAD_LENGTH;
        }

        parser->buffer[parser->index++] = byte;
        parser->expected_total_len = (uint8_t)(byte + 2u);
        return CRSF_PARSE_NO_FRAME;
    }

    if (parser->index >= CRSF_FRAME_MAX_LEN)
    {
        parser->index = 0u;
        parser->expected_total_len = 0u;
        parser->length_errors++;
        return CRSF_PARSE_FRAME_BAD_LENGTH;
    }

    parser->buffer[parser->index++] = byte;

    if (parser->index < parser->expected_total_len)
    {
        return CRSF_PARSE_NO_FRAME;
    }

    const uint8_t frame_length = parser->buffer[1];
    const uint8_t received_crc = parser->buffer[1u + frame_length];
    const uint8_t computed_crc = crsf_crc8_dvb_s2(&parser->buffer[2], (uint8_t)(frame_length - 1u));

    if (computed_crc != received_crc)
    {
        parser->index = 0u;
        parser->expected_total_len = 0u;
        parser->crc_errors++;
        return CRSF_PARSE_FRAME_BAD_CRC;
    }

    memset(out_frame, 0, sizeof(*out_frame));
    out_frame->address = parser->buffer[0];
    out_frame->length = parser->buffer[1];
    out_frame->type = parser->buffer[2];
    out_frame->payload_len = (uint8_t)(frame_length - 2u);

    if (out_frame->payload_len > 0u)
    {
        memcpy(out_frame->payload, &parser->buffer[3], out_frame->payload_len);
    }

    parser->index = 0u;
    parser->expected_total_len = 0u;
    parser->frames_ok++;

    return CRSF_PARSE_FRAME_OK;
}

bool crsf_decode_channels(const crsf_frame_t *frame, crsf_channels_t *out_channels)
{
    if ((frame == NULL) || (out_channels == NULL))
    {
        return false;
    }

    if ((frame->type != CRSF_FRAME_TYPE_RC_CHANNELS) || (frame->payload_len < CRSF_RC_PAYLOAD_LEN))
    {
        return false;
    }

    uint32_t bit_buffer = 0u;
    uint8_t bits_in_buffer = 0u;
    uint8_t payload_index = 0u;

    for (uint8_t channel = 0u; channel < CRSF_CHANNEL_COUNT; channel++)
    {
        while (bits_in_buffer < 11u)
        {
            bit_buffer |= ((uint32_t)frame->payload[payload_index]) << bits_in_buffer;
            bits_in_buffer = (uint8_t)(bits_in_buffer + 8u);
            payload_index++;
        }

        out_channels->raw[channel] = (uint16_t)(bit_buffer & 0x07FFu);
        bit_buffer >>= 11u;
        bits_in_buffer = (uint8_t)(bits_in_buffer - 11u);
    }

    out_channels->valid = true;
    return true;
}

bool crsf_decode_link_statistics(const crsf_frame_t *frame, crsf_link_statistics_t *out_stats)
{
    if ((frame == NULL) || (out_stats == NULL))
    {
        return false;
    }

    if ((frame->type != CRSF_FRAME_TYPE_LINK_STATISTICS) || (frame->payload_len < CRSF_LINK_STATS_PAYLOAD_LEN))
    {
        return false;
    }

    out_stats->uplink_rssi_1 = frame->payload[0];
    out_stats->uplink_rssi_2 = frame->payload[1];
    out_stats->uplink_link_quality = frame->payload[2];
    out_stats->uplink_snr = (int8_t)frame->payload[3];
    out_stats->active_antenna = frame->payload[4];
    out_stats->rf_mode = frame->payload[5];
    out_stats->uplink_tx_power = frame->payload[6];
    out_stats->downlink_rssi = frame->payload[7];
    out_stats->downlink_link_quality = frame->payload[8];
    out_stats->downlink_snr = (int8_t)frame->payload[9];
    out_stats->valid = true;

    return true;
}

int16_t crsf_channel_to_milli(uint16_t raw)
{
    int32_t value = 0;

    if (raw >= CRSF_RC_CHANNEL_MID)
    {
        value = ((int32_t)(raw - CRSF_RC_CHANNEL_MID) * 1000) /
                (int32_t)(CRSF_RC_CHANNEL_MAX - CRSF_RC_CHANNEL_MID);
    }
    else
    {
        value = -(((int32_t)(CRSF_RC_CHANNEL_MID - raw) * 1000) /
                  (int32_t)(CRSF_RC_CHANNEL_MID - CRSF_RC_CHANNEL_MIN));
    }

    if ((value > -(int32_t)CRSF_RC_CHANNEL_DEADBAND) && (value < (int32_t)CRSF_RC_CHANNEL_DEADBAND))
    {
        value = 0;
    }

    if (value > 1000)
    {
        value = 1000;
    }
    else if (value < -1000)
    {
        value = -1000;
    }

    return (int16_t)value;
}

bool crsf_channel_is_switch_high(uint16_t raw)
{
    return raw > 1500u;
}

bool crsf_channel_is_switch_low(uint16_t raw)
{
    return raw < 500u;
}
