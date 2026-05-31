#include "drv_crsf.h"

#include <string.h>

static UART_HandleTypeDef *s_huart = NULL;
static uint8_t *s_dma_buffer = NULL;
static uint16_t s_dma_buffer_len = 0u;
static uint16_t s_dma_read_index = 0u;

static volatile bool s_uart_recover_pending = false;

static crsf_parser_t s_parser;
static drv_crsf_state_t s_state;

static bool drv_crsf_restart_dma(void)
{
    if ((s_huart == NULL) || (s_dma_buffer == NULL) || (s_dma_buffer_len == 0u))
    {
        return false;
    }

    (void)HAL_UART_AbortReceive(s_huart);

    if (HAL_UART_Receive_DMA(s_huart, s_dma_buffer, s_dma_buffer_len) != HAL_OK)
    {
        return false;
    }

    if (s_huart->hdmarx != NULL)
    {
        __HAL_DMA_DISABLE_IT(s_huart->hdmarx, DMA_IT_HT);
    }

    s_dma_read_index = 0u;
    s_state.dma_read_index = 0u;
    s_state.dma_write_index = 0u;
    s_state.dma_restart_count++;
    s_uart_recover_pending = false;

    return true;
}

static uint16_t drv_crsf_dma_write_index(void)
{
    if ((s_huart == NULL) || (s_huart->hdmarx == NULL) || (s_dma_buffer_len == 0u))
    {
        return 0u;
    }

    const uint16_t remaining = (uint16_t)__HAL_DMA_GET_COUNTER(s_huart->hdmarx);

    if (remaining > s_dma_buffer_len)
    {
        return 0u;
    }

    return (uint16_t)(s_dma_buffer_len - remaining);
}

static void drv_crsf_handle_frame(const crsf_frame_t *frame)
{
    if (frame == NULL)
    {
        return;
    }

    s_state.valid_frame_count++;

    if (frame->type == CRSF_FRAME_TYPE_RC_CHANNELS)
    {
        crsf_channels_t channels;

        memset(&channels, 0, sizeof(channels));

        if (crsf_decode_channels(frame, &channels))
        {
            s_state.channels = channels;
            s_state.last_rc_frame_tick_ms = HAL_GetTick();
            s_state.valid_rc_frame_count++;
            s_state.receiver_connected = true;
        }
    }
    else if (frame->type == CRSF_FRAME_TYPE_LINK_STATISTICS)
    {
        crsf_link_statistics_t link_stats;

        memset(&link_stats, 0, sizeof(link_stats));

        if (crsf_decode_link_statistics(frame, &link_stats))
        {
            s_state.link_stats = link_stats;
        }
    }
    else
    {
        /* Valid CRSF frame, but not one used by this driver yet. */
    }
}

bool drv_crsf_init(UART_HandleTypeDef *huart, uint8_t *dma_buffer, uint16_t dma_buffer_len)
{
    if ((huart == NULL) || (dma_buffer == NULL) || (dma_buffer_len < 64u))
    {
        return false;
    }

    s_huart = huart;
    s_dma_buffer = dma_buffer;
    s_dma_buffer_len = dma_buffer_len;
    s_dma_read_index = 0u;
    s_uart_recover_pending = false;

    memset(&s_state, 0, sizeof(s_state));
    crsf_parser_init(&s_parser);

    return true;
}

bool drv_crsf_start(void)
{
    return drv_crsf_restart_dma();
}

void drv_crsf_process(void)
{
    if ((s_huart == NULL) || (s_dma_buffer == NULL) || (s_dma_buffer_len == 0u))
    {
        return;
    }

    if (s_uart_recover_pending || (s_huart->RxState != HAL_UART_STATE_BUSY_RX))
    {
        (void)drv_crsf_restart_dma();
    }

    const uint16_t write_index = drv_crsf_dma_write_index();

    s_state.dma_write_index = write_index;
    s_state.dma_read_index = s_dma_read_index;

    while (s_dma_read_index != write_index)
    {
        const uint8_t byte = s_dma_buffer[s_dma_read_index];

        s_state.rx_byte_count++;
        s_state.last_rx_byte = byte;

        s_dma_read_index++;

        if (s_dma_read_index >= s_dma_buffer_len)
        {
            s_dma_read_index = 0u;
        }

        crsf_frame_t frame;
        const crsf_parse_result_t result = crsf_parser_push_byte(&s_parser, byte, &frame);

        if (result == CRSF_PARSE_FRAME_OK)
        {
            drv_crsf_handle_frame(&frame);
        }
        else if (result == CRSF_PARSE_FRAME_BAD_CRC)
        {
            s_state.crc_error_count++;
        }
        else if (result == CRSF_PARSE_FRAME_BAD_LENGTH)
        {
            s_state.length_error_count++;
        }
        else
        {
            /* No complete frame yet. */
        }
    }

    s_state.dma_write_index = write_index;
    s_state.dma_read_index = s_dma_read_index;

    if (s_state.valid_rc_frame_count == 0u)
    {
        s_state.receiver_connected = false;
    }
    else if ((HAL_GetTick() - s_state.last_rc_frame_tick_ms) > DRV_CRSF_LINK_TIMEOUT_MS)
    {
        s_state.receiver_connected = false;
    }
    else
    {
        s_state.receiver_connected = true;
    }
}

void drv_crsf_get_state(drv_crsf_state_t *out_state)
{
    if (out_state == NULL)
    {
        return;
    }

    *out_state = s_state;
}

bool drv_crsf_get_channels(crsf_channels_t *out_channels)
{
    if (out_channels == NULL)
    {
        return false;
    }

    if (!s_state.receiver_connected || !s_state.channels.valid)
    {
        return false;
    }

    *out_channels = s_state.channels;

    return true;
}

bool drv_crsf_is_receiver_connected(void)
{
    return s_state.receiver_connected;
}

void drv_crsf_note_uart_error_from_isr(UART_HandleTypeDef *huart)
{
    if ((s_huart != NULL) && (huart == s_huart))
    {
        s_state.uart_error_count++;
        s_uart_recover_pending = true;
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    drv_crsf_note_uart_error_from_isr(huart);
}