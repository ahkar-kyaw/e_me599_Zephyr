#include "ui/ui_canvas.h"

#include <stddef.h>
#include <string.h>

#define UI_FONT_WIDTH   5u
#define UI_FONT_HEIGHT  7u
#define UI_FONT_ADVANCE 6u

typedef struct
{
    char character;
    uint8_t column[UI_FONT_WIDTH];
} ui_glyph_t;

static const ui_glyph_t g_font[] =
{
    {' ', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u}},
    {'+', {0x08u, 0x08u, 0x3Eu, 0x08u, 0x08u}},
    {'-', {0x08u, 0x08u, 0x08u, 0x08u, 0x08u}},
    {'.', {0x00u, 0x60u, 0x60u, 0x00u, 0x00u}},
    {'/', {0x20u, 0x10u, 0x08u, 0x04u, 0x02u}},
    {':', {0x00u, 0x36u, 0x36u, 0x00u, 0x00u}},
    {'?', {0x02u, 0x01u, 0x59u, 0x09u, 0x06u}},
    {'0', {0x3Eu, 0x51u, 0x49u, 0x45u, 0x3Eu}},
    {'1', {0x00u, 0x42u, 0x7Fu, 0x40u, 0x00u}},
    {'2', {0x42u, 0x61u, 0x51u, 0x49u, 0x46u}},
    {'3', {0x21u, 0x41u, 0x45u, 0x4Bu, 0x31u}},
    {'4', {0x18u, 0x14u, 0x12u, 0x7Fu, 0x10u}},
    {'5', {0x27u, 0x45u, 0x45u, 0x45u, 0x39u}},
    {'6', {0x3Cu, 0x4Au, 0x49u, 0x49u, 0x30u}},
    {'7', {0x01u, 0x71u, 0x09u, 0x05u, 0x03u}},
    {'8', {0x36u, 0x49u, 0x49u, 0x49u, 0x36u}},
    {'9', {0x06u, 0x49u, 0x49u, 0x29u, 0x1Eu}},
    {'A', {0x7Eu, 0x11u, 0x11u, 0x11u, 0x7Eu}},
    {'B', {0x7Fu, 0x49u, 0x49u, 0x49u, 0x36u}},
    {'C', {0x3Eu, 0x41u, 0x41u, 0x41u, 0x22u}},
    {'D', {0x7Fu, 0x41u, 0x41u, 0x22u, 0x1Cu}},
    {'E', {0x7Fu, 0x49u, 0x49u, 0x49u, 0x41u}},
    {'F', {0x7Fu, 0x09u, 0x09u, 0x09u, 0x01u}},
    {'G', {0x3Eu, 0x41u, 0x49u, 0x49u, 0x7Au}},
    {'H', {0x7Fu, 0x08u, 0x08u, 0x08u, 0x7Fu}},
    {'I', {0x00u, 0x41u, 0x7Fu, 0x41u, 0x00u}},
    {'J', {0x20u, 0x40u, 0x41u, 0x3Fu, 0x01u}},
    {'K', {0x7Fu, 0x08u, 0x14u, 0x22u, 0x41u}},
    {'L', {0x7Fu, 0x40u, 0x40u, 0x40u, 0x40u}},
    {'M', {0x7Fu, 0x02u, 0x0Cu, 0x02u, 0x7Fu}},
    {'N', {0x7Fu, 0x04u, 0x08u, 0x10u, 0x7Fu}},
    {'O', {0x3Eu, 0x41u, 0x41u, 0x41u, 0x3Eu}},
    {'P', {0x7Fu, 0x09u, 0x09u, 0x09u, 0x06u}},
    {'Q', {0x3Eu, 0x41u, 0x51u, 0x21u, 0x5Eu}},
    {'R', {0x7Fu, 0x09u, 0x19u, 0x29u, 0x46u}},
    {'S', {0x46u, 0x49u, 0x49u, 0x49u, 0x31u}},
    {'T', {0x01u, 0x01u, 0x7Fu, 0x01u, 0x01u}},
    {'U', {0x3Fu, 0x40u, 0x40u, 0x40u, 0x3Fu}},
    {'V', {0x1Fu, 0x20u, 0x40u, 0x20u, 0x1Fu}},
    {'W', {0x3Fu, 0x40u, 0x38u, 0x40u, 0x3Fu}},
    {'X', {0x63u, 0x14u, 0x08u, 0x14u, 0x63u}},
    {'Y', {0x07u, 0x08u, 0x70u, 0x08u, 0x07u}},
    {'Z', {0x61u, 0x51u, 0x49u, 0x45u, 0x43u}},
};

