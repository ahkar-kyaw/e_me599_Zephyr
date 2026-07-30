# UI subsystem

The UI subsystem provides a read-only local interface on the Waveshare
1.5-inch RGB OLED module. The display uses an SSD1351 controller,
128 x 128 RGB565 framebuffer, and four-wire SPI.

Portable UI code has no ESP-IDF, FreeRTOS, GPIO, SPI-host, or board-pin
dependencies.

## Current RadioMaster Pocket mapping

| Channel | Radio control | Current UI function |
| --- | --- | --- |
| CH1 | Ail, right stick left/right | Previous/next page in browse mode |
| CH2 | Thr, right stick up/down | Previous/next selection in interact mode |
| CH3 | Ele, left stick up/down | Unassigned |
| CH4 | Rud, left stick left/right | Unassigned |
| CH5 | SA, latching switch | Unassigned |
| CH6 | SD, latching switch | Interact mode off/on |
| CH7 | SE, momentary switch | Enter |
| CH8 | S1, non-return potentiometer | Reserved for future value adjustment |
| CH9 | SB, three-position switch | Unassigned |
| CH10 | SC, three-position switch | UI input lock/enable |

Channel numbers in `config_ui.h` are one-based so they match the EdgeTX
channel monitor.

## Input lock

SC must be in its high position before the UI accepts navigation,
interaction, or Enter events.

```text
SC low or middle
    Lock UI input.
    Return to browse mode.
    Ignore CH1, CH2, CH6, and CH7.

SC high
    Enable UI input after an intentional low/middle-to-high transition.
```

After boot or RC link recovery, SC must first be moved to low or middle
and then moved high. This prevents a transmitter that powers up with SC
already high from immediately enabling UI input.

When SC enables input:

```text
CH1 and CH2
    Must be neutral before an axis event can occur.

CH6
    Must be inactive before interact mode can be entered.

CH7
    Must be released before an Enter event can occur.
```

## UI state machine

```text
LOCKED
    SC high transition -> BROWSE

BROWSE
    CH1 left/right -> previous/next top-level page
    CH6 active -> INTERACT
    SC inactive or RC link lost -> LOCKED

INTERACT
    CH2 up/down -> previous/next selection or view
    CH7 press -> Enter event
    CH6 inactive -> BROWSE
    SC inactive or RC link lost -> LOCKED
```

Both stick axes require a return to neutral before another event. Holding
a stick does not auto-repeat. An interact-mode transition suppresses
simultaneous axis and Enter events.

Enter is counted and reported by `task_ui`, but the current pages remain
read-only.

## Current pages

```text
STATUS
    RC state and freshness.
    IMU state and freshness.
    Large roll and pitch values.

CRSF
    Selection 1 shows CH1-CH8.
    Selection 2 shows CH9-CH16.
    Shows link state, age, CRC errors, and parser errors.

IMU
    Selection 1 shows attitude, pitch rate, and calibration state.
    Selection 2 shows acceleration, gyro, temperature, and norm.
```

The layout uses:

```text
Top bar
    Page name and LOCK/BROWSE/INTERACT state.

Content area
    Only page-relevant values.

Bottom bar
    Compact RC and IMU health indicators.
    Page number and current selection number.
```

Permanent control instructions are intentionally omitted to reduce
clutter.

## Module responsibilities

```text
ui_rc_input
    Maps configured CRSF channels to semantic UI events.
    Owns input gating, thresholds, hysteresis, neutral re-arm, and
    reconnect behavior.

ui_state
    Owns locked/browse/interact behavior, page, selection, and Enter
    count.

ui_canvas
    Provides portable RGB565 rectangles, compact text, and font
    rendering.

ui_pages
    Formats STATUS, CRSF, and IMU snapshots on a ui_canvas.

drv_ssd1351
    Owns SSD1351 initialization, RGB565 framebuffer storage format,
    controller commands, and display updates.

bsp_display_spi_esp32
    Owns the ESP32 SPI3 device, D/C pin, reset pin, and transfer
    chunking.

task_ui
    Owns the display instance and static framebuffer.
    Collects RC and IMU snapshots.
    Runs safety freshness checks.
    Feeds the portable input and UI state machines.
```

## Reassigning controls

Edit:

```text
04_firmware/platforms/esp32/src/config/config_ui.h
```

Current assignments:

```c
#define APP_UI_RC_PAGE_AXIS_CHANNEL      1u
#define APP_UI_RC_VERTICAL_AXIS_CHANNEL  2u
#define APP_UI_RC_INTERACT_CHANNEL       6u
#define APP_UI_RC_ENTER_CHANNEL          7u
#define APP_UI_RC_INPUT_ENABLE_CHANNEL  10u
```

Direction and polarity are also configurable:

```c
#define APP_UI_RC_PAGE_RIGHT_HIGH          1
#define APP_UI_RC_VERTICAL_UP_HIGH         1
#define APP_UI_RC_INTERACT_ACTIVE_HIGH     1
#define APP_UI_RC_ENTER_ACTIVE_HIGH        1
#define APP_UI_RC_INPUT_ENABLE_ACTIVE_HIGH 1
```

All five assigned channels must be unique. Invalid assignments or
threshold ordering prevent the UI task from starting.

SC uses separate thresholds so only its high position enables input.
Its low and middle positions lock input. The action-switch thresholds
remain suitable for the two-position SD switch and momentary SE switch.

## CRSF raw channel values

CRSF packed channels use 11-bit fields, but the normal control range is
narrower than the numeric `0-2047` field capacity.

```text
Raw 172     Extended low endpoint, approximately 988 us
Raw 191     Nominal low endpoint, 1000 us
Raw 992     Center endpoint, 1500 us
Raw 1792    Nominal high endpoint, 2000 us
Raw 1811    Extended high endpoint, approximately 2012 us
```

Zephyr accepts `172-1811` in `config_rc.h` as the protocol validity
range. UI neutral and activation thresholds are separate values in
`config_ui.h`.

## Future tuning boundary

The current UI is read-only. Future tuning should introduce an explicit
edit state and a configuration service.

```text
UI requests configuration mode.
Supervisor accepts only from a non-moving safe state.
Safety continues denying motion.
CH2 selects a setting.
S1 adjusts a pending value.
SE confirms an explicit action.
Control configuration validates bounds.
An explicit save action writes persistent storage.
```

The UI must never write controller globals, motor commands, torque
state, or persistent storage directly.
