# Electrical interfaces

This document records firmware-facing electrical assumptions. It should be updated whenever wiring, connectors, pin assignments, or target boards change.

## ESP32 bring-up pin map

### Debug LED

```text
Signal              ESP32 GPIO
BOARD_LED_DEBUG1    GPIO2
```

GPIO2 is used only for the onboard debug LED during bring-up.

### ISM330DHCX IMU over SPI

```text
Signal      ESP32 GPIO      Notes
SCLK        GPIO18          SPI clock
MISO        GPIO19          IMU SDO to ESP32 MISO
MOSI        GPIO23          ESP32 MOSI to IMU SDI
CS          GPIO27          SPI chip select
INT1        GPIO34          Optional interrupt input, unused now
INT2        GPIO35          Optional interrupt input, unused now
VDD         3.3 V           IMU supply
VDDIO       3.3 V           IMU I/O supply
GND         GND             Common ground
```

Current firmware polling does not use INT1 or INT2. Future firmware may use INT1 for data-ready timing and INT2 for FIFO or event interrupts.

## STM32 Nucleo LED map

```text
Signal      STM32 pin       CubeMX label
LD1         PB0             DBG_LED1
LD2         PE1             DBG_LED2
LD3         PB14            DBG_LED3
```

Application code should use `board_led.h` rather than CubeMX GPIO macros directly.

## Future electrical interfaces

The following sections should be completed as the electrical design matures.

### CAN motor bus

```text
Purpose
    Communicate with CubeMars AK40-10 motors.

Expected hardware
    STM32 FDCAN or ESP32 TWAI
    TJA1051T/3 CAN transceiver
    CANH and CANL connector
    Optional 120 ohm termination jumper
    TVS diode footprint
    Common-mode choke footprint if needed
```

Document these when assigned:

```text
MCU CAN_TX pin
MCU CAN_RX pin
CAN transceiver VIO
CAN transceiver VCC
CAN silent or standby control pin
CAN connector pinout
Termination strategy
```

### RadioMaster RP2 ExpressLRS receiver

```text
Purpose
    Receive user commands through CRSF.

ESP32 interface
    UART2
    GPIO16 receives from RP2 TX
    GPIO17 transmits to RP2 RX
    420000 baud, 8N1
    Non-inverted, full-duplex CRSF

Power
    RP2 VCC uses regulated 5 V
    RP2 GND and ESP32 GND must be common
```

The current firmware receives RC channel frames on GPIO16. GPIO17 is
assigned and should be wired for future CRSF telemetry, but the firmware
does not transmit telemetry yet.

```text
Suggested connector pinout
    Pin 1    5 V
    Pin 2    GND
    Pin 3    RP2 TX / ESP32 GPIO16
    Pin 4    RP2 RX / ESP32 GPIO17
```

RC link freshness is checked in firmware. A fresh RC link alone must
never grant permission to move; the safety subsystem must also approve
system mode, IMU state, motor state, battery state, and other required
conditions.

Keep the receiver and antenna away from motor phase wires, high-current
battery wiring, switching regulators, and the CAN transceiver. Add local
supply decoupling near the receiver connector.

### SSD1306 OLED display

```text
Purpose
    Local status and debug UI.

ESP32 interface
    I2C port 0
    SDA on GPIO21
    SCL on GPIO22
    400 kHz
    7-bit address 0x3C

Power
    OLED VCC uses 3.3 V
    OLED GND and ESP32 GND must be common
```

The selected bring-up configuration is a 128 x 64 SSD1306 module. Power
the module from 3.3 V so any onboard I2C pull-ups also pull to 3.3 V.
Verify the actual module address; some variants use `0x3D`.

```text
Suggested connector pinout
    Pin 1    3.3 V
    Pin 2    GND
    Pin 3    SCL / GPIO22
    Pin 4    SDA / GPIO21
```

Use one effective set of I2C pull-ups. Start with 4.7 kohm to 3.3 V if
the module has none. Keep SDA and SCL short and route them away from
motor and battery conductors.

### Battery sensing

```text
Purpose
    Measure battery voltage and optionally current.

Expected hardware
    Resistor divider for voltage
    ADC input protection
    Low-pass filter if needed
    Current sensor or spare ADC input if needed
```

Document these when assigned:

```text
Battery voltage divider values
Maximum pack voltage
ADC reference voltage
ADC input pin
Current sensor type
Current sensor scaling
```

### E-stop and safety inputs

```text
Purpose
    Provide hardware-level and firmware-visible safety state.

Expected behavior
    Hardware power safety must not depend only on firmware.
    Firmware may read an E-stop sense input for state reporting and command blocking.
```

Document these when assigned:

```text
E-stop sense pin
Active level
Pull-up or pull-down strategy
Hardware power cut path
Firmware reaction
```

## Electrical documentation rules

```text
Keep firmware pin maps synchronized with board headers.
Document voltage levels for every external connector.
Document active levels for control and interrupt signals.
Document termination and protection components for bus interfaces.
Avoid routing high motor current through logic or IMU ground paths.
Keep sensor supply and ground clean where practical.
```

## STM32 migration note

The STM32H723ZG OLED I2C instance, CRSF UART instance, pins, DMA
channels, connector assignments, and power rails are not assigned yet.
Do not copy ESP32 GPIO numbers or edit generated CubeMX files by hand.
Assign the peripherals in CubeMX, regenerate generated code, and add
STM32 BSP implementations behind the existing portable protocol,
driver, and snapshot boundaries.
