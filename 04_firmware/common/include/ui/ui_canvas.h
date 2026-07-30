#ifndef UI_CANVAS_H
#define UI_CANVAS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ui_color_t;

#define UI_COLOR_RGB565(red, green, blue) \
    ((ui_color_t)((((uint16_t)(red) & 0xF8u) << 8u) | \
                  (((uint16_t)(green) & 0xFCu) << 3u) | \
                  (((uint16_t)(blue)) >> 3u)))

typedef void (*ui_canvas_draw_pixel_fn)(void *context,
                                        uint16_t x,
                                        uint16_t y,
                                        ui_color_t color);
typedef void (*ui_canvas_fill_rect_fn)(void *context,
                                       uint16_t x,
                                       uint16_t y,
                                       uint16_t width,
                                       uint16_t height,
                                       ui_color_t color);

typedef struct
{
    void *context;
    uint16_t width;
    uint16_t height;
    ui_canvas_draw_pixel_fn draw_pixel;
    ui_canvas_fill_rect_fn fill_rect;
} ui_canvas_t;

bool ui_canvas_init(ui_canvas_t *canvas,
                    void *context,
                    uint16_t width,
                    uint16_t height,
                    ui_canvas_draw_pixel_fn draw_pixel,
                    ui_canvas_fill_rect_fn fill_rect);

void ui_canvas_clear(ui_canvas_t *canvas, ui_color_t color);

void ui_canvas_fill(ui_canvas_t *canvas,
                    uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t height,
                    ui_color_t color);

void ui_canvas_draw_text(ui_canvas_t *canvas,
                         uint16_t x,
                         uint16_t y,
                         const char *text,
                         ui_color_t color,
                         uint8_t scale);

uint16_t ui_canvas_text_width(const char *text, uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif
