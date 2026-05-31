#include "drv_ssd1306.h"

#include <string.h>

#define SSD1306_CONTROL_COMMAND           0x00u
#define SSD1306_CONTROL_DATA              0x40u

#define SSD1306_CMD_SET_CONTRAST          0x81u
#define SSD1306_CMD_DISPLAY_ALL_RESUME    0xA4u
#define SSD1306_CMD_DISPLAY_ALL_ON        0xA5u
#define SSD1306_CMD_NORMAL_DISPLAY        0xA6u
#define SSD1306_CMD_INVERT_DISPLAY        0xA7u
#define SSD1306_CMD_DISPLAY_OFF           0xAEu
#define SSD1306_CMD_DISPLAY_ON            0xAFu
#define SSD1306_CMD_SET_DISPLAY_OFFSET    0xD3u
#define SSD1306_CMD_SET_CLOCK_DIV         0xD5u
#define SSD1306_CMD_SET_PRECHARGE         0xD9u
#define SSD1306_CMD_SET_COM_PINS          0xDAu
#define SSD1306_CMD_SET_VCOM_DESELECT     0xDBu
#define SSD1306_CMD_SET_MULTIPLEX         0xA8u
#define SSD1306_CMD_SET_START_LINE        0x40u
#define SSD1306_CMD_MEMORY_MODE           0x20u
#define SSD1306_CMD_COLUMN_ADDR           0x21u
#define SSD1306_CMD_PAGE_ADDR             0x22u
#define SSD1306_CMD_CHARGE_PUMP           0x8Du
#define SSD1306_CMD_SEG_REMAP_NORMAL      0xA0u
#define SSD1306_CMD_SEG_REMAP_INV         0xA1u
#define SSD1306_CMD_COM_SCAN_INC          0xC0u
#define SSD1306_CMD_COM_SCAN_DEC          0xC8u
#define SSD1306_CMD_DEACTIVATE_SCROLL     0x2Eu

#define SSD1306_MEMORY_MODE_HORIZONTAL    0x00u

#define SSD1306_TX_CHUNK_BYTES            32u
#define SSD1306_MAX_COMMAND_BYTES         32u

#define FONT_WIDTH                        5u
#define FONT_HEIGHT                       7u
#define FONT_SPACING                      1u
#define TEXT_LINE_HEIGHT                  8u

static const uint8_t glyph_space[FONT_WIDTH] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
static const uint8_t glyph_unknown[FONT_WIDTH] = {0x02u, 0x01u, 0x51u, 0x09u, 0x06u};

static const uint8_t glyph_0[FONT_WIDTH] = {0x3Eu, 0x51u, 0x49u, 0x45u, 0x3Eu};
static const uint8_t glyph_1[FONT_WIDTH] = {0x00u, 0x42u, 0x7Fu, 0x40u, 0x00u};
static const uint8_t glyph_2[FONT_WIDTH] = {0x42u, 0x61u, 0x51u, 0x49u, 0x46u};
static const uint8_t glyph_3[FONT_WIDTH] = {0x21u, 0x41u, 0x45u, 0x4Bu, 0x31u};
static const uint8_t glyph_4[FONT_WIDTH] = {0x18u, 0x14u, 0x12u, 0x7Fu, 0x10u};
static const uint8_t glyph_5[FONT_WIDTH] = {0x27u, 0x45u, 0x45u, 0x45u, 0x39u};
static const uint8_t glyph_6[FONT_WIDTH] = {0x3Cu, 0x4Au, 0x49u, 0x49u, 0x30u};
static const uint8_t glyph_7[FONT_WIDTH] = {0x01u, 0x71u, 0x09u, 0x05u, 0x03u};
static const uint8_t glyph_8[FONT_WIDTH] = {0x36u, 0x49u, 0x49u, 0x49u, 0x36u};
static const uint8_t glyph_9[FONT_WIDTH] = {0x06u, 0x49u, 0x49u, 0x29u, 0x1Eu};

