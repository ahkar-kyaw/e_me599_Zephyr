# Common firmware layer

The common layer contains portable firmware modules that can be reused by ESP32, STM32, and future platforms.

## Purpose

```text
common/
    Owns reusable robot logic.
    Does not own MCU startup.
    Does not own pin mapping.
    Does not own RTOS task creation.
    Does not include vendor HAL headers.
```

## Current structure

```text
common/
    include/
        app/
            app_balance_state.h
            app_imu_types.h

        control/
            ctrl_balance_types.h

        drivers/
            drv_ism330dhcx.h

        estimation/
            est_attitude.h
            est_imu_calibration.h
            est_imu_mount.h

        interfaces/
            if_spi.h

        safety/
            safety_imu.h

    src/
        app/
            app_balance_state.c

        drivers/
            drv_ism330dhcx.c

        estimation/
            est_attitude.c
            est_imu_calibration.c
            est_imu_mount.c

        safety/
            safety_imu.c
```

## Module responsibilities

### interfaces

```text
if_spi
    Defines a generic SPI transfer function.
    Allows common drivers to use SPI without knowing the MCU backend.
```

### drivers

```text
drv_ism330dhcx
    Portable ISM330DHCX register driver.
    Probes WHO_AM_I.
    Configures ODR and full-scale settings.
    Reads raw temperature, gyro, and accelerometer data.
    Converts raw counts to SI units using the active driver configuration.
```

### estimation

```text
est_imu_mount
    Maps sensor-frame acceleration and gyro data into the robot body frame.
    Body X is robot front.
    Body Y is robot left.
    Body Z is robot up.

est_imu_calibration
    Collects stationary gyro samples.
    Computes gyro bias.
    Applies bias correction to gyro data.

est_attitude
    Provides a generic attitude-estimator API.
    Currently implements a complementary filter.
    Reserves algorithm modes for Madgwick and Kalman filters.
    Supports 6DOF inputs now and 9DOF inputs later.
```

### safety

```text
safety_imu
    Checks whether the IMU data is safe for balance use.
    Checks validity, calibration, attitude validity, timestamp freshness, tilt, and acceleration magnitude.
```

### control

```text
ctrl_balance_types
    Defines balance_state_t only.
    Does not depend on IMU or safety modules.
```

### app

```text
app_imu_types
    Defines imu_snapshot_t.
    This is the shared IMU state published by platform tasks.

app_balance_state
    Converts imu_snapshot_t and safety_imu_status_t into balance_state_t.
    This keeps control types independent from IMU and safety details.
```

## Dependency direction

```text
interfaces
    used by drivers

drivers
    used by platform tasks and estimation modules

estimation
    used by platform tasks

safety
    used by app glue and future safety tasks

control
    used by app glue and future control tasks

app
    allowed to connect subsystem types together
```

## Common-layer rules

```text
Allowed:
    stdint.h
    stdbool.h
    stddef.h
    string.h
    math.h
    common project headers

Not allowed:
    ESP-IDF headers
    STM32 HAL headers
    CubeMX generated headers
    FreeRTOS headers
    GPIO pin numbers
    MCU register access
    board names
```
