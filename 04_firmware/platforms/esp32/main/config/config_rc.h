#ifndef CONFIG_RC_H
#define CONFIG_RC_H

#include "protocols/proto_crsf.h"

#define APP_RC_MAX_AGE_US              100000u
#define APP_RC_CHANNEL_MIN             PROTO_CRSF_CHANNEL_VALUE_MIN
#define APP_RC_CHANNEL_MAX             PROTO_CRSF_CHANNEL_VALUE_MAX
#define APP_RC_UART_READ_TIMEOUT_MS    20u
#define APP_RC_RETRY_MS                1000u

#endif