static const uint8_t glyph_a[FONT_WIDTH] = {0x7Eu, 0x11u, 0x11u, 0x11u, 0x7Eu};
static const uint8_t glyph_b[FONT_WIDTH] = {0x7Fu, 0x49u, 0x49u, 0x49u, 0x36u};
static const uint8_t glyph_c[FONT_WIDTH] = {0x3Eu, 0x41u, 0x41u, 0x41u, 0x22u};
static const uint8_t glyph_d[FONT_WIDTH] = {0x7Fu, 0x41u, 0x41u, 0x22u, 0x1Cu};
static const uint8_t glyph_e[FONT_WIDTH] = {0x7Fu, 0x49u, 0x49u, 0x49u, 0x41u};
static const uint8_t glyph_f[FONT_WIDTH] = {0x7Fu, 0x09u, 0x09u, 0x09u, 0x01u};
static const uint8_t glyph_g[FONT_WIDTH] = {0x3Eu, 0x41u, 0x49u, 0x49u, 0x7Au};
static const uint8_t glyph_h[FONT_WIDTH] = {0x7Fu, 0x08u, 0x08u, 0x08u, 0x7Fu};
static const uint8_t glyph_i[FONT_WIDTH] = {0x00u, 0x41u, 0x7Fu, 0x41u, 0x00u};
static const uint8_t glyph_j[FONT_WIDTH] = {0x20u, 0x40u, 0x41u, 0x3Fu, 0x01u};
static const uint8_t glyph_k[FONT_WIDTH] = {0x7Fu, 0x08u, 0x14u, 0x22u, 0x41u};
static const uint8_t glyph_l[FONT_WIDTH] = {0x7Fu, 0x40u, 0x40u, 0x40u, 0x40u};
static const uint8_t glyph_m[FONT_WIDTH] = {0x7Fu, 0x02u, 0x0Cu, 0x02u, 0x7Fu};
static const uint8_t glyph_n[FONT_WIDTH] = {0x7Fu, 0x04u, 0x08u, 0x10u, 0x7Fu};
static const uint8_t glyph_o[FONT_WIDTH] = {0x3Eu, 0x41u, 0x41u, 0x41u, 0x3Eu};
static const uint8_t glyph_p[FONT_WIDTH] = {0x7Fu, 0x09u, 0x09u, 0x09u, 0x06u};
static const uint8_t glyph_q[FONT_WIDTH] = {0x3Eu, 0x41u, 0x51u, 0x21u, 0x5Eu};
static const uint8_t glyph_r[FONT_WIDTH] = {0x7Fu, 0x09u, 0x19u, 0x29u, 0x46u};
static const uint8_t glyph_s[FONT_WIDTH] = {0x46u, 0x49u, 0x49u, 0x49u, 0x31u};
static const uint8_t glyph_t[FONT_WIDTH] = {0x01u, 0x01u, 0x7Fu, 0x01u, 0x01u};
static const uint8_t glyph_u[FONT_WIDTH] = {0x3Fu, 0x40u, 0x40u, 0x40u, 0x3Fu};
static const uint8_t glyph_v[FONT_WIDTH] = {0x1Fu, 0x20u, 0x40u, 0x20u, 0x1Fu};
static const uint8_t glyph_w[FONT_WIDTH] = {0x7Fu, 0x20u, 0x18u, 0x20u, 0x7Fu};
static const uint8_t glyph_x[FONT_WIDTH] = {0x63u, 0x14u, 0x08u, 0x14u, 0x63u};
static const uint8_t glyph_y[FONT_WIDTH] = {0x03u, 0x04u, 0x78u, 0x04u, 0x03u};
static const uint8_t glyph_z[FONT_WIDTH] = {0x61u, 0x51u, 0x49u, 0x45u, 0x43u};

static const uint8_t glyph_exclaim[FONT_WIDTH] = {0x00u, 0x00u, 0x5Fu, 0x00u, 0x00u};
static const uint8_t glyph_dash[FONT_WIDTH] = {0x08u, 0x08u, 0x08u, 0x08u, 0x08u};
static const uint8_t glyph_dot[FONT_WIDTH] = {0x00u, 0x60u, 0x60u, 0x00u, 0x00u};
static const uint8_t glyph_slash[FONT_WIDTH] = {0x20u, 0x10u, 0x08u, 0x04u, 0x02u};
static const uint8_t glyph_colon[FONT_WIDTH] = {0x00u, 0x36u, 0x36u, 0x00u, 0x00u};
static const uint8_t glyph_plus[FONT_WIDTH] = {0x08u, 0x08u, 0x3Eu, 0x08u, 0x08u};
static const uint8_t glyph_percent[FONT_WIDTH] = {0x23u, 0x13u, 0x08u, 0x64u, 0x62u};
static const uint8_t glyph_equal[FONT_WIDTH] = {0x14u, 0x14u, 0x14u, 0x14u, 0x14u};
static const uint8_t glyph_underscore[FONT_WIDTH] = {0x40u, 0x40u, 0x40u, 0x40u, 0x40u};
static const uint8_t glyph_pipe[FONT_WIDTH] = {0x00u, 0x00u, 0x7Fu, 0x00u, 0x00u};
static const uint8_t glyph_left_paren[FONT_WIDTH] = {0x00u, 0x1Cu, 0x22u, 0x41u, 0x00u};
static const uint8_t glyph_right_paren[FONT_WIDTH] = {0x00u, 0x41u, 0x22u, 0x1Cu, 0x00u};

