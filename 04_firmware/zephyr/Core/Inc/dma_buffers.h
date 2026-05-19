#ifndef DMA_BUFFERS_H
#define DMA_BUFFERS_H

#include <stdint.h>
#include <stddef.h>

#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer"), aligned(32)))

#define ADC1_RAW_COUNT          2u
#define CRSF_RX_DMA_BUF_LEN     512u

extern uint16_t adc1_raw[ADC1_RAW_COUNT];
extern uint8_t crsf_rx_dma_buf[CRSF_RX_DMA_BUF_LEN];

void dma_buffers_init(void);

#endif