#ifndef CONFIG_APP_H
#define CONFIG_APP_H

#include "cmsis_os2.h"

#define APP_DEBUG_UART_BAUDRATE       115200u
#define APP_DEBUG_TASK_PERIOD_MS      1000u

#define APP_DEBUG_TASK_STACK_BYTES    2048u
#define APP_DEBUG_TASK_PRIORITY       osPriorityLow

#endif