static drv_ssd1306_result_t drv_ssd1306_record_result(
    drv_ssd1306_t *dev,
    drv_ssd1306_result_t result)
{
    if (dev != NULL)
    {
        dev->status.last_result = result;

        if (result != DRV_SSD1306_OK)
        {
            dev->status.error_count++;
        }
    }

    return result;
}

static bool drv_ssd1306_valid_config(const drv_ssd1306_config_t *cfg)
{
    if ((cfg == NULL) || (cfg->hi2c == NULL))
    {
        return false;
    }

    if (cfg->width != DRV_SSD1306_WIDTH)
    {
        return false;
    }

    if ((cfg->height != DRV_SSD1306_HEIGHT_32) &&
        (cfg->height != DRV_SSD1306_HEIGHT_64))
    {
        return false;
    }

    if ((cfg->i2c_addr_7bit != DRV_SSD1306_I2C_ADDR_3C) &&
        (cfg->i2c_addr_7bit != DRV_SSD1306_I2C_ADDR_3D))
    {
        return false;
    }

    if (cfg->i2c_timeout_ms == 0u)
    {
        return false;
    }

    return true;
}

static uint16_t drv_ssd1306_hal_addr(const drv_ssd1306_t *dev)
{
    return (uint16_t)((uint16_t)dev->cfg.i2c_addr_7bit << 1u);
}

static uint8_t drv_ssd1306_page_count(const drv_ssd1306_t *dev)
{
    return (uint8_t)(dev->cfg.height / DRV_SSD1306_PAGE_HEIGHT_PIXELS);
}

static size_t drv_ssd1306_framebuffer_len(const drv_ssd1306_t *dev)
{
    return ((size_t)dev->cfg.width * (size_t)dev->cfg.height) / 8u;
}

static drv_ssd1306_result_t drv_ssd1306_i2c_write(
    drv_ssd1306_t *dev,
    const uint8_t *data,
    uint16_t len)
{
    if ((dev == NULL) || (data == NULL) || (len == 0u))
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NULL);
    }

    if (HAL_I2C_Master_Transmit(
            dev->cfg.hi2c,
            drv_ssd1306_hal_addr(dev),
            (uint8_t *)data,
            len,
            dev->cfg.i2c_timeout_ms) != HAL_OK)
    {
        dev->status.present = false;
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_I2C);
    }

    dev->status.transfer_count++;
    dev->status.present = true;

    return drv_ssd1306_record_result(dev, DRV_SSD1306_OK);
}

static drv_ssd1306_result_t drv_ssd1306_write_command(
    drv_ssd1306_t *dev,
    uint8_t command)
{
    uint8_t packet[2];

    packet[0] = SSD1306_CONTROL_COMMAND;
    packet[1] = command;

    return drv_ssd1306_i2c_write(dev, packet, sizeof(packet));
}

static drv_ssd1306_result_t drv_ssd1306_write_commands(
    drv_ssd1306_t *dev,
    const uint8_t *commands,
    size_t command_count)
{
    uint8_t packet[1u + SSD1306_MAX_COMMAND_BYTES];

    if ((dev == NULL) || (commands == NULL) || (command_count == 0u))
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NULL);
    }

    if (command_count > SSD1306_MAX_COMMAND_BYTES)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_UNSUPPORTED);
    }

    packet[0] = SSD1306_CONTROL_COMMAND;
    memcpy(&packet[1], commands, command_count);

    return drv_ssd1306_i2c_write(dev, packet, (uint16_t)(command_count + 1u));
}

