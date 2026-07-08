#ifndef ROBOT_COMMON_SMOKE_H
#define ROBOT_COMMON_SMOKE_H

#include <stdint.h>

#define ROBOT_COMMON_SMOKE_MAGIC 0xE5992026u

static inline const char *robot_common_smoke_name(void)
{
    return "robot_common_smoke";
}

static inline uint32_t robot_common_smoke_magic(void)
{
    return ROBOT_COMMON_SMOKE_MAGIC;
}

#endif