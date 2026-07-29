# UI subsystem

The UI subsystem provides a read-only status interface on the SSD1306
OLED. It uses CRSF controls for page navigation while keeping display
logic independent from ESP-IDF, FreeRTOS, GPIO pins, and UART details.

## Current RadioMaster Pocket mapping

The transmitter currently sends ten configured channels.

| Channel | Radio control | Function |
| --- | --- | --- |
| CH1 | Ail, right stick left/right | UI page or subpage axis |
| CH2 | Thr, right stick up/down | Not used by UI |
| CH3 | Ele, left stick up/down | Not used by UI |
| CH4 | Rud, left stick left/right | Not used by UI |
| CH5 | SA, latching switch | Not used by UI |
| CH6 | SD, latching switch | UI interaction on/off |
| CH7 | SE, momentary switch | UI Enter event |
| CH8 | S1, non-return potentiometer | Reserved for future adjustment |
| CH9 | SB, three-position switch | Not used by UI |
| CH10 | SC, three-position switch | Not used by UI |

Channel numbers in `config_ui.h` are one-based so they match the
transmitter channel monitor.

## User interaction

### Browse mode

```text
CH1 left
    Previous page.

CH1 right
    Next page.

CH6 inactive
    Remain in browse mode.
```

CH1 must return to its neutral region before another left or right event
is generated. Holding the stick does not repeatedly cycle pages.

### Interact mode

```text
CH6 active
    Lock the current top-level page and enter interaction.

CH1 left or right
    Change the current page's subpage.

CH7 momentary press
    Generate an Enter event for the current page and subpage.
```

Enter is wired through the state machine and reported by `task_ui`, but
the current read-only pages do not perform configuration actions.

If the RC link becomes invalid or stale, the state machine returns to
browse mode and clears the subpage. After reconnecting, CH6 must be
moved inactive before interaction can be entered again. This prevents
automatic interaction when the transmitter reconnects with CH6 already
active.

The input mapper also validates the three assigned UI channels against
the CRSF protocol range. Unused CRSF channels do not control whether UI
input is accepted.

## Current pages

```text
STATUS
    RC state and age.
    IMU state and age.
    Roll and pitch.
    IMU sample and error counts.
    RC and IMU fault flags.

CRSF
    Four subpages.
    Shows CH1-CH4, CH5-CH8, CH9-CH12, or CH13-CH16.
    Shows link age and parser error counters.

IMU
    Two subpages.
    Shows attitude and pitch rate.
    Shows acceleration, gyro, temperature, and raw temperature.
```

The header shows `B` for browse or `I` for interact and the current
one-based subpage number.

## Module responsibilities

```text
ui_rc_input
    Maps configured CRSF channels to semantic UI events.
    Owns thresholds, hysteresis, neutral re-arm, and safe reconnect
    behavior.

ui_state
    Owns browse/interact mode, page, subpage, and Enter count.
    Does not know about ESP32, FreeRTOS, or the OLED.

ui_pages
    Formats STATUS, CRSF, and IMU snapshots into the SSD1306
    framebuffer.

task_ui
    Owns OLED initialization and updates.
    Collects task-owned RC and IMU snapshots.
    Runs safety freshness checks.
    Feeds events into the UI state machine.
```

## Reassigning controls

Edit:

```text
04_firmware/platforms/esp32/src/config/config_ui.h
```

Current assignments:

```c
#define APP_UI_RC_AXIS_CHANNEL         1u
#define APP_UI_RC_INTERACT_CHANNEL     6u
#define APP_UI_RC_ENTER_CHANNEL        7u
```

Change these one-based channel numbers to move a UI function to another
transmitter channel. Switch polarity is also configurable:

```c
#define APP_UI_RC_INTERACT_ACTIVE_HIGH 1
#define APP_UI_RC_ENTER_ACTIVE_HIGH    1
```

Set a polarity macro to `0` if that control should be active at the low
CRSF endpoint.

## CRSF raw channel values

CRSF packed channels use 11-bit fields, but the normal legacy channel
range is narrower than the numeric `0-2047` field capacity.

```text
Raw 172     Extended low endpoint, approximately 988 us
Raw 191     Nominal low endpoint, 1000 us
Raw 992     Center endpoint, 1500 us
Raw 1792    Nominal high endpoint, 2000 us
Raw 1811    Extended high endpoint, approximately 2012 us
```

The `191-1792` range observed on the RadioMaster Pocket is the expected
nominal output range. Zephyr accepts `172-1811` in `config_rc.h` as the
protocol validity range. UI activation, neutral, and switch thresholds
are separate values in `config_ui.h`.

Changing `APP_RC_CHANNEL_MIN` and `APP_RC_CHANNEL_MAX` changes the RC
safety validity check; it does not change what the transmitter sends.
Transmitter endpoint changes are made in the EdgeTX model Outputs
settings. Extended transmitter limits are not needed for the current UI.

## Future tuning boundary

The current UI is read-only. Future tuning should add an edit state only
after the supervisor and control-configuration owner exist.

```text
UI requests configuration mode.
Supervisor accepts only from a non-moving safe state.
Safety continues denying motion.
UI edits a pending value.
Control configuration validates bounds.
An explicit Enter action applies the value.
An explicit save action writes persistent storage.
```

The UI must not write controller globals, motor commands, torque state,
or persistent storage directly.
