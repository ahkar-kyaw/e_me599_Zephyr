#include "drivers/drv_ssd1306.h"

#include <stddef.h>
#include <string.h>

#define DRV_SSD1306_CONTROL_COMMAND 0x00u
#define DRV_SSD1306_CONTROL_DATA    0x40u
#define DRV_SSD1306_DATA_CHUNK_SIZE 16u
#define DRV_SSD1306_FONT_WIDTH      5u
#define DRV_SSD1306_FONT_ADVANCE    6u

typedef struct
{
    char character;
    uint8_t column[DRV_SSD1306_FONT_WIDTH];
} drv_ssd1306_glyph_t;

static const drv_ssd1306_glyph_t g_font[] =
{
    {' ', {0x00u, 0x00u, 0x00u, 0x00u, 0x00u}},
    {'-', {0x08u, 0x08u, 0x08u, 0x08u, 0x08u}},
    {'.', {0x00u, 0x60u, 0x60u, 0x00u, 0x00u}},
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

static int drv_ssd1306_write_commands(drv_ssd1306_t *display,
                                       const uint8_t *commands,
                                       size_t command_count);
static const uint8_t *drv_ssd1306_find_glyph(char character);
static void drv_ssd1306_draw_character(drv_ssd1306_t *display,
                                        uint8_t x,
                                        uint8_t y,
                                        char character);

drv_ssd1306_config_t drv_ssd1306_default_config(void)
{
    const drv_ssd1306_config_t config =
    {
        .i2c_address = 0x3Cu,
        .width = 128u,
        .height = 64u,
        .contrast = 0x7Fu,
        .rotate_180 = false,
        .invert = false,
    };

    return config;
}

int drv_ssd1306_init(drv_ssd1306_t *display,
                     const if_i2c_t *i2c,
                     const drv_ssd1306_config_t *config)
{
    if ((display == NULL) || (i2c == NULL) || (i2c->write == NULL) ||
        (config == NULL) ||
        (config->width != DRV_SSD1306_MAX_WIDTH) ||
        ((config->height != 32u) &&
         (config->height != DRV_SSD1306_MAX_HEIGHT)))
    {
        return DRV_SSD1306_ERR;
    }

    memset(display, 0, sizeof(*display));
    display->i2c = *i2c;
    display->config = *config;

    const uint8_t commands[] =
    {
        0xAEu,
        0xD5u, 0x80u,
        0xA8u, (uint8_t)(config->height - 1u),
        0xD3u, 0x00u,
        0x40u,
        0x8Du, 0x14u,
        0x20u, 0x00u,
        config->rotate_180 ? 0xA0u : 0xA1u,
        config->rotate_180 ? 0xC0u : 0xC8u,
        0xDAu, (config->height == 64u) ? 0x12u : 0x02u,
        0x81u, config->contrast,
        0xD9u, 0xF1u,
        0xDBu, 0x40u,
        0xA4u,
        config->invert ? 0xA7u : 0xA6u,
        0x2Eu,
        0xAFu,
    };

    if (drv_ssd1306_write_commands(display,
                                    commands,
                                    sizeof(commands)) != DRV_SSD1306_OK)
    {
        return DRV_SSD1306_ERR;
    }

    display->initialized = true;
    drv_ssd1306_clear(display);

    return drv_ssd1306_update(display);
}

void drv_ssd1306_clear(drv_ssd1306_t *display)
{
    if (display == NULL)
    {
        return;
    }

    memset(display->framebuffer, 0, sizeof(display->framebuffer));
}

void drv_ssd1306_draw_pixel(drv_ssd1306_t *display,
                            uint8_t x,
                            uint8_t y,
                            bool on)
{
    if ((display == NULL) ||
        (x >= display->config.width) ||
        (y >= display->config.height))
    {
        return;
    }

    const size_t index =
        (size_t)x + ((size_t)(y / 8u) * display->config.width);
    const uint8_t mask = (uint8_t)(1u << (y % 8u));

    if (on)
    {
        display->framebuffer[index] |= mask;
    }
    else
    {
        display->framebuffer[index] &= (uint8_t)~mask;
    }
}

void drv_ssd1306_draw_text(drv_ssd1306_t *display,
                           uint8_t x,
                           uint8_t y,
                           const char *text)
{
    if ((display == NULL) || (text == NULL))
    {
        return;
    }

    uint8_t cursor_x = x;

    while ((*text != '\0') &&
           ((uint16_t)cursor_x + DRV_SSD1306_FONT_WIDTH <=
            display->config.width))
    {
        drv_ssd1306_draw_character(display, cursor_x, y, *text);
        cursor_x = (uint8_t)(cursor_x + DRV_SSD1306_FONT_ADVANCE);
        text++;
    }
}

int drv_ssd1306_update(drv_ssd1306_t *display)
{
    if ((display == NULL) || !display->initialized)
    {
        return DRV_SSD1306_ERR;
    }

    const uint8_t page_count = (uint8_t)(display->config.height / 8u);
    const uint8_t address_commands[] =
    {
        0x21u, 0x00u, (uint8_t)(display->config.width - 1u),
        0x22u, 0x00u, (uint8_t)(page_count - 1u),
    };

    if (drv_ssd1306_write_commands(display,
                                    address_commands,
                                    sizeof(address_commands)) !=
        DRV_SSD1306_OK)
    {
        return DRV_SSD1306_ERR;
    }

    const size_t framebuffer_size =
        (size_t)display->config.width * page_count;
    uint8_t transfer[DRV_SSD1306_DATA_CHUNK_SIZE + 1u];

    transfer[0] = DRV_SSD1306_CONTROL_DATA;

    for (size_t offset = 0u;
         offset < framebuffer_size;
         offset += DRV_SSD1306_DATA_CHUNK_SIZE)
    {
        size_t chunk_size = framebuffer_size - offset;

        if (chunk_size > DRV_SSD1306_DATA_CHUNK_SIZE)
        {
            chunk_size = DRV_SSD1306_DATA_CHUNK_SIZE;
        }

        memcpy(&transfer[1], &display->framebuffer[offset], chunk_size);

        if (display->i2c.write(display->i2c.context,
                               display->config.i2c_address,
                               transfer,
                               chunk_size + 1u) != IF_I2C_OK)
        {
            return DRV_SSD1306_ERR;
        }
    }

    return DRV_SSD1306_OK;
}

static int drv_ssd1306_write_commands(drv_ssd1306_t *display,
                                       const uint8_t *commands,
                                       size_t command_count)
{
    if ((display == NULL) || (commands == NULL) ||
        (command_count == 0u) || (command_count > 31u))
    {
        return DRV_SSD1306_ERR;
    }

    uint8_t transfer[32u];

    transfer[0] = DRV_SSD1306_CONTROL_COMMAND;
    memcpy(&transfer[1], commands, command_count);

    return (display->i2c.write(display->i2c.context,
                               display->config.i2c_address,
                               transfer,
                               command_count + 1u) == IF_I2C_OK)
        ? DRV_SSD1306_OK
        : DRV_SSD1306_ERR;
}

static const uint8_t *drv_ssd1306_find_glyph(char character)
{
    for (size_t i = 0u; i < (sizeof(g_font) / sizeof(g_font[0])); i++)
    {
        if (g_font[i].character == character)
        {
            return g_font[i].column;
        }
    }

    return g_font[4].column;
}

static void drv_ssd1306_draw_character(drv_ssd1306_t *display,
                                        uint8_t x,
                                        uint8_t y,
                                        char character)
{
    const uint8_t *glyph = drv_ssd1306_find_glyph(character);

    for (uint8_t column = 0u; column < DRV_SSD1306_FONT_WIDTH; column++)
    {
        for (uint8_t row = 0u; row < 7u; row++)
        {
            drv_ssd1306_draw_pixel(display,
                                   (uint8_t)(x + column),
                                   (uint8_t)(y + row),
                                   (glyph[column] & (1u << row)) != 0u);
        }
    }
}