static const uint8_t *ui_canvas_find_glyph(char character);
static void ui_canvas_draw_character(ui_canvas_t *canvas,
                                     uint16_t x,
                                     uint16_t y,
                                     char character,
                                     ui_color_t color,
                                     uint8_t scale);

bool ui_canvas_init(ui_canvas_t *canvas,
                    void *context,
                    uint16_t width,
                    uint16_t height,
                    ui_canvas_draw_pixel_fn draw_pixel,
                    ui_canvas_fill_rect_fn fill_rect)
{
    if ((canvas == NULL) || (context == NULL) ||
        (width == 0u) || (height == 0u) ||
        (draw_pixel == NULL) || (fill_rect == NULL))
    {
        return false;
    }

    canvas->context = context;
    canvas->width = width;
    canvas->height = height;
    canvas->draw_pixel = draw_pixel;
    canvas->fill_rect = fill_rect;
    return true;
}

void ui_canvas_clear(ui_canvas_t *canvas, ui_color_t color)
{
    if (canvas == NULL)
    {
        return;
    }

    ui_canvas_fill(canvas,
                   0u,
                   0u,
                   canvas->width,
                   canvas->height,
                   color);
}

void ui_canvas_fill(ui_canvas_t *canvas,
                    uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t height,
                    ui_color_t color)
{
    if ((canvas == NULL) || (canvas->fill_rect == NULL))
    {
        return;
    }

    canvas->fill_rect(canvas->context, x, y, width, height, color);
}

void ui_canvas_draw_text(ui_canvas_t *canvas,
                         uint16_t x,
                         uint16_t y,
                         const char *text,
                         ui_color_t color,
                         uint8_t scale)
{
    if ((canvas == NULL) || (text == NULL) ||
        (canvas->draw_pixel == NULL) || (scale == 0u))
    {
        return;
    }

    uint16_t cursor_x = x;
    const uint16_t advance = (uint16_t)(UI_FONT_ADVANCE * scale);

    while ((*text != '\0') &&
           ((uint32_t)cursor_x + (UI_FONT_WIDTH * scale) <=
            canvas->width))
    {
        ui_canvas_draw_character(canvas,
                                 cursor_x,
                                 y,
                                 *text,
                                 color,
                                 scale);
        cursor_x = (uint16_t)(cursor_x + advance);
        text++;
    }
}

uint16_t ui_canvas_text_width(const char *text, uint8_t scale)
{
    if ((text == NULL) || (scale == 0u))
    {
        return 0u;
    }

    const size_t length = strlen(text);

    if (length == 0u)
    {
        return 0u;
    }

    const size_t width =
        ((length - 1u) * UI_FONT_ADVANCE * scale) +
        (UI_FONT_WIDTH * scale);

    return (width > UINT16_MAX) ? UINT16_MAX : (uint16_t)width;
}

static const uint8_t *ui_canvas_find_glyph(char character)
{
    for (size_t i = 0u; i < (sizeof(g_font) / sizeof(g_font[0])); i++)
    {
        if (g_font[i].character == character)
        {
            return g_font[i].column;
        }
    }

    return g_font[6].column;
}

static void ui_canvas_draw_character(ui_canvas_t *canvas,
                                     uint16_t x,
                                     uint16_t y,
                                     char character,
                                     ui_color_t color,
                                     uint8_t scale)
{
    const uint8_t *glyph = ui_canvas_find_glyph(character);

    for (uint8_t column = 0u; column < UI_FONT_WIDTH; column++)
    {
        for (uint8_t row = 0u; row < UI_FONT_HEIGHT; row++)
        {
            if ((glyph[column] & (uint8_t)(1u << row)) == 0u)
            {
                continue;
            }

            const uint16_t pixel_x =
                (uint16_t)(x + ((uint16_t)column * scale));
            const uint16_t pixel_y =
                (uint16_t)(y + ((uint16_t)row * scale));

            if ((pixel_x >= canvas->width) ||
                (pixel_y >= canvas->height))
            {
                continue;
            }

            canvas->fill_rect(canvas->context,
                              pixel_x,
                              pixel_y,
                              scale,
                              scale,
                              color);
        }
    }
}
