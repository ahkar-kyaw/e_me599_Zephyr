# ESP32 platform firmware

The ESP32 platform is the active bring-up target. It uses native
ESP-IDF with CMake and Ninja and runs the IMU pipeline, CRSF receiver,
SSD1351 status UI, CubeMars feedback monitor, and a safety-gated manual
velocity commissioning path.

## Structure

```text
platforms/esp32/
    CMakeLists.txt
    sdkconfig.defaults

    include/
        board_esp32_nodemcu_v1.h

    main/
        CMakeLists.txt
        main.c

        board/
            board_led.c
            board_led.h

        bsp/
            bsp_can_esp32.c
            bsp_can_esp32.h
            bsp_crsf_uart_esp32.c
            bsp_crsf_uart_esp32.h
            bsp_display_spi_esp32.c
            bsp_display_spi_esp32.h
            bsp_imu_spi_esp32.c
            bsp_imu_spi_esp32.h

        config/
            config_actuator.h
            config_imu.h
            config_rc.h
            config_ui.h

        tasks/
            task_imu.c
            task_imu.h
            task_motor.c
            task_motor.h
            task_rc.c
            task_rc.h
            task_safety.c
            task_safety.h
            task_supervisor.c
            task_supervisor.h
            task_ui.c
            task_ui.h

        tests/
            test_actuator_snapshot.c
            test_actuator_snapshot.h
            test_debug_led.c
            test_debug_led.h
            test_imu_snapshot.c
            test_imu_snapshot.h
            test_rc_snapshot.c
            test_rc_snapshot.h
```

## Toolchain

The current firmware is validated with Espressif's ESP-IDF `v5.5`
release tag. Activate the ESP-IDF environment before running any
`idf.py` command:

```bash
. ~/esp/esp-idf-v5.5/export.sh
```

The generated `sdkconfig` and `build/` directory are local build
artifacts. Project-owned defaults belong in `sdkconfig.defaults`.

## Configure, build, and upload

Run from the repository root.

```bash
cd 04_firmware/platforms/esp32
idf.py set-target esp32
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

`idf.py set-target esp32` is required after cloning or when resetting
the generated configuration. It does not need to run before every
build. Use `idf.py menuconfig` for interactive ESP-IDF configuration.
Exit the serial monitor with `Ctrl+]`.

## Build-time flags

```text
APP_PLATFORM_ESP32
BOARD_ESP32_NODEMCU_V1
APP_ENABLE_ACTUATORS
APP_ENABLE_ACTUATOR_DIAGNOSTICS
APP_ENABLE_CAN_ONLY_LOGGING
APP_ENABLE_DEBUG_LED_TEST
APP_ENABLE_IMU_DIAGNOSTICS
APP_ENABLE_RC_RECEIVER
APP_ENABLE_RC_DIAGNOSTICS
APP_ENABLE_UI
APP_ENABLE_BRINGUP_TESTS
```

`APP_ENABLE_ACTUATORS` starts the motor feedback and safety-approved
command task. No command is transmitted automatically at boot.
`APP_ENABLE_ACTUATOR_DIAGNOSTICS` starts the read-only actuator serial
logger when bring-up tests are also enabled.
`APP_ENABLE_CAN_ONLY_LOGGING` suppresses recurring non-CAN application
logs during CAN bring-up.
`APP_ENABLE_DEBUG_LED_TEST`, `APP_ENABLE_IMU_DIAGNOSTICS`, and
`APP_ENABLE_RC_DIAGNOSTICS` independently control the other diagnostics.
`APP_ENABLE_RC_RECEIVER` starts the receiver task.
`APP_ENABLE_UI` starts the display task.
`APP_ENABLE_BRINGUP_TESTS` is the master diagnostic enable.

The current CAN bring-up configuration enables the actuator logger and
disables the LED, IMU, and RC diagnostic tasks. The logger produces one
line every two seconds. ESP32 ROM and bootloader messages may still
appear once before application logging is configured.
The current values are defined in `main/CMakeLists.txt`.

## Board pin map

```text
IMU SCLK        GPIO18
IMU MISO        GPIO19
IMU MOSI        GPIO23
IMU CS          GPIO27
IMU INT1        GPIO34
IMU INT2        GPIO35

Display CLK     GPIO14
Display DIN     GPIO13
Display CS      GPIO25
Display D/C     GPIO21
Display RST     GPIO22

CRSF UART RX    GPIO16, connected to RP2 TX
CRSF UART TX    GPIO17, connected to RP2 RX

