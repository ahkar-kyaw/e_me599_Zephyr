#ifndef TASK_LOG_H
#define TASK_LOG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool task_log_start(void);
bool task_log_write(const char *text);
bool task_log_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif