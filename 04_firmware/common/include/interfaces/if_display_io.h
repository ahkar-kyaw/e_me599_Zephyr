#ifndef IF_DISPLAY_IO_H
#define IF_DISPLAY_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_DISPLAY_IO_OK   0
#define IF_DISPLAY_IO_ERR -1

typedef int (*if_display_write_command_fn)(void *context,
                                           uint8_t command);
typedef int (*if_display_write_data_fn)(void *context,
                                        const uint8_t *data,
                                        size_t length);
typedef void (*if_display_set_reset_fn)(void *context, bool high);
typedef void (*if_display_delay_ms_fn)(void *context, uint32_t delay_ms);

typedef struct
{
    void *context;
    if_display_write_command_fn write_command;
    if_display_write_data_fn write_data;
    if_display_set_reset_fn set_reset;
    if_display_delay_ms_fn delay_ms;
} if_display_io_t;

#ifdef __cplusplus
}
#endif

#endif
