#include "drivers/drv_ssd1351.h"

#include <stddef.h>
#include <string.h>

#define SSD1351_CMD_SET_COLUMN       0x15u
#define SSD1351_CMD_SET_ROW          0x75u
#define SSD1351_CMD_WRITE_RAM        0x5Cu
#define SSD1351_CMD_SET_REMAP        0xA0u
#define SSD1351_CMD_START_LINE       0xA1u
#define SSD1351_CMD_DISPLAY_OFFSET   0xA2u
#define SSD1351_CMD_NORMAL_DISPLAY   0xA6u
#define SSD1351_CMD_DISPLAY_OFF      0xAEu
#define SSD1351_CMD_DISPLAY_ON       0xAFu
#define SSD1351_CMD_FUNCTION_SELECT  0xABu
#define SSD1351_CMD_PHASE_LENGTH     0xB1u
#define SSD1351_CMD_ENHANCEMENT      0xB2u
#define SSD1351_CMD_CLOCK_DIVIDER    0xB3u
#define SSD1351_CMD_SET_VSL          0xB4u
#define SSD1351_CMD_SECOND_PRECHARGE 0xB6u
#define SSD1351_CMD_PRECHARGE        0xBBu
#define SSD1351_CMD_VCOMH            0xBEu
#define SSD1351_CMD_COLOR_CONTRAST   0xC1u
#define SSD1351_CMD_MASTER_CONTRAST  0xC7u
#define SSD1351_CMD_MUX_RATIO        0xCAu
#define SSD1351_CMD_COMMAND_LOCK     0xFDu

static int drv_ssd1351_command(drv_ssd1351_t *display,
                               uint8_t command,
                               const uint8_t *data,
                               size_t data_length);
static int drv_ssd1351_write_init_sequence(drv_ssd1351_t *display);
static void drv_ssd1351_write_pixel(uint8_t *destination,
                                    uint16_t color);

drv_ssd1351_config_t drv_ssd1351_default_config(void)
{
    const drv_ssd1351_config_t config =
    {
        .master_contrast = 0x0Fu,
    };

    return config;
}

int drv_ssd1351_init(drv_ssd1351_t *display,
                     const if_display_io_t *io,
                     const drv_ssd1351_config_t *config,
                     uint8_t *framebuffer,
                     size_t framebuffer_size)
{
    if ((display == NULL) || (io == NULL) || (config == NULL) ||
        (framebuffer == NULL) ||
        (framebuffer_size < DRV_SSD1351_FRAMEBUFFER_SIZE) ||
        (io->write_command == NULL) || (io->write_data == NULL) ||
        (io->set_reset == NULL) || (io->delay_ms == NULL) ||
        (config->master_contrast > 0x0Fu))
    {
        return DRV_SSD1351_ERR;
    }

    memset(display, 0, sizeof(*display));
    display->io = *io;
    display->config = *config;
    display->framebuffer = framebuffer;
    display->framebuffer_size = framebuffer_size;

    display->io.set_reset(display->io.context, true);
    display->io.delay_ms(display->io.context, 100u);
    display->io.set_reset(display->io.context, false);
    display->io.delay_ms(display->io.context, 100u);
    display->io.set_reset(display->io.context, true);
    display->io.delay_ms(display->io.context, 100u);

    if (drv_ssd1351_write_init_sequence(display) != DRV_SSD1351_OK)
    {
        return DRV_SSD1351_ERR;
    }

    display->initialized = true;
    drv_ssd1351_clear(display, 0x0000u);

    if (drv_ssd1351_update(display) != DRV_SSD1351_OK)
    {
        display->initialized = false;
        return DRV_SSD1351_ERR;
    }

    if (drv_ssd1351_command(display,
                            SSD1351_CMD_DISPLAY_ON,
                            NULL,
                            0u) != DRV_SSD1351_OK)
    {
        display->initialized = false;
        return DRV_SSD1351_ERR;
    }

    display->io.delay_ms(display->io.context, 200u);
    return DRV_SSD1351_OK;
}

void drv_ssd1351_clear(drv_ssd1351_t *display, uint16_t color)
{
    if ((display == NULL) || (display->framebuffer == NULL))
    {
        return;
    }

    drv_ssd1351_fill_rect(display,
                          0u,
                          0u,
                          DRV_SSD1351_WIDTH,
                          DRV_SSD1351_HEIGHT,
                          color);
}

void drv_ssd1351_draw_pixel(drv_ssd1351_t *display,
                            uint16_t x,
                            uint16_t y,
                            uint16_t color)
{
    if ((display == NULL) || (display->framebuffer == NULL) ||
        (x >= DRV_SSD1351_WIDTH) || (y >= DRV_SSD1351_HEIGHT))
    {
        return;
    }

    const size_t offset =
        (((size_t)y * DRV_SSD1351_WIDTH) + x) *
        DRV_SSD1351_BYTES_PER_PIXEL;

    drv_ssd1351_write_pixel(&display->framebuffer[offset], color);
}

