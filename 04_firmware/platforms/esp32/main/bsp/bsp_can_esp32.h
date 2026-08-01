#ifndef BSP_CAN_ESP32_H
#define BSP_CAN_ESP32_H

#include "interfaces/if_can.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BSP_CAN_ESP32_OK = 0,
    BSP_CAN_ESP32_TIMEOUT = 1,
    BSP_CAN_ESP32_ERROR = -1
} bsp_can_esp32_result_t;

typedef enum
{
    BSP_CAN_ESP32_BUS_UNKNOWN = 0,
    BSP_CAN_ESP32_BUS_ACTIVE,
    BSP_CAN_ESP32_BUS_WARNING,
    BSP_CAN_ESP32_BUS_PASSIVE,
    BSP_CAN_ESP32_BUS_OFF
} bsp_can_esp32_bus_state_t;

typedef struct
{
    bsp_can_esp32_bus_state_t state;
    uint32_t bus_error_count;
    uint32_t rx_queue_overflow_count;
    uint32_t recovery_count;
    bool initialized;
} bsp_can_esp32_status_t;

bsp_can_esp32_result_t bsp_can_esp32_init(uint32_t bitrate);

bsp_can_esp32_result_t
bsp_can_esp32_receive(if_can_frame_t *frame, uint32_t timeout_ms);

bsp_can_esp32_result_t
bsp_can_esp32_transmit(const if_can_frame_t *frame, uint32_t timeout_ms);

bsp_can_esp32_result_t
bsp_can_esp32_get_status(bsp_can_esp32_status_t *status);

bsp_can_esp32_result_t bsp_can_esp32_recover_if_bus_off(void);

#ifdef __cplusplus
}
#endif

#endif
