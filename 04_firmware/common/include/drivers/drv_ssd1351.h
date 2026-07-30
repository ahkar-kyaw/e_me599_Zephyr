#ifndef DRV_SSD1351_H
#define DRV_SSD1351_H

#include "interfaces/if_display_io.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_SSD1351_OK                0
#define DRV_SSD1351_ERR              -1
#define DRV_SSD1351_WIDTH             128u
#define DRV_SSD1351_HEIGHT            128u
#define DRV_SSD1351_BYTES_PER_PIXEL   2u
#define DRV_SSD1351_FRAMEBUFFER_SIZE \
    (DRV_SSD1351_WIDTH * DRV_SSD1351_HEIGHT * \
     DRV_SSD1351_BYTES_PER_PIXEL)

typedef struct
{
    uint8_t master_contrast;
} drv_ssd1351_config_t;

typedef struct
{
    if_display_io_t io;
    drv_ssd1351_config_t config;
    uint8_t *framebuffer;
    size_t framebuffer_size;
    bool initialized;
} drv_ssd1351_t;

drv_ssd1351_config_t drv_ssd1351_default_config(void);

int drv_ssd1351_init(drv_ssd1351_t *display,
                     const if_display_io_t *io,
                     const drv_ssd1351_config_t *config,
                     uint8_t *framebuffer,
                     size_t framebuffer_size);

void drv_ssd1351_clear(drv_ssd1351_t *display, uint16_t color);

void drv_ssd1351_draw_pixel(drv_ssd1351_t *display,
                            uint16_t x,
                            uint16_t y,
                            uint16_t color);

void drv_ssd1351_fill_rect(drv_ssd1351_t *display,
                           uint16_t x,
                           uint16_t y,
                           uint16_t width,
                           uint16_t height,
                           uint16_t color);

int drv_ssd1351_update(drv_ssd1351_t *display);

#ifdef __cplusplus
}
#endif

#endif