void drv_ssd1351_fill_rect(drv_ssd1351_t *display,
                           uint16_t x,
                           uint16_t y,
                           uint16_t width,
                           uint16_t height,
                           uint16_t color)
{
    if ((display == NULL) || (display->framebuffer == NULL) ||
        (width == 0u) || (height == 0u) ||
        (x >= DRV_SSD1351_WIDTH) || (y >= DRV_SSD1351_HEIGHT))
    {
        return;
    }

    uint16_t end_x = (uint16_t)(x + width);
    uint16_t end_y = (uint16_t)(y + height);

    if ((end_x < x) || (end_x > DRV_SSD1351_WIDTH))
    {
        end_x = DRV_SSD1351_WIDTH;
    }

    if ((end_y < y) || (end_y > DRV_SSD1351_HEIGHT))
    {
        end_y = DRV_SSD1351_HEIGHT;
    }

    for (uint16_t row = y; row < end_y; row++)
    {
        for (uint16_t column = x; column < end_x; column++)
        {
            const size_t offset =
                (((size_t)row * DRV_SSD1351_WIDTH) + column) *
                DRV_SSD1351_BYTES_PER_PIXEL;
            drv_ssd1351_write_pixel(&display->framebuffer[offset],
                                    color);
        }
    }
}

int drv_ssd1351_update(drv_ssd1351_t *display)
{
    if ((display == NULL) || !display->initialized ||
        (display->framebuffer == NULL))
    {
        return DRV_SSD1351_ERR;
    }

    const uint8_t column[] = {0u, DRV_SSD1351_WIDTH - 1u};
    const uint8_t row[] = {0u, DRV_SSD1351_HEIGHT - 1u};

    if ((drv_ssd1351_command(display,
                             SSD1351_CMD_SET_COLUMN,
                             column,
                             sizeof(column)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SET_ROW,
                             row,
                             sizeof(row)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_WRITE_RAM,
                             NULL,
                             0u) != DRV_SSD1351_OK))
    {
        return DRV_SSD1351_ERR;
    }

    return (display->io.write_data(display->io.context,
                                   display->framebuffer,
                                   DRV_SSD1351_FRAMEBUFFER_SIZE) ==
            IF_DISPLAY_IO_OK)
        ? DRV_SSD1351_OK
        : DRV_SSD1351_ERR;
}

static int drv_ssd1351_command(drv_ssd1351_t *display,
                               uint8_t command,
                               const uint8_t *data,
                               size_t data_length)
{
    if ((display == NULL) ||
        ((data == NULL) && (data_length != 0u)))
    {
        return DRV_SSD1351_ERR;
    }

    if (display->io.write_command(display->io.context, command) !=
        IF_DISPLAY_IO_OK)
    {
        return DRV_SSD1351_ERR;
    }

    if ((data_length != 0u) &&
        (display->io.write_data(display->io.context,
                                data,
                                data_length) != IF_DISPLAY_IO_OK))
    {
        return DRV_SSD1351_ERR;
    }

    return DRV_SSD1351_OK;
}

static int drv_ssd1351_write_init_sequence(drv_ssd1351_t *display)
{
    const uint8_t unlock_commands = 0x12u;
    const uint8_t unlock_protected = 0xB1u;
    const uint8_t column[] = {0x00u, 0x7Fu};
    const uint8_t row[] = {0x00u, 0x7Fu};
    const uint8_t clock_divider = 0xF1u;
    const uint8_t mux_ratio = 0x7Fu;
    const uint8_t remap = 0x74u;
    const uint8_t start_line = 0x00u;
    const uint8_t display_offset = 0x00u;
    const uint8_t function_select = 0x01u;
    const uint8_t vsl[] = {0xA0u, 0xB5u, 0x55u};
    const uint8_t color_contrast[] = {0xC8u, 0x80u, 0xC0u};
    const uint8_t enhancement[] = {0xA4u, 0x00u, 0x00u};
    const uint8_t phase_length = 0x32u;
    const uint8_t precharge = 0x17u;
    const uint8_t second_precharge = 0x01u;
    const uint8_t vcomh = 0x05u;

    if ((drv_ssd1351_command(display,
                             SSD1351_CMD_COMMAND_LOCK,
                             &unlock_commands,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_COMMAND_LOCK,
                             &unlock_protected,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_DISPLAY_OFF,
                             NULL,
                             0u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SET_COLUMN,
                             column,
                             sizeof(column)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SET_ROW,
                             row,
                             sizeof(row)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_CLOCK_DIVIDER,
                             &clock_divider,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_MUX_RATIO,
                             &mux_ratio,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SET_REMAP,
                             &remap,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_START_LINE,
                             &start_line,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_DISPLAY_OFFSET,
                             &display_offset,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_FUNCTION_SELECT,
                             &function_select,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SET_VSL,
                             vsl,
                             sizeof(vsl)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_COLOR_CONTRAST,
                             color_contrast,
                             sizeof(color_contrast)) !=
         DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_MASTER_CONTRAST,
                             &display->config.master_contrast,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_PHASE_LENGTH,
                             &phase_length,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_ENHANCEMENT,
                             enhancement,
                             sizeof(enhancement)) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_PRECHARGE,
                             &precharge,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_SECOND_PRECHARGE,
                             &second_precharge,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_VCOMH,
                             &vcomh,
                             1u) != DRV_SSD1351_OK) ||
        (drv_ssd1351_command(display,
                             SSD1351_CMD_NORMAL_DISPLAY,
                             NULL,
                             0u) != DRV_SSD1351_OK))
    {
        return DRV_SSD1351_ERR;
    }

    return DRV_SSD1351_OK;
}

static void drv_ssd1351_write_pixel(uint8_t *destination,
                                    uint16_t color)
{
    destination[0] = (uint8_t)(color >> 8u);
    destination[1] = (uint8_t)color;
}
