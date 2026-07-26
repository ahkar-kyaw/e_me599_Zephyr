# ESP32 platform firmware

The ESP32 platform is the active bring-up target. It uses PlatformIO with ESP-IDF and runs the early IMU, estimator, safety, and balance-state pipeline.

## Structure

```text
platforms/esp32/
    CMakeLists.txt
    platformio.ini
    sdkconfig.defaults

    include/
        board_esp32_nodemcu_v1.h

    src/
        CMakeLists.txt
        main.c

        board/
            board_led.c
            board_led.h

        bsp/
            bsp_imu_spi_esp32.c
            bsp_imu_spi_esp32.h

        config/
            config_imu.h

        tasks/
            task_imu.c
            task_imu.h

        tests/
            test_debug_led.c
            test_debug_led.h
            test_imu_snapshot.c
            test_imu_snapshot.h
```

## Build and upload

Run from the repository root.

```bash
pio run -d 04_firmware/platforms/esp32
pio run -d 04_firmware/platforms/esp32 -t upload
pio device monitor -d 04_firmware/platforms/esp32 -b 115200
```

## PlatformIO configuration

```text
platformio.ini
    Selects the ESP32 board.
    Selects ESP-IDF as the framework.
    Defines board and app build flags.
```

Current flags:

```text
APP_PLATFORM_ESP32
BOARD_ESP32_NODEMCU_V1
APP_ENABLE_BRINGUP_TESTS
```

## Board configuration

```text
include/board_esp32_nodemcu_v1.h
    Defines the board name.
    Defines debug LED pin and active level.
    Defines IMU SPI host, pins, clock, and mode.
    Defines optional IMU interrupt pins.
```

Current IMU pin map:

```text
SCLK    GPIO18
MISO    GPIO19
MOSI    GPIO23
CS      GPIO27
INT1    GPIO34
INT2    GPIO35
```

INT1 and INT2 are defined for future use. The current firmware polls the IMU and does not use those pins yet.

## BSP layer

```text
bsp_imu_spi_esp32
    Initializes the ESP32 SPI bus.
    Adds the IMU SPI device.
    Implements the if_spi transfer function used by the common driver.
```

## Task layer

```text
task_imu
    Owns the IMU device.
    Initializes the SPI BSP.
    Initializes the ISM330DHCX driver.
    Applies mounting orientation.
    Performs gyro bias calibration.
    Runs the attitude estimator.
    Publishes imu_snapshot_t.
```

## Test layer

```text
test_debug_led
    Temporary board LED bring-up test.

test_imu_snapshot
    Reads the latest IMU snapshot.
    Runs IMU safety checks.
    Extracts balance_state_t.
    Prints the pipeline to the serial monitor.
```

Tests are only started when APP_ENABLE_BRINGUP_TESTS is enabled.

## IMU configuration

```text
src/config/config_imu.h
    Sets IMU task period.
    Sets calibration sample count.
    Sets accelerometer ODR and full-scale range.
    Sets gyroscope ODR and full-scale range.
    Sets body-frame mounting orientation.
    Sets estimator algorithm and sensor mode.
    Sets safety thresholds.
```

Body frame convention:

```text
Body X positive
    robot front

Body Y positive
    robot left

Body Z positive
    robot up
```

## ESP32 design rules

```text
Keep pin definitions in include/board_*.h.
Keep ESP-IDF SPI setup in bsp/.
Keep long-running FreeRTOS tasks in tasks/.
Keep temporary bring-up code in tests/.
Do not include ESP-IDF headers in common/.
Do not let tests own hardware that is already owned by a task.
```
