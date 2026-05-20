#ifndef DRV_CRSF_H
#define DRV_CRSF_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "proto_crsf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_CRSF_LINK_TIMEOUT_MS 250u

typedef struct
{
    crsf_channels_t channels;
    crsf_link_statistics_t link_stats;
    uint32_t last_rc_frame_tick_ms;
    uint32_t valid_frame_count;
    uint32_t valid_rc_frame_count;
    uint32_t crc_error_count;
    uint32_t length_error_count;
    uint32_t uart_error_count;
    uint32_t dma_restart_count;
    bool receiver_connected;
} drv_crsf_state_t;

bool drv_crsf_init(UART_HandleTypeDef *huart, uint8_t *dma_buffer, uint16_t dma_buffer_len);
bool drv_crsf_start(void);
void drv_crsf_process(void);
void drv_crsf_get_state(drv_crsf_state_t *out_state);
bool drv_crsf_get_channels(crsf_channels_t *out_channels);
bool drv_crsf_is_receiver_connected(void);
void drv_crsf_note_uart_error_from_isr(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif
