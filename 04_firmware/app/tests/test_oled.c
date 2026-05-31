#include "test_oled.h"

#include <stdio.h>

#include "cmsis_os2.h"
#include "drv_ssd1306.h"
#include "i2c.h"
#include "test_debug_uart.h"
#include "usart.h"

#define TEST_OLED_TASK_STACK_BYTES        2048u
#define TEST_OLED_TASK_PERIOD_MS          500u
#define TEST_OLED_RETRY_PERIOD_MS         1000u

static osThreadId_t s_test_oled_task_handle = NULL;
static drv_ssd1306_t s_oled;

static const char *test_oled_result_name(drv_ssd1306_result_t result)
{
    switch (result)
    {
        case DRV_SSD1306_OK: return "OK";
        case DRV_SSD1306_ERROR_NULL: return "NULL";
        case DRV_SSD1306_ERROR_CONFIG: return "CONFIG";
        case DRV_SSD1306_ERROR_NOT_READY: return "NOT_READY";
        case DRV_SSD1306_ERROR_I2C: return "I2C";
        case DRV_SSD1306_ERROR_NOT_INITIALIZED: return "NOT_INIT";
        case DRV_SSD1306_ERROR_UNSUPPORTED: return "UNSUP";
        default: return "UNKNOWN";
    }
}

static bool test_oled_init_display(void)
{
    drv_ssd1306_config_t cfg;
    drv_ssd1306_default_config(&cfg, &hi2c1);

    cfg.i2c_addr_7bit = DRV_SSD1306_I2C_ADDR_3C;
    cfg.height = DRV_SSD1306_HEIGHT_64;
    cfg.contrast = 0x7Fu;
    cfg.rotate_180 = false;
    cfg.external_vcc = false;
    cfg.i2c_timeout_ms = 20u;

    const drv_ssd1306_result_t result = drv_ssd1306_init(&s_oled, &cfg);

    printf("OLED init result=%s addr=0x%02X\r\n",
           test_oled_result_name(result),
           cfg.i2c_addr_7bit);

    return (result == DRV_SSD1306_OK);
}

static void test_oled_task(void *argument)
{
    (void)argument;

    test_debug_uart_init(&huart3);

    printf("\r\nOLED test task starting\r\n");

    bool oled_ok = test_oled_init_display();
    uint32_t counter = 0u;

    for (;;)
    {
        if (!oled_ok)
        {
            osDelay(TEST_OLED_RETRY_PERIOD_MS);
            oled_ok = test_oled_init_display();
            continue;
        }

        char line[24];

        (void)drv_ssd1306_clear(&s_oled, DRV_SSD1306_COLOR_BLACK);

        (void)drv_ssd1306_draw_text_line(&s_oled, 0u, "ZEPHYR OLED", DRV_SSD1306_COLOR_WHITE);
        (void)drv_ssd1306_draw_text_line(&s_oled, 1u, "SSD1306 I2C OK", DRV_SSD1306_COLOR_WHITE);

        snprintf(line, sizeof(line), "TICK %lu", (unsigned long)osKernelGetTickCount());
        (void)drv_ssd1306_draw_text_line(&s_oled, 2u, line, DRV_SSD1306_COLOR_WHITE);

        snprintf(line, sizeof(line), "COUNT %lu", (unsigned long)counter);
        (void)drv_ssd1306_draw_text_line(&s_oled, 3u, line, DRV_SSD1306_COLOR_WHITE);

        (void)drv_ssd1306_draw_text_line(&s_oled, 5u, "SAFE DEBUG ONLY", DRV_SSD1306_COLOR_WHITE);

        drv_ssd1306_status_t status;
        (void)drv_ssd1306_get_status(&s_oled, &status);

        snprintf(line, sizeof(line), "ERR %lu", (unsigned long)status.error_count);
        (void)drv_ssd1306_draw_text_line(&s_oled, 7u, line, DRV_SSD1306_COLOR_WHITE);

        const drv_ssd1306_result_t update_result = drv_ssd1306_update(&s_oled);

        if (update_result != DRV_SSD1306_OK)
        {
            printf("OLED update failed result=%s\r\n", test_oled_result_name(update_result));
            oled_ok = false;
        }

        counter++;
        osDelay(TEST_OLED_TASK_PERIOD_MS);
    }
}

void test_oled_start(void)
{
    static const osThreadAttr_t task_attributes =
    {
        .name = "test_oled",
        .stack_size = TEST_OLED_TASK_STACK_BYTES,
        .priority = (osPriority_t)osPriorityLow
    };

    if (s_test_oled_task_handle == NULL)
    {
        s_test_oled_task_handle = osThreadNew(test_oled_task, NULL, &task_attributes);
    }
}