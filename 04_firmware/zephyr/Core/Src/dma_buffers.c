#include "dma_buffers.h"
#include <string.h>

DMA_BUFFER_SECTION uint16_t adc1_raw[ADC1_RAW_COUNT];
DMA_BUFFER_SECTION uint8_t crsf_rx_dma_buf[CRSF_RX_DMA_BUF_LEN];

void dma_buffers_init(void)
{
    memset(adc1_raw, 0, sizeof(adc1_raw));
    memset(crsf_rx_dma_buf, 0, sizeof(crsf_rx_dma_buf));
}