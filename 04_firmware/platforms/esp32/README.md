# ESP32 platform firmware

The ESP32 platform is the active bring-up target. It uses PlatformIO
with ESP-IDF and runs the IMU pipeline, CRSF receiver, and SSD1306
status UI.

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
            bsp_crsf_uart_esp32.c
            bsp_crsf_uart_esp32.h
            bsp_imu_spi_esp32.c
            bsp_imu_spi_esp32.h
            bsp_oled_i2c_esp32.c
            bsp_oled_i2c_esp32.h

        config/
            config_imu.h
            config_rc.h
            config_ui.h

        tasks/
            task_imu.c
            task_imu.h
            task_rc.c
            task_rc.h
            task_ui.c
            task_ui.h

        tests/
            test_debug_led.c
            test_debug_led.h
            test_imu_snapshot.c
            test_imu_snapshot.h
            test_rc_snapshot.c
            test_rc_snapshot.h
```

## Build and upload

Run from the repository root.

```bash
pio run -d 04_firmware/platforms/esp32
pio run -d 04_firmware/platforms/esp32 -t upload
pio device monitor -d 04_firmware/platforms/esp32 -b 115200
```

## Build-time flags

```text
APP_PLATFORM_ESP32
BOARD_ESP32_NODEMCU_V1
APP_ENABLE_RC_RECEIVER
APP_ENABLE_OLED_UI
APP_ENABLE_BRINGUP_TESTS
```

`APP_ENABLE_RC_RECEIVER` starts the receiver task.
`APP_ENABLE_OLED_UI` starts the display task.
`APP_ENABLE_BRINGUP_TESTS` starts the LED, IMU, and RC diagnostic tasks.

## Board pin map

```text
IMU SCLK        GPIO18
IMU MISO        GPIO19
IMU MOSI        GPIO23
IMU CS          GPIO27
IMU INT1        GPIO34
IMU INT2        GPIO35

OLED SDA        GPIO21
OLED SCL        GPIO22

CRSF UART RX    GPIO16, connected to RP2 TX
CRSF UART TX    GPIO17, connected to RP2 RX
```

INT1 and INT2 are defined for future use. The IMU is currently polled.
The full electrical pin map and power notes are in
`03_electrical/docs/pin_map_esp32_bringup.md`.

## BSP layer

```text
bsp_imu_spi_esp32
    Initializes the ESP32 SPI bus.
    Implements if_spi for drv_ism330dhcx.

bsp_oled_i2c_esp32
    Initializes I2C port 0 at 400 kHz.
    Implements if_i2c for drv_ssd1306.

bsp_crsf_uart_esp32
    Initializes UART2 for non-inverted 420000-baud CRSF.
    Provides buffered UART reads to task_rc.
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
    Owns the OLED.
    Shows RC status and alternates between channels 1-8 and 9-16.
    Does not own safety state or motion commands.
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
```

Tests read task-owned snapshots. They do not own the underlying hardware
and cannot grant permission to move.

## Configuration

```text
config_imu.h
    IMU sampling, mounting, estimation, calibration, and safety values.

config_rc.h
    RC freshness, raw channel range, UART read timeout, and retry values.

config_ui.h
    SSD1306 geometry, address, orientation, contrast, and UI timing.
```

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
