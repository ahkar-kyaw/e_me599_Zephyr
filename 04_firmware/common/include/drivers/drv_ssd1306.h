#ifndef DRV_SSD1306_H
#define DRV_SSD1306_H

#include "interfaces/if_i2c.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_SSD1306_OK                 0
#define DRV_SSD1306_ERR               -1
#define DRV_SSD1306_MAX_WIDTH          128u
#define DRV_SSD1306_MAX_HEIGHT         64u
#define DRV_SSD1306_FRAMEBUFFER_SIZE   1024u

typedef struct
{
    uint8_t i2c_address;
    uint8_t width;
    uint8_t height;
    uint8_t contrast;
    bool rotate_180;
    bool invert;
} drv_ssd1306_config_t;

typedef struct
{
    if_i2c_t i2c;
    drv_ssd1306_config_t config;
    uint8_t framebuffer[DRV_SSD1306_FRAMEBUFFER_SIZE];
    bool initialized;
} drv_ssd1306_t;

drv_ssd1306_config_t drv_ssd1306_default_config(void);

int drv_ssd1306_init(drv_ssd1306_t *display,
                     const if_i2c_t *i2c,
                     const drv_ssd1306_config_t *config);

void drv_ssd1306_clear(drv_ssd1306_t *display);

void drv_ssd1306_draw_pixel(drv_ssd1306_t *display,
                            uint8_t x,
                            uint8_t y,
                            bool on);

void drv_ssd1306_draw_text(drv_ssd1306_t *display,
                           uint8_t x,
                           uint8_t y,
                           const char *text);

int drv_ssd1306_update(drv_ssd1306_t *display);

#ifdef __cplusplus
}
#endif

#endif
