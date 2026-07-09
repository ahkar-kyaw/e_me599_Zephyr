#include "app_main.h"

#include "task_debug.h"

#if APP_ENABLE_BRINGUP_TESTS
#include "test_debug_led.h"
#endif

void app_main_create_tasks(void)
{
#if APP_ENABLE_BRINGUP_TESTS
    test_debug_led_start();
#endif

    task_debug_start();
}