static drv_ssd1306_result_t drv_ssd1306_set_address_window(
    drv_ssd1306_t *dev,
    uint8_t column_start,
    uint8_t column_end,
    uint8_t page_start,
    uint8_t page_end)
{
    const uint8_t commands[] =
    {
        SSD1306_CMD_COLUMN_ADDR,
        column_start,
        column_end,
        SSD1306_CMD_PAGE_ADDR,
        page_start,
        page_end
    };

    return drv_ssd1306_write_commands(dev, commands, sizeof(commands));
}

static drv_ssd1306_result_t drv_ssd1306_write_data(
    drv_ssd1306_t *dev,
    const uint8_t *data,
    size_t len)
{
    uint8_t packet[1u + SSD1306_TX_CHUNK_BYTES];
    size_t offset = 0u;

    if ((dev == NULL) || (data == NULL))
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NULL);
    }

    while (offset < len)
    {
        size_t chunk = len - offset;

        if (chunk > SSD1306_TX_CHUNK_BYTES)
        {
            chunk = SSD1306_TX_CHUNK_BYTES;
        }

        packet[0] = SSD1306_CONTROL_DATA;
        memcpy(&packet[1], &data[offset], chunk);

        const drv_ssd1306_result_t result =
            drv_ssd1306_i2c_write(dev, packet, (uint16_t)(chunk + 1u));

        if (result != DRV_SSD1306_OK)
        {
            return result;
        }

        offset += chunk;
    }

    return drv_ssd1306_record_result(dev, DRV_SSD1306_OK);
}

static const uint8_t *drv_ssd1306_get_glyph(char c)
{
    if ((c >= 'a') && (c <= 'z'))
    {
        c = (char)(c - ('a' - 'A'));
    }

    switch (c)
    {
        case ' ': return glyph_space;
        case '!': return glyph_exclaim;
        case '-': return glyph_dash;
        case '.': return glyph_dot;
        case '/': return glyph_slash;
        case ':': return glyph_colon;
        case '+': return glyph_plus;
        case '%': return glyph_percent;
        case '=': return glyph_equal;
        case '_': return glyph_underscore;
        case '|': return glyph_pipe;
        case '(': return glyph_left_paren;
        case ')': return glyph_right_paren;

        case '0': return glyph_0;
        case '1': return glyph_1;
        case '2': return glyph_2;
        case '3': return glyph_3;
        case '4': return glyph_4;
        case '5': return glyph_5;
        case '6': return glyph_6;
        case '7': return glyph_7;
        case '8': return glyph_8;
        case '9': return glyph_9;

        case 'A': return glyph_a;
        case 'B': return glyph_b;
        case 'C': return glyph_c;
        case 'D': return glyph_d;
        case 'E': return glyph_e;
        case 'F': return glyph_f;
        case 'G': return glyph_g;
        case 'H': return glyph_h;
        case 'I': return glyph_i;
        case 'J': return glyph_j;
        case 'K': return glyph_k;
        case 'L': return glyph_l;
        case 'M': return glyph_m;
        case 'N': return glyph_n;
        case 'O': return glyph_o;
        case 'P': return glyph_p;
        case 'Q': return glyph_q;
        case 'R': return glyph_r;
        case 'S': return glyph_s;
        case 'T': return glyph_t;
        case 'U': return glyph_u;
        case 'V': return glyph_v;
        case 'W': return glyph_w;
        case 'X': return glyph_x;
        case 'Y': return glyph_y;
        case 'Z': return glyph_z;

        default: return glyph_unknown;
    }
}

static int32_t drv_ssd1306_abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

void drv_ssd1306_default_config(drv_ssd1306_config_t *cfg, I2C_HandleTypeDef *hi2c)
{
    if (cfg == NULL)
    {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));

    cfg->hi2c = hi2c;
    cfg->i2c_addr_7bit = DRV_SSD1306_I2C_ADDR_3C;
    cfg->width = DRV_SSD1306_WIDTH;
    cfg->height = DRV_SSD1306_DEFAULT_HEIGHT;
    cfg->contrast = 0x7Fu;
    cfg->external_vcc = false;
    cfg->rotate_180 = false;
    cfg->i2c_timeout_ms = DRV_SSD1306_DEFAULT_I2C_TIMEOUT;
}

