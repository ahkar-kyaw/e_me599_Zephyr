#include "task_oled_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "drv_ssd1306.h"

#define TASK_OLED_UI_STACK_BYTES          2048u
#define TASK_OLED_UI_QUEUE_DEPTH          16u

#define TASK_OLED_UI_I2C_TIMEOUT_MS       20u
#define TASK_OLED_UI_PERIOD_MS            50u
#define TASK_OLED_UI_RETRY_MS             1000u
#define TASK_OLED_UI_MIN_REDRAW_MS        100u

#define TASK_OLED_UI_COL_COUNT            21u

typedef enum
{
    OLED_UI_CMD_PRINT = 0,
    OLED_UI_CMD_CLEAR,
    OLED_UI_CMD_SET_LINE,
    OLED_UI_CMD_CLEAR_LINE,
    OLED_UI_CMD_SET_SCREEN,
    OLED_UI_CMD_SELECT_PAGE,
    OLED_UI_CMD_REDRAW,
    OLED_UI_CMD_ENABLE
} oled_ui_cmd_id_t;

typedef struct
{
    oled_ui_cmd_id_t id;
    uint8_t line;
    uint8_t page;
    bool flag;
    char text[TASK_OLED_UI_TEXT_MAX_LEN];
    task_oled_ui_screen_t screen;
} oled_ui_cmd_t;

typedef struct
{
    char terminal_lines[TASK_OLED_UI_LINE_COUNT][TASK_OLED_UI_LINE_MAX_LEN];
    char manual_lines[TASK_OLED_UI_LINE_COUNT][TASK_OLED_UI_LINE_MAX_LEN];

    uint8_t terminal_row;
    uint8_t terminal_col;

    task_oled_ui_page_t active_page;
    bool enabled;
    bool dirty;
} oled_ui_state_t;

static osThreadId_t s_oled_thread_id = NULL;
static osMessageQueueId_t s_oled_cmd_queue = NULL;

static StaticTask_t s_oled_task_cb;
static uint64_t s_oled_task_stack[TASK_OLED_UI_STACK_BYTES / sizeof(uint64_t)];

static StaticQueue_t s_oled_queue_cb;
static uint64_t s_oled_queue_mem[
    ((TASK_OLED_UI_QUEUE_DEPTH * sizeof(oled_ui_cmd_t)) + sizeof(uint64_t) - 1u) / sizeof(uint64_t)
];

static I2C_HandleTypeDef *s_hi2c = NULL;
static drv_ssd1306_t s_oled;
static oled_ui_state_t s_state;

static void task_oled_ui_thread(void *argument);

static void oled_ui_clear_lines(char lines[TASK_OLED_UI_LINE_COUNT][TASK_OLED_UI_LINE_MAX_LEN])
{
    for (uint8_t i = 0u; i < TASK_OLED_UI_LINE_COUNT; i++)
    {
        lines[i][0] = '\0';
    }
}

static void oled_ui_reset_state(void)
{
    memset(&s_state, 0, sizeof(s_state));

    oled_ui_clear_lines(s_state.terminal_lines);
    oled_ui_clear_lines(s_state.manual_lines);

    s_state.terminal_row = 0u;
    s_state.terminal_col = 0u;
    s_state.active_page = TASK_OLED_UI_PAGE_TERMINAL;
    s_state.enabled = true;
    s_state.dirty = true;
}

static bool oled_ui_post_cmd(const oled_ui_cmd_t *cmd)
{
    if ((cmd == NULL) || (s_oled_cmd_queue == NULL))
    {
        return false;
    }

    return (osMessageQueuePut(s_oled_cmd_queue, cmd, 0u, 0u) == osOK);
}

