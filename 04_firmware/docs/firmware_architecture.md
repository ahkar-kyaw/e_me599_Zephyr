# Firmware architecture

The firmware is organized around clear ownership boundaries. Each layer owns one type of responsibility and depends only on lower-level or peer interfaces that are stable.

## High-level layers

```text
Application startup
    app_main or app_main_create_tasks

Platform tasks
    task_imu
    future task_control
    future task_safety
    future task_motor
    future task_ui
    future task_log

Platform BSP
    SPI, CAN, I2C, UART, GPIO, timers, ADC, and board-specific hardware backends

Common app glue
    app_imu_types
    app_balance_state

Common subsystem logic
    drivers
    estimation
    safety
    control
    protocols
    services

Common interfaces
    if_spi
    future if_can, if_i2c, if_uart, if_time
```

## Current IMU pipeline

```text
ESP32 SPI pins
    |
    v
bsp_imu_spi_esp32
    |
    v
if_spi_t
    |
    v
drv_ism330dhcx
    |
    v
est_imu_mount
    |
    v
est_imu_calibration
    |
    v
est_attitude
    |
    v
imu_snapshot_t
    |
    +--> safety_imu_check
    |       |
    |       v
    |   safety_imu_status_t
    |
    +--> app_balance_state_from_imu
            |
            v
        balance_state_t
```

## Ownership rules

```text
task_imu
    Owns IMU hardware access.
    Publishes the latest IMU snapshot.

drv_ism330dhcx
    Owns ISM330DHCX register operations and raw-data conversion.

est_imu_mount
    Owns sensor-frame to body-frame mapping.

est_imu_calibration
    Owns gyro bias estimation and correction.

est_attitude
    Owns attitude estimation.

safety_imu
    Owns IMU-level validity and freshness checks.

app_balance_state
    Owns the conversion from IMU and safety status to control-facing balance state.

ctrl_balance_types
    Owns only the control-facing balance state definition.
```

## Planned full robot task flow

```text
Supervisor task
    Owns whole-system mode.
    Coordinates safe idle, calibration, ready, armed, balancing, controlled stop, fault, and estop states.

Safety task
    Owns permission to move.
    Checks IMU, RC link, battery, motor feedback, command freshness, and system mode.

Control task
    Owns desired command calculation.
    Uses balance_state_t and user command inputs.
    Produces desired robot or motor commands.

Motor task
    Owns physical motor communication.
    Sends only safety-approved commands.

UI task
    Owns display and user interaction.
    Sends requests through queues or service interfaces.
    Does not directly enable torque or write motor commands.

Logging task
    Records snapshots, faults, command data, and experiment data.
```

## Data passing rules

```text
Use snapshots for latest state.
Use queues for events that must not be missed.
Use state machines for system mode, safety, UI, calibration, logging, and subsystem behavior.
Use timestamps and validity flags for data freshness.
```

## Build ownership

```text
common/
    Portable source files are compiled into each platform target as needed.

platforms/esp32/src/CMakeLists.txt
    Adds ESP32 files and selected common files to the ESP-IDF component.

platforms/stm32/app/CMakeLists.txt
    Adds user-owned STM32 app files and selected common files to the STM32 target.
```