drv_ssd1306_result_t drv_ssd1306_probe(drv_ssd1306_t *dev)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!drv_ssd1306_valid_config(&dev->cfg))
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_CONFIG);
    }

    if (HAL_I2C_IsDeviceReady(
            dev->cfg.hi2c,
            drv_ssd1306_hal_addr(dev),
            2u,
            dev->cfg.i2c_timeout_ms) != HAL_OK)
    {
        dev->status.present = false;
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_READY);
    }

    dev->status.present = true;
    return drv_ssd1306_record_result(dev, DRV_SSD1306_OK);
}

drv_ssd1306_result_t drv_ssd1306_init(drv_ssd1306_t *dev, const drv_ssd1306_config_t *cfg)
{
    uint8_t com_pins;
    uint8_t charge_pump;
    uint8_t precharge;
    uint8_t seg_remap;
    uint8_t com_scan;

    if ((dev == NULL) || (cfg == NULL))
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!drv_ssd1306_valid_config(cfg))
    {
        return DRV_SSD1306_ERROR_CONFIG;
    }

    memset(dev, 0, sizeof(*dev));
    dev->cfg = *cfg;
    dev->status.last_result = DRV_SSD1306_OK;

    if (drv_ssd1306_probe(dev) != DRV_SSD1306_OK)
    {
        return dev->status.last_result;
    }

    HAL_Delay(20u);

    com_pins = (dev->cfg.height == DRV_SSD1306_HEIGHT_64) ? 0x12u : 0x02u;
    charge_pump = dev->cfg.external_vcc ? 0x10u : 0x14u;
    precharge = dev->cfg.external_vcc ? 0x22u : 0xF1u;

    if (dev->cfg.rotate_180)
    {
        seg_remap = SSD1306_CMD_SEG_REMAP_NORMAL;
        com_scan = SSD1306_CMD_COM_SCAN_INC;
    }
    else
    {
        seg_remap = SSD1306_CMD_SEG_REMAP_INV;
        com_scan = SSD1306_CMD_COM_SCAN_DEC;
    }

    const uint8_t init_sequence[] =
    {
        SSD1306_CMD_DISPLAY_OFF,
        SSD1306_CMD_SET_CLOCK_DIV, 0x80u,
        SSD1306_CMD_SET_MULTIPLEX, (uint8_t)(dev->cfg.height - 1u),
        SSD1306_CMD_SET_DISPLAY_OFFSET, 0x00u,
        SSD1306_CMD_SET_START_LINE,
        SSD1306_CMD_CHARGE_PUMP, charge_pump,
        SSD1306_CMD_MEMORY_MODE, SSD1306_MEMORY_MODE_HORIZONTAL,
        seg_remap,
        com_scan,
        SSD1306_CMD_SET_COM_PINS, com_pins,
        SSD1306_CMD_SET_CONTRAST, dev->cfg.contrast,
        SSD1306_CMD_SET_PRECHARGE, precharge,
        SSD1306_CMD_SET_VCOM_DESELECT, 0x40u,
        SSD1306_CMD_DISPLAY_ALL_RESUME,
        SSD1306_CMD_NORMAL_DISPLAY,
        SSD1306_CMD_DEACTIVATE_SCROLL
    };

    drv_ssd1306_result_t result =
        drv_ssd1306_write_commands(dev, init_sequence, sizeof(init_sequence));

    if (result != DRV_SSD1306_OK)
    {
        dev->status.initialized = false;
        return result;
    }

    dev->status.initialized = true;

    result = drv_ssd1306_clear(dev, DRV_SSD1306_COLOR_BLACK);
    if (result != DRV_SSD1306_OK)
    {
        dev->status.initialized = false;
        return result;
    }

    result = drv_ssd1306_update(dev);
    if (result != DRV_SSD1306_OK)
    {
        dev->status.initialized = false;
        return result;
    }

    result = drv_ssd1306_display_on(dev);
    if (result != DRV_SSD1306_OK)
    {
        dev->status.initialized = false;
        return result;
    }

    return drv_ssd1306_record_result(dev, DRV_SSD1306_OK);
}

drv_ssd1306_result_t drv_ssd1306_get_status(
    const drv_ssd1306_t *dev,
    drv_ssd1306_status_t *out_status)
{
    if ((dev == NULL) || (out_status == NULL))
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    *out_status = dev->status;
    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_display_on(drv_ssd1306_t *dev)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    const drv_ssd1306_result_t result =
        drv_ssd1306_write_command(dev, SSD1306_CMD_DISPLAY_ON);

    if (result == DRV_SSD1306_OK)
    {
        dev->status.display_on = true;
    }

    return result;
}