static void oled_ui_scroll_terminal(void)
{
    for (uint8_t row = 0u; row < (TASK_OLED_UI_LINE_COUNT - 1u); row++)
    {
        memcpy(
            s_state.terminal_lines[row],
            s_state.terminal_lines[row + 1u],
            TASK_OLED_UI_LINE_MAX_LEN);
    }

    s_state.terminal_lines[TASK_OLED_UI_LINE_COUNT - 1u][0] = '\0';
    s_state.terminal_row = TASK_OLED_UI_LINE_COUNT - 1u;
    s_state.terminal_col = 0u;
}

static void oled_ui_newline(void)
{
    if (s_state.terminal_row >= (TASK_OLED_UI_LINE_COUNT - 1u))
    {
        oled_ui_scroll_terminal();
    }
    else
    {
        s_state.terminal_row++;
        s_state.terminal_col = 0u;
        s_state.terminal_lines[s_state.terminal_row][0] = '\0';
    }
}

static void oled_ui_put_char(char c)
{
    if (c == '\r')
    {
        return;
    }

    if (c == '\n')
    {
        oled_ui_newline();
        return;
    }

    if (c == '\t')
    {
        c = ' ';
    }

    if (s_state.terminal_col >= TASK_OLED_UI_COL_COUNT)
    {
        oled_ui_newline();
    }

    s_state.terminal_lines[s_state.terminal_row][s_state.terminal_col] = c;
    s_state.terminal_col++;

    if (s_state.terminal_col < TASK_OLED_UI_LINE_MAX_LEN)
    {
        s_state.terminal_lines[s_state.terminal_row][s_state.terminal_col] = '\0';
    }
    else
    {
        s_state.terminal_lines[s_state.terminal_row][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
    }

    if (s_state.terminal_col >= TASK_OLED_UI_COL_COUNT)
    {
        oled_ui_newline();
    }
}

static void oled_ui_print_text(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        oled_ui_put_char(*text);
        text++;
    }

    s_state.active_page = TASK_OLED_UI_PAGE_TERMINAL;
    s_state.dirty = true;
}

