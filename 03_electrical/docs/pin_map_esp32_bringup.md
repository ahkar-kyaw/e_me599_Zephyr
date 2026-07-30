# ESP32 bring-up pin map

## ISM330DHCX IMU

```text
Signal      ESP32 GPIO
SPI SCLK    GPIO18
SPI MISO    GPIO19
SPI MOSI    GPIO23
SPI CS      GPIO27
IMU INT1    GPIO34
IMU INT2    GPIO35
```

INT1 and INT2 are not used by the polling firmware yet.

## Waveshare 1.5-inch RGB OLED

```text
OLED signal     ESP32 connection      Notes
VCC             3.3 V                 Match ESP32 logic level
GND             GND                   Common ground
DIN             GPIO13                SPI3 MOSI
CLK             GPIO14                SPI3 SCLK
CS              GPIO25                Active low
D/C             GPIO21                Command low, data high
RST             GPIO22                Active low
```

The module uses an SSD1351 controller and the factory-default four-wire
SPI interface. Firmware uses SPI3 at 8 MHz with RGB565 pixel data. The
interface is write-only; no MISO connection is required.

Use the complete seven-pin Waveshare harness and keep it short. Place
local supply decoupling near the display connector. Keep the harness
away from motor phase wires, battery conductors, switching nodes, and
the IMU signal/ground path.

## RadioMaster RP2 ExpressLRS receiver

```text
RP2 signal      ESP32 connection
TX              GPIO16 (UART2 RX)
RX              GPIO17 (UART2 TX)
5V              Regulated 5 V
GND             GND
```

The firmware uses CRSF at 420000 baud, 8 data bits, no parity, and one
stop bit. CRSF is non-inverted and full-duplex. The current firmware
receives RC channel frames but does not transmit telemetry yet.

Do not power the RP2 from the ESP32 3.3 V pin. Confirm that the selected
5 V source remains regulated and adequately decoupled when motors are
active.
