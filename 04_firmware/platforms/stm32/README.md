# STM32 platform firmware

The STM32 platform is the target firmware path for the STM32H723ZG Nucleo. It uses STM32CubeMX generated code together with user-owned application code and CMake presets.

## Structure

```text
platforms/stm32/
    CMakeLists.txt
    CMakePresets.json
    STM32H723XG_FLASH.ld
    startup_stm32h723xx.s
    zephyr.ioc

    Core/
        CubeMX generated application entry, peripheral init, and FreeRTOS setup

    Drivers/
        CubeMX generated CMSIS and STM32 HAL drivers

    cmake/
        toolchain files
        STM32CubeMX generated CMake integration

    include/
        board_stm32h723_v1.h

    app/
        app_main.c
        app_main.h
```

## Build

Run from the STM32 platform folder.

```bash
cd 04_firmware/platforms/stm32
cmake --preset Debug
cmake --build --preset Debug
```

Release build:

```bash
cd 04_firmware/platforms/stm32
cmake --preset Release
cmake --build --preset Release
```

## CubeMX boundary

```text
Core/
    CubeMX generated code.
    Preserve USER CODE sections when editing generated files.

Drivers/
    CubeMX generated or vendor code.

app/
    User-owned application code.
    Safe place for board wrappers, BSPs, tasks, configuration, and
    platform application logic as they are migrated.
```

## Application entry flow

```text
Core/Src/main.c
    CubeMX generated startup
    initializes HAL, clock, GPIO, peripherals, and RTOS

Core/Src/freertos.c
    CubeMX generated RTOS setup
    calls app_main_create_tasks inside USER CODE section

app/app_main.c
    is the user-owned task creation hook
    currently creates no tasks until the ESP32 subsystems are migrated
```

## STM32 design rules

```text
Keep CubeMX generated code separate from user-owned app code.
Keep board metadata in include/board_*.h.
Keep GPIO macro mapping in app/board modules.
Keep temporary tests under app/tests/.
Keep long-running tasks under app/tasks/.
Add common modules through app/CMakeLists.txt only when needed.
Use only cmake/gcc-arm-none-eabi.cmake for the supported toolchain.
```
