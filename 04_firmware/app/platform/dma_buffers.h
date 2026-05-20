#ifndef DMA_BUFFERS_H
#define DMA_BUFFERS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * STM32H7 DMA buffers must be placed in DMA-accessible SRAM.
 * The definitions in dma_buffers.c use this attribute to place buffers in
 * the .dma_buffer linker section, which is mapped to RAM_D2 by the custom
 * linker script.
 */
#define DMA_BUFFER_SECTION __attribute__((section(".dma_buffer"), aligned(32)))

#define ADC1_RAW_COUNT          2u
#define CRSF_RX_DMA_BUF_LEN     512u

extern uint16_t adc1_raw[ADC1_RAW_COUNT];
extern uint8_t crsf_rx_dma_buf[CRSF_RX_DMA_BUF_LEN];

void dma_buffers_init(void);

#ifdef __cplusplus
}
#endif

#endif
