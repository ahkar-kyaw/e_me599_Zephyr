#ifndef IF_I2C_H
#define IF_I2C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IF_I2C_OK   0
#define IF_I2C_ERR -1

typedef int (*if_i2c_write_fn)(void *context,
                               uint8_t address,
                               const uint8_t *data,
                               size_t length);

typedef struct
{
    void *context;
    if_i2c_write_fn write;
} if_i2c_t;

#ifdef __cplusplus
}
#endif

#endif
