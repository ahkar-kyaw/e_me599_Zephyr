# Zephyr firmware

Zephyr firmware is organized to support an ESP32 bring-up platform and an STM32H723ZG target platform while keeping reusable robot logic portable. The firmware is split into a common layer and platform-specific layers.

## Current targets

| Target | Purpose | Toolchain |
| --- | --- | --- |
| ESP32 NodeMCU | Prototype bring-up and hardware validation | PlatformIO with ESP-IDF |
| STM32H723ZG Nucleo | Final target platform | STM32CubeMX, CMake, Ninja, Arm GNU Toolchain |

## Repository layout

```text
04_firmware/
    common/
        include/
        src/

    platforms/
        esp32/
        stm32/

    docs/
```

## Layer responsibilities

```text
common/
    Portable robot logic.
    No ESP-IDF headers.
    No STM32 HAL headers.
    No FreeRTOS headers.
    No MCU pin numbers.
    No board-specific assumptions.

platforms/esp32/
    ESP32 startup, build files, board definitions, BSPs, FreeRTOS tasks, and bring-up tests.

platforms/stm32/
    STM32CubeMX generated code, STM32 app code, board support, build files, and bring-up tests.

docs/
    Firmware architecture notes, naming rules, subsystem notes, and electrical interface documentation.
```

## Active ESP32 firmware flow

```text
app_main
    starts task_imu
    starts bring-up tests when APP_ENABLE_BRINGUP_TESTS is enabled

task_imu
    owns the IMU hardware
    initializes ESP32 SPI through the BSP
    initializes the ISM330DHCX driver
    applies IMU mounting orientation
    performs gyro bias calibration
    runs the attitude estimator
    publishes imu_snapshot_t

test_imu_snapshot
    reads imu_snapshot_t
    runs IMU safety checks
    extracts balance_state_t
    prints the full IMU pipeline
```

## Common firmware flow

```text
if_spi
    generic SPI transfer interface

drv_ism330dhcx
    portable ISM330DHCX register driver

est_imu_mount
    maps sensor-frame measurements into robot body frame

est_imu_calibration
    estimates and removes gyro bias

est_attitude
    generic attitude estimator API
    currently uses a complementary filter
    prepared for Madgwick or Kalman later

safety_imu
    checks IMU validity, calibration, freshness, tilt, and acceleration magnitude

app_balance_state
    converts imu_snapshot_t and safety_imu_status_t into balance_state_t

ctrl_balance_types
    defines the control-facing balance state type
```

## ESP32 build commands

Run these from the repository root.

```bash
pio run -d 04_firmware/platforms/esp32
pio run -d 04_firmware/platforms/esp32 -t upload
pio device monitor -d 04_firmware/platforms/esp32 -b 115200
```

## STM32 build commands

Run these from the STM32 platform folder.

```bash
cd 04_firmware/platforms/stm32
cmake --preset Debug
cmake --build --preset Debug
```

## Build-time flags

```text
APP_ENABLE_BRINGUP_TESTS
    Enables temporary bring-up tests.
    ESP32 sets this in platformio.ini.
    STM32 sets this in CMakePresets.json.
```

## Development rules

```text
Keep common code portable.
Keep platform code isolated.
Keep temporary tests under tests/.
Keep long-term tasks under tasks/.
Keep board pin maps in board headers.
Keep MCU peripheral setup in BSP modules.
Keep safety checks separate from control calculations.
Do not let UI, tests, or control write directly to motors.
```

## Documentation map

```text
04_firmware/README.md
    Firmware entry point and build guide.

04_firmware/common/README.md
    Portable common-layer organization.

04_firmware/platforms/esp32/README.md
    ESP32 bring-up platform guide.

04_firmware/platforms/stm32/README.md
    STM32 target platform guide.

04_firmware/docs/firmware_architecture.md
    Firmware architecture and data flow.

04_firmware/docs/naming_conventions.md
    File, module, task, and macro naming rules.

04_firmware/docs/imu_subsystem.md
    IMU driver, calibration, mounting, estimator, safety, and balance-state pipeline.

04_firmware/docs/electrical_interfaces.md
    Firmware-facing electrical interface notes and pin maps.
```