CAN TX          GPIO26, connected to CAN Pal TX
CAN RX          GPIO32, connected to CAN Pal RX
```

INT1 and INT2 are defined for future use. The IMU is currently polled.
The full electrical pin map and power notes are in
`03_electrical/docs/pin_map_esp32_bringup.md`.

## BSP layer

```text
bsp_imu_spi_esp32
    Initializes the ESP32 SPI bus.
    Implements if_spi for drv_ism330dhcx.

bsp_display_spi_esp32
    Initializes the dedicated ESP32 SPI3 display bus at 8 MHz.
    Owns display D/C and reset GPIO.
    Implements if_display_io for drv_ssd1351.

bsp_crsf_uart_esp32
    Initializes UART2 for non-inverted 420000-baud CRSF.
    Provides buffered UART reads to task_rc.

bsp_can_esp32
    Initializes the ESP32 TWAI controller at 1 Mbit/s.
    Receives classic CAN frames through an ISR-to-task queue.
    Reports bus state and supports bus-off recovery.
```

## Task layer

```text
task_imu
    Owns the IMU device and publishes imu_snapshot_t.

task_rc
    Owns the receiver UART.
    Feeds bytes to the portable CRSF parser.
    Publishes all 16 channels in rc_snapshot_t with timestamp and
    communication error counters.

task_ui
    Owns the SSD1351 and static RGB565 framebuffer.
    Collects RC, IMU, and actuator snapshots.
    Runs the portable UI input mapper and state machine.
    Renders STATUS, CRSF, IMU, and CAN pages.
    Publishes a timestamped manual-drive request.
    Does not own safety state or motion commands.

task_supervisor
    Owns SAFE_IDLE and MANUAL_DRIVE system modes.

task_safety
    Owns manual-drive permission.
    Checks RC, CAN, feedback, fault, neutral-stick, and supervisor gates.
    Publishes the only approved command accepted by task_motor.

task_motor
    Owns the TWAI BSP.
    Decodes CubeMars periodic servo feedback.
    Publishes timestamped feedback for four configurable actuator slots.
    Transmits only fresh, safety-approved velocity commands.
    Sends no command automatically at boot.
```

## Test layer

```text
test_debug_led
    Temporary board LED bring-up test.

test_imu_snapshot
    Prints the IMU, estimator, safety, and balance-state pipeline.

test_rc_snapshot
    Runs RC validity, freshness, and range checks.
    Prints all 16 raw CRSF channels and error counters.

test_actuator_snapshot
    Prints essential CAN state, actuator feedback, drive command, and
    transmit-error status in one line every two seconds.
    Reads task_motor snapshots and cannot command an actuator.
```

Tests read task-owned snapshots. They do not own the underlying hardware
and cannot grant permission to move.

## Configuration

```text
config_imu.h
    IMU sampling, mounting, estimation, calibration, and safety values.

config_rc.h
    RC freshness, 10 active channels, raw range, UART timeout, and retry.

config_ui.h
    SSD1351 master contrast and UI timing.
    One-based RC channel assignments, directions, polarities, and
    input thresholds.

config_actuator.h
    CAN bitrate, freshness, and four actuator enable/ID entries.
    Manual-drive channel, polarity, 1000 ERPM limit, command freshness,
    and zero-command hold time.
    Motor 1 is enabled at decimal CAN ID 69.
    Motors 2 through 4 are disabled until their unique physical IDs are
    assigned and verified.
```

Current UI mapping:

```text
CH1 Ail    previous/next page in browse mode
CH2 Thr    bounded M1 velocity while CAN DRIVE is live
CH6 SD     interaction off/on; enables drive on CAN
CH7 SE     Enter; not required for manual drive
CH10 SC    UI input lock/enable
```

See `04_firmware/docs/ui_subsystem.md` for UI behavior, channel
reassignment, CRSF raw ranges, and the future tuning boundary.
See `04_firmware/docs/actuator_subsystem.md` for CubeMars protocol,
wiring, configuration, safety ownership, and the MIT-mode growth path.

Body frame convention:

```text
Body X positive    robot front
Body Y positive    robot left
Body Z positive    robot up
```

## ESP32 design rules

```text
Keep pin definitions in include/board_*.h.
Keep ESP-IDF peripheral setup in bsp/.
Keep long-running FreeRTOS tasks in tasks/.
Keep temporary bring-up code in tests/.
Do not include ESP-IDF or FreeRTOS headers in common/.
Do not let tests own hardware that is already owned by a task.
```
