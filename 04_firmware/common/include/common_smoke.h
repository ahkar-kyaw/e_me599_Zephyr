#ifndef COMMON_SMOKE_H
#define COMMON_SMOKE_H

#include <stdint.h>

#define COMMON_SMOKE_MAGIC 0xE5992026u

static inline const char *common_smoke_name(void)
{
    return "common_smoke";
}

static inline uint32_t common_smoke_magic(void)
{
    return COMMON_SMOKE_MAGIC;
}

#endif