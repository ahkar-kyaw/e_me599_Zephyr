# CubeMars actuator subsystem

## Current scope

The current implementation provides CubeMars AK40-10 feedback and a
deliberately limited manual velocity commissioning path in servo mode.
It:

```text
uses one 1 Mbit/s classic CAN bus
supports four configurable actuator slots
enables motor 1 at CAN ID 69 decimal
decodes periodic servo feedback
publishes timestamped actuator snapshots
checks feedback freshness
records CAN, decode, queue, and unknown-ID counters
requests ESP32 TWAI recovery after bus-off
allows one selected actuator to be driven at a time
limits the manual command to 1000 electrical RPM by default
requires explicit UI, supervisor, safety, and arm-switch approval
```

The manual path is for an unloaded bench test. It is not the final robot
motion mode and does not replace a hardware E-stop, power contactor, or
the actuator's configured CAN command timeout.

## Layer ownership

```text
CubeMars AK40-10
    |
    v
TJA1051T/3 CAN transceiver
    |
    v
bsp_can_esp32
    |
    v
if_can_frame_t
    |
    v
proto_cubemars_ak
    |
    v
task_motor
    |
    v
actuator_snapshot_t
    |
    +--> test_actuator_snapshot
    +--> task_safety
    +--> task_ui
```

Responsibilities:

```text
if_can
    Portable classic CAN frame representation.

proto_cubemars_ak
    Portable CubeMars servo frame packing and feedback decoding.
    Contains no ESP-IDF, FreeRTOS, pin, or board dependencies.

bsp_can_esp32
    Owns ESP-IDF TWAI setup, ISR-to-task receive queue, bus status, and
    bus-off recovery.

task_motor
    Owns the CAN BSP.
    Matches feedback to configured actuator slots.
    Publishes the latest snapshot for all four actuators.
    Transmits only fresh, safety-approved manual velocity commands.
    Applies a local command freshness check and zero-command stop hold.

task_supervisor
    Owns SAFE_IDLE and MANUAL_DRIVE whole-system modes.
    Accepts a fresh CAN-page request from task_ui.

task_safety
    Owns permission to move during manual commissioning.
    Checks RC, CAN, actuator feedback, driver fault, selected slot,
    neutral stick, and fresh supervisor mode.

test_actuator_snapshot
    Reads and logs the task-owned snapshot.
    Does not own CAN and cannot command an actuator.
```

## Actuator configuration

Open:

```text
04_firmware/platforms/esp32/main/config/config_actuator.h
```

The current entries are:

```text
Slot    Enabled    CAN ID
1       yes        69
2       no         70
3       no         71
4       no         72
```

IDs are decimal in this file. ID 69 is hexadecimal `0x45`. The other
three IDs are reserved configuration values, not assumptions about the
motors. Before enabling them, assign each physical actuator a unique
CAN ID with the CubeMars configuration tool, update the matching
`APP_ACTUATOR_n_CAN_ID`, and set `APP_ACTUATOR_n_ENABLED` to `true`.

All actuators on this bus must use:

```text
CAN bitrate        1 Mbit/s
Frame format       classic CAN 2.0 extended, 29-bit identifier
Control mode       servo
Feedback function  0x29
Feedback DLC       8 bytes
```

Configure timed servo feedback in the actuator. The CubeMars driver
supports a configured feedback rate from 1 Hz to 500 Hz. Use 100 Hz for
initial robot integration so the current 100 ms freshness limit has
clear margin.

## Servo protocol support

The portable protocol module currently provides encoders for:

```text
current
velocity
position
position with velocity and acceleration limits
```

It also decodes periodic servo feedback:

```text
position in degrees
velocity in electrical RPM
current in amperes
driver temperature in degrees Celsius
driver fault code
```

The distinction between electrical RPM and output-shaft RPM is
intentional. Conversion to mechanical units depends on motor pole pairs
and gear ratio and belongs above the wire protocol.

The active manual path uses servo velocity command function `0x03`, a
29-bit extended identifier, and a signed four-byte big-endian ERPM
value. Origin-setting, persistent configuration writes, runtime mode
switching, current mode, position mode, and MIT commands are not wired
to runtime control.

