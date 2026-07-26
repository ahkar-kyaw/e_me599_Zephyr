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

### RC receiver

```text
Purpose
    Receive user commands through CRSF.

Expected interface
    UART RX from receiver TX
    Optional UART TX to receiver RX
    3.3 V logic level unless verified otherwise
```

Document these when assigned:

```text
MCU UART instance
MCU RX pin
MCU TX pin
Receiver supply voltage
Connector pinout
Failsafe behavior
```

### OLED display

```text
Purpose
    Local status and debug UI.

Expected interface
    I2C SDA
    I2C SCL
    3.3 V supply
    Pull-up resistors on SDA and SCL
```

Document these when assigned:

```text
MCU I2C instance
SDA pin
SCL pin
I2C address
Pull-up values
Connector pinout
```

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
