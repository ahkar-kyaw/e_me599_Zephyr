#ifndef DRV_SSD1306_H
#define DRV_SSD1306_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_SSD1306_WIDTH                 128u
#define DRV_SSD1306_HEIGHT_32             32u
#define DRV_SSD1306_HEIGHT_64             64u
#define DRV_SSD1306_DEFAULT_HEIGHT        DRV_SSD1306_HEIGHT_64
#define DRV_SSD1306_PAGE_HEIGHT_PIXELS    8u
#define DRV_SSD1306_MAX_PAGES             8u
#define DRV_SSD1306_FRAMEBUFFER_SIZE      ((DRV_SSD1306_WIDTH * DRV_SSD1306_HEIGHT_64) / 8u)

#define DRV_SSD1306_I2C_ADDR_3C           0x3Cu
#define DRV_SSD1306_I2C_ADDR_3D           0x3Du
#define DRV_SSD1306_DEFAULT_I2C_TIMEOUT   20u

typedef enum
{
    DRV_SSD1306_OK = 0,
    DRV_SSD1306_ERROR_NULL,
    DRV_SSD1306_ERROR_CONFIG,
    DRV_SSD1306_ERROR_NOT_READY,
    DRV_SSD1306_ERROR_I2C,
    DRV_SSD1306_ERROR_NOT_INITIALIZED,
    DRV_SSD1306_ERROR_UNSUPPORTED
} drv_ssd1306_result_t;

typedef enum
{
    DRV_SSD1306_COLOR_BLACK = 0,
    DRV_SSD1306_COLOR_WHITE = 1,
    DRV_SSD1306_COLOR_INVERT = 2
} drv_ssd1306_color_t;

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr_7bit;
    uint16_t width;
    uint16_t height;
    uint8_t contrast;
    bool external_vcc;
    bool rotate_180;
    uint32_t i2c_timeout_ms;
} drv_ssd1306_config_t;

typedef struct
{
    bool initialized;
    bool present;
    bool display_on;
    drv_ssd1306_result_t last_result;
    uint32_t transfer_count;
    uint32_t error_count;
} drv_ssd1306_status_t;

typedef struct
{
    drv_ssd1306_config_t cfg;
    drv_ssd1306_status_t status;
    uint8_t framebuffer[DRV_SSD1306_FRAMEBUFFER_SIZE];
} drv_ssd1306_t;

void drv_ssd1306_default_config(drv_ssd1306_config_t *cfg, I2C_HandleTypeDef *hi2c);

drv_ssd1306_result_t drv_ssd1306_init(drv_ssd1306_t *dev, const drv_ssd1306_config_t *cfg);
drv_ssd1306_result_t drv_ssd1306_probe(drv_ssd1306_t *dev);
drv_ssd1306_result_t drv_ssd1306_get_status(const drv_ssd1306_t *dev, drv_ssd1306_status_t *out_status);

drv_ssd1306_result_t drv_ssd1306_display_on(drv_ssd1306_t *dev);
drv_ssd1306_result_t drv_ssd1306_display_off(drv_ssd1306_t *dev);
drv_ssd1306_result_t drv_ssd1306_set_inverted(drv_ssd1306_t *dev, bool inverted);
drv_ssd1306_result_t drv_ssd1306_set_contrast(drv_ssd1306_t *dev, uint8_t contrast);

drv_ssd1306_result_t drv_ssd1306_clear(drv_ssd1306_t *dev, drv_ssd1306_color_t color);
drv_ssd1306_result_t drv_ssd1306_update(drv_ssd1306_t *dev);
drv_ssd1306_result_t drv_ssd1306_update_page(drv_ssd1306_t *dev, uint8_t page);

drv_ssd1306_result_t drv_ssd1306_set_pixel(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    drv_ssd1306_color_t color);

drv_ssd1306_result_t drv_ssd1306_draw_line(
    drv_ssd1306_t *dev,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    drv_ssd1306_color_t color);

drv_ssd1306_result_t drv_ssd1306_draw_rect(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    drv_ssd1306_color_t color);

drv_ssd1306_result_t drv_ssd1306_fill_rect(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    drv_ssd1306_color_t color);

drv_ssd1306_result_t drv_ssd1306_draw_char(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    char c,
    drv_ssd1306_color_t color,
    uint8_t scale);

drv_ssd1306_result_t drv_ssd1306_draw_string(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    const char *text,
    drv_ssd1306_color_t color,
    uint8_t scale);

drv_ssd1306_result_t drv_ssd1306_draw_text_line(
    drv_ssd1306_t *dev,
    uint8_t line,
    const char *text,
    drv_ssd1306_color_t color);

#ifdef __cplusplus
}
#endif

#endif