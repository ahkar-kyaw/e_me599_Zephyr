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

## CAN motor bus

```text
Purpose
    Communicate with four CubeMars AK40-10 actuators.

ESP32 interface
    ESP32 TWAI
    GPIO26 is CAN_TX
    GPIO32 is CAN_RX
    1 Mbit/s classic CAN
    29-bit extended frames

Transceiver
    Adafruit CAN Pal with TJA1051T/3
    Vcc uses 3.3 V to match ESP32 logic
    GND is common with ESP32 and actuator CAN ground
    ESP32 GPIO26 connects to CAN Pal TX
    ESP32 GPIO32 connects to CAN Pal RX
    SLNT is held low for normal operation
```

The CAN Pal has an onboard charge pump for the transceiver bus-side
supply and a switchable 120 ohm terminator. It is not an isolated CAN
interface.

```text
Bus connector
    L                   CANL
    Middle terminal     CAN ground
    H                   CANH

Topology
    One linear trunk
    Short drops to four actuators
    Exactly two 120 ohm endpoint terminations
    No star wiring
```

Enable the CAN Pal terminator only if the ESP32/CAN Pal is one physical
end of the trunk. Confirm whether the actuator or harness at the far
end provides termination. Intermediate actuator nodes must not add
termination.

The AK40-10 product specification identifies an A1257WR-S-4P CAN
connector. The exact harness pin order has not yet been verified in
this repository. Do not infer the pin order from wire colors. Verify it
against the exact CubeMars driver installation instructions and cable,
then record CANH, CANL, ground, and any auxiliary conductor here.

The actuator power bus is separate from logic power. The AK40-10 is a
24 V actuator rated at 2.7 A with a listed peak current of 7.3 A. Four
actuators therefore require a deliberately designed power tree. Before
loaded tests, document:

```text
Battery chemistry and maximum voltage
Main fuse and branch protection
Contactor or hardware E-stop power cut
Precharge or inrush behavior
Wire gauge and connector current ratings
Grounding and return-current plan
Regenerative-energy handling
```

Keep battery and motor-current paths physically separated from the CAN
pair, ESP32 logic wiring, OLED harness, receiver, and IMU supply and
signals. Cross noisy conductors at right angles where separation cannot
be maintained.

## Future electrical interfaces

The following sections should be completed as the electrical design matures.

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

### Waveshare 1.5-inch RGB OLED display

```text
Purpose
    Local status, diagnostics, and future tuning UI.

ESP32 interface
    SSD1351 controller
    Four-wire SPI
    SPI3 host
    CLK on GPIO14
    DIN/MOSI on GPIO13
    CS on GPIO25, active low
    D/C on GPIO21, command low and data high
    RST on GPIO22, active low
    8 MHz
    No MISO connection

Power
    Module VCC uses 3.3 V
    Module logic therefore uses 3.3 V
    OLED and ESP32 grounds must be common
```

The selected module is the Waveshare 1.5-inch RGB OLED Module with a
128 x 128 SSD1351 controller. Leave its interface selection in the
factory-default four-wire SPI position. Do not configure the module for
three-wire SPI.

```text
Waveshare seven-pin connector
    Pin 1    3.3 V
    Pin 2    GND
    Pin 3    DIN / GPIO13
    Pin 4    CLK / GPIO14
    Pin 5    CS / GPIO25
    Pin 6    D/C / GPIO21
    Pin 7    RST / GPIO22
```

The Waveshare documentation lists approximately 60 mA at 3.3 V for a
full-white screen and approximately 4 mA for full black. Size the 3.3 V
rail for the worst case and place local decoupling near the display
connector. The default UI uses a black background and reduced master
contrast to limit current and OLED aging.

Keep the SPI harness short and route it away from motor phase wires,
high-current battery wiring, switching nodes, and CAN transceiver
signals. Do not route display return current through the IMU ground
path.

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

The STM32H723ZG display SPI instance, CRSF UART instance, pins, DMA
channels, connector assignments, and power rails are not assigned yet.
Do not copy ESP32 GPIO numbers or edit generated CubeMX files by hand.
Assign the peripherals in CubeMX, regenerate generated code, and add
STM32 BSP implementations behind the existing portable protocol,
driver, and snapshot boundaries.
