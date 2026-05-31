#ifndef TASK_OLED_UI_H
#define TASK_OLED_UI_H

#include <stdbool.h>
#include <stdint.h>

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TASK_OLED_UI_TEXT_MAX_LEN     64u
#define TASK_OLED_UI_LINE_MAX_LEN     22u
#define TASK_OLED_UI_LINE_COUNT       8u

typedef enum
{
    TASK_OLED_UI_PAGE_TERMINAL = 0,
    TASK_OLED_UI_PAGE_LINES = 1
} task_oled_ui_page_t;

bool task_oled_ui_start(I2C_HandleTypeDef *hi2c);

bool task_oled_ui_clear(void);
bool task_oled_ui_print(const char *text);
bool task_oled_ui_printf(const char *format, ...);

bool task_oled_ui_set_line(uint8_t line, const char *text);
bool task_oled_ui_clear_line(uint8_t line);

bool task_oled_ui_select_page(task_oled_ui_page_t page);
bool task_oled_ui_redraw(void);
bool task_oled_ui_enable(bool enabled);

#ifdef __cplusplus
}
#endif

#endif