drv_ssd1306_result_t drv_ssd1306_display_off(drv_ssd1306_t *dev)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    const drv_ssd1306_result_t result =
        drv_ssd1306_write_command(dev, SSD1306_CMD_DISPLAY_OFF);

    if (result == DRV_SSD1306_OK)
    {
        dev->status.display_on = false;
    }

    return result;
}

drv_ssd1306_result_t drv_ssd1306_set_inverted(drv_ssd1306_t *dev, bool inverted)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    return drv_ssd1306_write_command(
        dev,
        inverted ? SSD1306_CMD_INVERT_DISPLAY : SSD1306_CMD_NORMAL_DISPLAY);
}

drv_ssd1306_result_t drv_ssd1306_set_contrast(drv_ssd1306_t *dev, uint8_t contrast)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    const uint8_t commands[] =
    {
        SSD1306_CMD_SET_CONTRAST,
        contrast
    };

    const drv_ssd1306_result_t result =
        drv_ssd1306_write_commands(dev, commands, sizeof(commands));

    if (result == DRV_SSD1306_OK)
    {
        dev->cfg.contrast = contrast;
    }

    return result;
}

drv_ssd1306_result_t drv_ssd1306_clear(drv_ssd1306_t *dev, drv_ssd1306_color_t color)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    uint8_t fill = 0x00u;

    if (color == DRV_SSD1306_COLOR_WHITE)
    {
        fill = 0xFFu;
    }

    memset(dev->framebuffer, fill, drv_ssd1306_framebuffer_len(dev));

    return drv_ssd1306_record_result(dev, DRV_SSD1306_OK);
}

drv_ssd1306_result_t drv_ssd1306_update(drv_ssd1306_t *dev)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    drv_ssd1306_result_t result =
        drv_ssd1306_set_address_window(
            dev,
            0u,
            (uint8_t)(dev->cfg.width - 1u),
            0u,
            (uint8_t)(drv_ssd1306_page_count(dev) - 1u));

    if (result != DRV_SSD1306_OK)
    {
        return result;
    }

    return drv_ssd1306_write_data(
        dev,
        dev->framebuffer,
        drv_ssd1306_framebuffer_len(dev));
}

drv_ssd1306_result_t drv_ssd1306_update_page(drv_ssd1306_t *dev, uint8_t page)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (!dev->status.initialized)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_NOT_INITIALIZED);
    }

    if (page >= drv_ssd1306_page_count(dev))
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_CONFIG);
    }

    drv_ssd1306_result_t result =
        drv_ssd1306_set_address_window(
            dev,
            0u,
            (uint8_t)(dev->cfg.width - 1u),
            page,
            page);

    if (result != DRV_SSD1306_OK)
    {
        return result;
    }

    return drv_ssd1306_write_data(
        dev,
        &dev->framebuffer[(size_t)page * dev->cfg.width],
        dev->cfg.width);
}