## Manual-drive ownership and safety path

```text
task_ui
    publishes a fresh request only while the CAN page, Interact mode,
    and SD remain active
    |
    v
task_supervisor
    owns SAFE_IDLE or MANUAL_DRIVE mode
    |
    v
task_safety + safety_manual_drive
    validates all commissioning gates and publishes the only approved
    command snapshot
    |
    v
task_motor
    checks command freshness and owns CAN transmission
```

Motion requires all of the following:

```text
CAN page selected
SC UI input gate enabled
SD Interact mode enabled
actuator slot 1 configured
RC snapshot valid and fresh
CAN bus active
M1 feedback fresh and fault-free
CH2 throttle centered when SD is enabled
```

The UI cannot grant permission or write CAN commands. SD requests
manual-drive mode; the supervisor and safety state machine still decide
whether motion is permitted. Dropping SD or SC, leaving the CAN page,
losing RC or feedback, a CAN state change, a motor fault, or a stale
approved command initiates a zero-velocity stop sequence. After a
running fault, cycle SD off and on before rearming.

## Manual bench-test controls

```text
CH1 Ail    Browse to the CAN page.
CH6 SD     Enable or disable manual drive on the CAN page.
CH2 Thr    Command -1000 to +1000 ERPM after the display says LIVE.
CH10 SC    UI input gate; dropping it cancels the request.
```

The simplified commissioning path currently targets slot M1. A separate
motor-selection input can be added when additional actuators are
enabled. All mappings, polarities, limits, freshness times, and the
zero-command hold time are in
`platforms/esp32/main/config/config_actuator.h`.

## Future MIT mode

MIT mode should be added beside servo mode inside
`proto_cubemars_ak`, not inside the ESP32 BSP. Add a tagged portable
command type only when the control design requires MIT fields. The CAN
BSP, four-slot configuration, feedback snapshot, and task ownership can
remain unchanged.

The future STM32 implementation should provide an FDCAN BSP that reads
and writes `if_can_frame_t`. It should reuse `proto_cubemars_ak` and
the application snapshot types without including STM32 HAL headers in
`common/`.

## Manual-drive bring-up sequence

Keep the actuator mechanically unloaded and securely restrained. Have
an immediate physical method to remove actuator power. Configure the
actuator's CAN command timeout before relying on remote commands.

```text
1. Start with SD low, SC low, and CH2 centered.
2. Verify CAN feedback is OK and the actuator fault code is zero.
3. Move SC high, then browse to the CAN page with CH1.
4. Keep CH2 centered and move SD high.
5. Confirm the display says DRV LIVE with command zero.
6. Move CH2 slightly and verify command and feedback direction.
7. Center CH2, then move SD low before ending the test.
8. Confirm DRV STOPPING, followed by DRV OFF.
```

Expected monitor tag:

```text
test_actuator
```

The current CAN bring-up configuration enables this read-only logger,
suppresses recurring non-CAN application logs, and prints one combined
line every two seconds. Change
`APP_ACTUATOR_DIAGNOSTIC_PERIOD_MS` in `config_actuator.h` to adjust the
period. ESP32 ROM and bootloader messages may still appear once before
the application applies its log filter.

The normal serial line intentionally contains only:

```text
CAN           ESP32 TWAI bus state
M1            WAIT, OK, STALE, or FAULT
P             output position in degrees
V             electrical RPM, not output-shaft RPM
I             measured current in amperes
T             driver temperature in degrees Celsius
F             CubeMars driver fault code
DRV           OFF, BLOCK, WAIT, LIVE, or STOP
CMD           safety-approved velocity command in ERPM
TXE           failed CAN command transmission attempts
```

No command is transmitted automatically at boot. Transmission begins
only after the complete manual-drive sequence above.

## Primary references

- CubeMars, `AK-series-driver-manual` for AK 2.0 robotic actuators.
- CubeMars, AK40-10 KV170 product specifications and downloads.
- Espressif, ESP-IDF 5.5 TWAI programming guide.
- Adafruit, CAN Pal pinout guide.
