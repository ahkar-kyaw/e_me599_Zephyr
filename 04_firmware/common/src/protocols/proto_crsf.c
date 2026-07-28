#include "protocols/proto_crsf.h"

#include <stddef.h>
#include <string.h>

#define PROTO_CRSF_FRAME_TYPE_RC_CHANNELS_PACKED 0x16u
#define PROTO_CRSF_FRAME_LENGTH_MIN              2u
#define PROTO_CRSF_FRAME_LENGTH_MAX              62u
#define PROTO_CRSF_RC_FRAME_LENGTH               24u
#define PROTO_CRSF_RC_PAYLOAD_LENGTH             22u

static bool proto_crsf_is_address(uint8_t byte);
static uint8_t proto_crsf_crc8(const uint8_t *data, uint8_t length);
static void proto_crsf_unpack_channels(
    const uint8_t payload[PROTO_CRSF_RC_PAYLOAD_LENGTH],
    proto_crsf_channels_t *channels);
static void proto_crsf_reset_frame(proto_crsf_parser_t *parser);

void proto_crsf_init(proto_crsf_parser_t *parser)
{
    if (parser == NULL)
    {
        return;
    }

    memset(parser, 0, sizeof(*parser));
}

bool proto_crsf_parse_byte(proto_crsf_parser_t *parser,
                           uint8_t byte,
                           proto_crsf_channels_t *channels)
{
    if (parser == NULL)
    {
        return false;
    }

    if (parser->index == 0u)
    {
        if (proto_crsf_is_address(byte))
        {
            parser->frame[0] = byte;
            parser->index = 1u;
        }

        return false;
    }

    if (parser->index == 1u)
    {
        if ((byte < PROTO_CRSF_FRAME_LENGTH_MIN) ||
            (byte > PROTO_CRSF_FRAME_LENGTH_MAX))
        {
            parser->parse_error_count++;
            proto_crsf_reset_frame(parser);

            if (proto_crsf_is_address(byte))
            {
                parser->frame[0] = byte;
                parser->index = 1u;
            }

            return false;
        }

        parser->frame[1] = byte;
        parser->expected_size = (uint8_t)(byte + 2u);
        parser->index = 2u;
        return false;
    }

    if ((parser->index >= parser->expected_size) ||
        (parser->index >= PROTO_CRSF_MAX_FRAME_SIZE))
    {
        parser->parse_error_count++;
        proto_crsf_reset_frame(parser);
        return false;
    }

    parser->frame[parser->index] = byte;
    parser->index++;

    if (parser->index < parser->expected_size)
    {
        return false;
    }

    const uint8_t frame_length = parser->frame[1];
    const uint8_t received_crc = parser->frame[parser->expected_size - 1u];
    const uint8_t calculated_crc =
        proto_crsf_crc8(&parser->frame[2], (uint8_t)(frame_length - 1u));
    bool channels_ready = false;

    if (received_crc != calculated_crc)
    {
        parser->crc_error_count++;
    }
    else
    {
        parser->frame_count++;

        if ((parser->frame[2] ==
             PROTO_CRSF_FRAME_TYPE_RC_CHANNELS_PACKED) &&
            (frame_length == PROTO_CRSF_RC_FRAME_LENGTH) &&
            (channels != NULL))
        {
            proto_crsf_unpack_channels(&parser->frame[3], channels);
            parser->rc_frame_count++;
            channels_ready = true;
        }
    }

    proto_crsf_reset_frame(parser);
    return channels_ready;
}

static bool proto_crsf_is_address(uint8_t byte)
{
    switch (byte)
    {
        case 0x00u:
        case 0x10u:
        case 0x12u:
        case 0x80u:
        case 0xC0u:
        case 0xC2u:
        case 0xC4u:
        case 0xC8u:
        case 0xCAu:
        case 0xCCu:
        case 0xCEu:
        case 0xEAu:
        case 0xECu:
        case 0xEEu:
            return true;

        default:
            return false;
    }
}

static uint8_t proto_crsf_crc8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0u;

    while (length > 0u)
    {
        crc ^= *data;
        data++;
        length--;

        for (uint8_t bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x80u) != 0u)
            {
                crc = (uint8_t)((crc << 1u) ^ 0xD5u);
            }
            else
            {
                crc = (uint8_t)(crc << 1u);
            }
        }
    }

    return crc;
}

static void proto_crsf_unpack_channels(
    const uint8_t payload[PROTO_CRSF_RC_PAYLOAD_LENGTH],
    proto_crsf_channels_t *channels)
{
    for (uint8_t channel_index = 0u;
         channel_index < PROTO_CRSF_CHANNEL_COUNT;
         channel_index++)
    {
        const uint16_t bit_index = (uint16_t)channel_index * 11u;
        const uint8_t byte_index = (uint8_t)(bit_index / 8u);
        const uint8_t bit_offset = (uint8_t)(bit_index % 8u);
        uint32_t packed = payload[byte_index];

        if ((byte_index + 1u) < PROTO_CRSF_RC_PAYLOAD_LENGTH)
        {
            packed |= (uint32_t)payload[byte_index + 1u] << 8u;
        }

        if ((byte_index + 2u) < PROTO_CRSF_RC_PAYLOAD_LENGTH)
        {
            packed |= (uint32_t)payload[byte_index + 2u] << 16u;
        }

        channels->channel[channel_index] =
            (uint16_t)((packed >> bit_offset) & 0x07FFu);
    }
}

static void proto_crsf_reset_frame(proto_crsf_parser_t *parser)
{
    parser->index = 0u;
    parser->expected_size = 0u;
}
