# Firmware naming conventions

These conventions keep firmware modules predictable as the project grows.

## Prefixes

```text
app_
    Application-level glue and shared application types.

board_
    Board support wrappers and board-level abstractions.

bsp_
    MCU peripheral backend support.

drv_
    Device drivers.

est_
    Estimation and sensor processing.

ctrl_
    Control-facing types and control algorithms.

safety_
    Safety checks, fault checks, and motion permission logic.

ui_
    User-interface input mapping, state machines, and rendering.

proto_
    Protocol parsers and encoders.

svc_
    Services and request handlers.

task_
    Long-running RTOS tasks.

test_
    Temporary bring-up and diagnostic tests.

config_
    Platform-local configuration headers.

if_
    Portable hardware interface contracts.
```

## File placement rules

```text
common/include/<module>/
    Public headers for portable modules.

common/src/<module>/
    Source files for portable modules.

platforms/<target>/include/
    Board headers and platform-wide public target headers.

platforms/<target>/src/board/
    Board support modules.

platforms/<target>/src/bsp/
    Peripheral backend implementations.

platforms/<target>/src/config/
    Platform-local configuration headers.

platforms/<target>/src/tasks/
    RTOS task modules.

platforms/<target>/src/tests/
    Temporary bring-up and diagnostic modules.
```

## Macro naming

```text
APP_ENABLE_...
    Build-time application feature flags.

APP_IMU_...
    IMU application configuration values.

BOARD_...
    Board identity, board pins, board electrical settings, and board-level constants.

DRV_...
    Driver constants, driver enums, driver return codes, and driver configuration values.
```

Avoid generic `CONFIG_...` application macros in ESP-IDF projects because ESP-IDF uses `CONFIG_...` heavily for sdkconfig and Kconfig values.

## Type naming

```text
*_t
    C struct, enum, or typedef type.

*_config_t
    Configuration object.

*_status_t
    Status object.

*_snapshot_t
    Latest-state snapshot object.

*_state_t
    State-machine state or control-facing state.
```

## Function naming

```text
module_init
    Initialize a module instance.

module_start
    Start a task or long-running subsystem.

module_update
    Run one update step.

module_get_snapshot
    Copy the latest snapshot.

module_check
    Run a safety or validity check.

module_from_...
    Convert from one representation to another.
```

## Current naming examples

```text
drv_ism330dhcx_init_config
    Driver initialization with explicit configuration.

est_imu_mount_apply_data
    Apply mounting transform to IMU data.

est_attitude_update
    Run one estimator update step.

safety_imu_check
    Check whether IMU data is safe for balance.

app_balance_state_from_imu
    Convert IMU and safety status into balance_state_t.

task_imu_get_snapshot
    Copy the latest IMU snapshot from the task-owned state.
```