static void oled_ui_set_manual_line(uint8_t line, const char *text)
{
    if (line >= TASK_OLED_UI_LINE_COUNT)
    {
        return;
    }

    if (text == NULL)
    {
        s_state.manual_lines[line][0] = '\0';
    }
    else
    {
        strncpy(s_state.manual_lines[line], text, TASK_OLED_UI_LINE_MAX_LEN - 1u);
        s_state.manual_lines[line][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
    }

    s_state.active_page = TASK_OLED_UI_PAGE_LINES;
    s_state.dirty = true;
}

static void oled_ui_set_manual_screen(const task_oled_ui_screen_t screen)
{
    for (uint8_t line = 0u; line < TASK_OLED_UI_LINE_COUNT; line++)
    {
        strncpy(
            s_state.manual_lines[line],
            screen[line],
            TASK_OLED_UI_LINE_MAX_LEN - 1u);

        s_state.manual_lines[line][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
    }

    s_state.active_page = TASK_OLED_UI_PAGE_LINES;
    s_state.dirty = true;
}

static void oled_ui_apply_cmd(const oled_ui_cmd_t *cmd)
{
    if (cmd == NULL)
    {
        return;
    }

    switch (cmd->id)
    {
        case OLED_UI_CMD_PRINT:
            oled_ui_print_text(cmd->text);
            break;

        case OLED_UI_CMD_CLEAR:
            if (s_state.active_page == TASK_OLED_UI_PAGE_LINES)
            {
                oled_ui_clear_lines(s_state.manual_lines);
            }
            else
            {
                oled_ui_clear_lines(s_state.terminal_lines);
                s_state.terminal_row = 0u;
                s_state.terminal_col = 0u;
            }

            s_state.dirty = true;
            break;

        case OLED_UI_CMD_SET_SCREEN:
            oled_ui_set_manual_screen(cmd->screen);
            break;

        case OLED_UI_CMD_SET_LINE:
            oled_ui_set_manual_line(cmd->line, cmd->text);
            break;

        case OLED_UI_CMD_CLEAR_LINE:
            oled_ui_set_manual_line(cmd->line, "");
            break;

        case OLED_UI_CMD_SELECT_PAGE:
            if (cmd->page == (uint8_t)TASK_OLED_UI_PAGE_LINES)
            {
                s_state.active_page = TASK_OLED_UI_PAGE_LINES;
            }
            else
            {
                s_state.active_page = TASK_OLED_UI_PAGE_TERMINAL;
            }

            s_state.dirty = true;
            break;

        case OLED_UI_CMD_REDRAW:
            s_state.dirty = true;
            break;

        case OLED_UI_CMD_ENABLE:
            s_state.enabled = cmd->flag;
            s_state.dirty = true;
            break;

        default:
            break;
    }
}

static bool oled_ui_init_display_at_addr(uint8_t addr_7bit)
{
    drv_ssd1306_config_t cfg;

    drv_ssd1306_default_config(&cfg, s_hi2c);

    cfg.i2c_addr_7bit = addr_7bit;
    cfg.height = DRV_SSD1306_HEIGHT_64;
    cfg.contrast = 0x7Fu;
    cfg.external_vcc = false;
    cfg.rotate_180 = false;
    cfg.i2c_timeout_ms = TASK_OLED_UI_I2C_TIMEOUT_MS;

    return (drv_ssd1306_init(&s_oled, &cfg) == DRV_SSD1306_OK);
}

static bool oled_ui_init_display(void)
{
    if (s_hi2c == NULL)
    {
        return false;
    }

    if (oled_ui_init_display_at_addr(DRV_SSD1306_I2C_ADDR_3C))
    {
        return true;
    }

    if (oled_ui_init_display_at_addr(DRV_SSD1306_I2C_ADDR_3D))
    {
        return true;
    }

    return false;
}

static void oled_ui_draw_page(void)
{
    const char (*lines)[TASK_OLED_UI_LINE_MAX_LEN];

    if (!s_state.enabled)
    {
        (void)drv_ssd1306_clear(&s_oled, DRV_SSD1306_COLOR_BLACK);
        (void)drv_ssd1306_update(&s_oled);
        return;
    }

    if (s_state.active_page == TASK_OLED_UI_PAGE_LINES)
    {
        lines = s_state.manual_lines;
    }
    else
    {
        lines = s_state.terminal_lines;
    }

    (void)drv_ssd1306_clear(&s_oled, DRV_SSD1306_COLOR_BLACK);

    for (uint8_t row = 0u; row < TASK_OLED_UI_LINE_COUNT; row++)
    {
        if (lines[row][0] != '\0')
        {
            (void)drv_ssd1306_draw_text_line(
                &s_oled,
                row,
                lines[row],
                DRV_SSD1306_COLOR_WHITE);
        }
    }
}

static void task_oled_ui_thread(void *argument)
{
    (void)argument;

    bool display_ready = false;
    uint32_t last_retry_tick = 0u;
    uint32_t last_redraw_tick = 0u;

    oled_ui_reset_state();
    oled_ui_print_text("OLED UI START\n");

    for (;;)
    {
        oled_ui_cmd_t cmd;

        while (osMessageQueueGet(s_oled_cmd_queue, &cmd, NULL, 0u) == osOK)
        {
            oled_ui_apply_cmd(&cmd);
        }

        const uint32_t now = osKernelGetTickCount();

        if (!display_ready)
        {
            if ((now - last_retry_tick) >= TASK_OLED_UI_RETRY_MS)
            {
                last_retry_tick = now;
                display_ready = oled_ui_init_display();

                if (display_ready)
                {
                    s_state.dirty = true;
                }
            }

            osDelay(TASK_OLED_UI_PERIOD_MS);
            continue;
        }

        if (s_state.dirty && ((now - last_redraw_tick) >= TASK_OLED_UI_MIN_REDRAW_MS))
        {
            oled_ui_draw_page();

            if (drv_ssd1306_update(&s_oled) == DRV_SSD1306_OK)
            {
                s_state.dirty = false;
                last_redraw_tick = now;
            }
            else
            {
                display_ready = false;
            }
        }

        osDelay(TASK_OLED_UI_PERIOD_MS);
    }
}

bool task_oled_ui_start(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL)
    {
        return false;
    }

    if (s_oled_thread_id != NULL)
    {
        return true;
    }

    s_hi2c = hi2c;

    static const osMessageQueueAttr_t queue_attr =
    {
        .name = "oled_ui_q",
        .attr_bits = 0u,
        .cb_mem = &s_oled_queue_cb,
        .cb_size = sizeof(s_oled_queue_cb),
        .mq_mem = s_oled_queue_mem,
        .mq_size = sizeof(s_oled_queue_mem)
    };

    s_oled_cmd_queue = osMessageQueueNew(
        TASK_OLED_UI_QUEUE_DEPTH,
        sizeof(oled_ui_cmd_t),
        &queue_attr);

    if (s_oled_cmd_queue == NULL)
    {
        return false;
    }

    static const osThreadAttr_t thread_attr =
    {
        .name = "oled_ui",
        .attr_bits = 0u,
        .cb_mem = &s_oled_task_cb,
        .cb_size = sizeof(s_oled_task_cb),
        .stack_mem = s_oled_task_stack,
        .stack_size = sizeof(s_oled_task_stack),
        .priority = (osPriority_t)osPriorityLow
    };

    s_oled_thread_id = osThreadNew(task_oled_ui_thread, NULL, &thread_attr);

    return (s_oled_thread_id != NULL);
}

bool task_oled_ui_clear(void)
{
    oled_ui_cmd_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_CLEAR;

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_print(const char *text)
{
    oled_ui_cmd_t cmd;

    if (text == NULL)
    {
        return false;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_PRINT;

    strncpy(cmd.text, text, sizeof(cmd.text) - 1u);
    cmd.text[sizeof(cmd.text) - 1u] = '\0';

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_printf(const char *format, ...)
{
    oled_ui_cmd_t cmd;
    va_list args;

    if (format == NULL)
    {
        return false;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_PRINT;

    va_start(args, format);
    (void)vsnprintf(cmd.text, sizeof(cmd.text), format, args);
    va_end(args);

    cmd.text[sizeof(cmd.text) - 1u] = '\0';

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_set_screen(const task_oled_ui_screen_t screen)
{
    oled_ui_cmd_t cmd;

    if (screen == NULL)
    {
        return false;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_SET_SCREEN;

    for (uint8_t line = 0u; line < TASK_OLED_UI_LINE_COUNT; line++)
    {
        strncpy(
            cmd.screen[line],
            screen[line],
            TASK_OLED_UI_LINE_MAX_LEN - 1u);

        cmd.screen[line][TASK_OLED_UI_LINE_MAX_LEN - 1u] = '\0';
    }

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_set_line(uint8_t line, const char *text)
{
    oled_ui_cmd_t cmd;

    if ((line >= TASK_OLED_UI_LINE_COUNT) || (text == NULL))
    {
        return false;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_SET_LINE;
    cmd.line = line;

    strncpy(cmd.text, text, sizeof(cmd.text) - 1u);
    cmd.text[sizeof(cmd.text) - 1u] = '\0';

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_clear_line(uint8_t line)
{
    oled_ui_cmd_t cmd;

    if (line >= TASK_OLED_UI_LINE_COUNT)
    {
        return false;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_CLEAR_LINE;
    cmd.line = line;

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_select_page(task_oled_ui_page_t page)
{
    oled_ui_cmd_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_SELECT_PAGE;
    cmd.page = (uint8_t)page;

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_redraw(void)
{
    oled_ui_cmd_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_REDRAW;

    return oled_ui_post_cmd(&cmd);
}

bool task_oled_ui_enable(bool enabled)
{
    oled_ui_cmd_t cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.id = OLED_UI_CMD_ENABLE;
    cmd.flag = enabled;

    return oled_ui_post_cmd(&cmd);
}