drv_ssd1306_result_t drv_ssd1306_set_pixel(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    drv_ssd1306_color_t color)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if ((x < 0) || (y < 0) ||
        (x >= (int16_t)dev->cfg.width) ||
        (y >= (int16_t)dev->cfg.height))
    {
        return DRV_SSD1306_OK;
    }

    const size_t index =
        (size_t)x + ((size_t)y / DRV_SSD1306_PAGE_HEIGHT_PIXELS) * dev->cfg.width;

    const uint8_t mask =
        (uint8_t)(1u << ((uint8_t)y & 0x07u));

    if (color == DRV_SSD1306_COLOR_WHITE)
    {
        dev->framebuffer[index] |= mask;
    }
    else if (color == DRV_SSD1306_COLOR_BLACK)
    {
        dev->framebuffer[index] &= (uint8_t)(~mask);
    }
    else
    {
        dev->framebuffer[index] ^= mask;
    }

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_draw_line(
    drv_ssd1306_t *dev,
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    drv_ssd1306_color_t color)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    int32_t dx = drv_ssd1306_abs_i32((int32_t)x1 - (int32_t)x0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -drv_ssd1306_abs_i32((int32_t)y1 - (int32_t)y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;)
    {
        (void)drv_ssd1306_set_pixel(dev, x0, y0, color);

        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }

        const int32_t e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 = (int16_t)((int32_t)x0 + sx);
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 = (int16_t)((int32_t)y0 + sy);
        }
    }

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_draw_rect(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    drv_ssd1306_color_t color)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if ((w <= 0) || (h <= 0))
    {
        return DRV_SSD1306_OK;
    }

    (void)drv_ssd1306_draw_line(dev, x, y, (int16_t)(x + w - 1), y, color);
    (void)drv_ssd1306_draw_line(dev, x, (int16_t)(y + h - 1), (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);
    (void)drv_ssd1306_draw_line(dev, x, y, x, (int16_t)(y + h - 1), color);
    (void)drv_ssd1306_draw_line(dev, (int16_t)(x + w - 1), y, (int16_t)(x + w - 1), (int16_t)(y + h - 1), color);

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_fill_rect(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    drv_ssd1306_color_t color)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if ((w <= 0) || (h <= 0))
    {
        return DRV_SSD1306_OK;
    }

    for (int16_t yy = y; yy < (int16_t)(y + h); yy++)
    {
        for (int16_t xx = x; xx < (int16_t)(x + w); xx++)
        {
            (void)drv_ssd1306_set_pixel(dev, xx, yy, color);
        }
    }

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_draw_char(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    char c,
    drv_ssd1306_color_t color,
    uint8_t scale)
{
    if (dev == NULL)
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (scale == 0u)
    {
        scale = 1u;
    }

    const uint8_t *glyph = drv_ssd1306_get_glyph(c);

    for (uint8_t col = 0u; col < FONT_WIDTH; col++)
    {
        const uint8_t column_bits = glyph[col];

        for (uint8_t row = 0u; row < FONT_HEIGHT; row++)
        {
            if ((column_bits & (uint8_t)(1u << row)) != 0u)
            {
                if (scale == 1u)
                {
                    (void)drv_ssd1306_set_pixel(
                        dev,
                        (int16_t)(x + col),
                        (int16_t)(y + row),
                        color);
                }
                else
                {
                    (void)drv_ssd1306_fill_rect(
                        dev,
                        (int16_t)(x + (int16_t)col * (int16_t)scale),
                        (int16_t)(y + (int16_t)row * (int16_t)scale),
                        scale,
                        scale,
                        color);
                }
            }
        }
    }

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_draw_string(
    drv_ssd1306_t *dev,
    int16_t x,
    int16_t y,
    const char *text,
    drv_ssd1306_color_t color,
    uint8_t scale)
{
    if ((dev == NULL) || (text == NULL))
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    if (scale == 0u)
    {
        scale = 1u;
    }

    int16_t cursor_x = x;
    int16_t cursor_y = y;
    const int16_t advance = (int16_t)((FONT_WIDTH + FONT_SPACING) * scale);

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursor_x = x;
            cursor_y = (int16_t)(cursor_y + (int16_t)(TEXT_LINE_HEIGHT * scale));
            text++;
            continue;
        }

        (void)drv_ssd1306_draw_char(dev, cursor_x, cursor_y, *text, color, scale);
        cursor_x = (int16_t)(cursor_x + advance);

        if (cursor_x >= (int16_t)dev->cfg.width)
        {
            break;
        }

        text++;
    }

    return DRV_SSD1306_OK;
}

drv_ssd1306_result_t drv_ssd1306_draw_text_line(
    drv_ssd1306_t *dev,
    uint8_t line,
    const char *text,
    drv_ssd1306_color_t color)
{
    if ((dev == NULL) || (text == NULL))
    {
        return DRV_SSD1306_ERROR_NULL;
    }

    const uint8_t max_lines = (uint8_t)(dev->cfg.height / TEXT_LINE_HEIGHT);

    if (line >= max_lines)
    {
        return drv_ssd1306_record_result(dev, DRV_SSD1306_ERROR_CONFIG);
    }

    const int16_t y = (int16_t)(line * TEXT_LINE_HEIGHT);

    (void)drv_ssd1306_fill_rect(
        dev,
        0,
        y,
        (int16_t)dev->cfg.width,
        TEXT_LINE_HEIGHT,
        DRV_SSD1306_COLOR_BLACK);

    return drv_ssd1306_draw_string(dev, 0, y, text, color, 1u);
}