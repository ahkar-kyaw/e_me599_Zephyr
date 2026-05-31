#ifndef TASK_CRSF_H
#define TASK_CRSF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_CRSF_CHANNEL_COUNT 16u

typedef struct
{
    bool task_running;
    bool driver_started;
    bool receiver_connected;
    bool channels_valid;

    uint32_t last_update_tick_ms;
    uint32_t rc_age_ms;
    uint32_t sequence;

    uint32_t valid_frame_count;
    uint32_t valid_rc_frame_count;
    uint32_t crc_error_count;
    uint32_t length_error_count;
    uint32_t uart_error_count;
    uint32_t dma_restart_count;

    uint32_t rx_byte_count;
    uint8_t last_rx_byte;
    uint16_t dma_write_index;
    uint16_t dma_read_index;

    uint16_t raw[TASK_CRSF_CHANNEL_COUNT];
    uint16_t us[TASK_CRSF_CHANNEL_COUNT];
    int16_t norm_permille[TASK_CRSF_CHANNEL_COUNT];
} task_crsf_snapshot_t;

bool task_crsf_start(void);
bool task_crsf_get_snapshot(task_crsf_snapshot_t *out_snapshot);
bool task_crsf_is_connected(void);
bool task_crsf_get_channel_raw(uint8_t channel_index, uint16_t *out_raw);
bool task_crsf_get_channel_us(uint8_t channel_index, uint16_t *out_us);
bool task_crsf_get_channel_norm_permille(uint8_t channel_index, int16_t *out_norm);

#ifdef __cplusplus
}
#endif

#endif