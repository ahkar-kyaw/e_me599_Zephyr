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

## SSD1306 OLED

```text
OLED signal     ESP32 connection
SDA             GPIO21
SCL             GPIO22
VCC             3.3 V
GND             GND
```

The firmware uses I2C port 0 at 400 kHz and 7-bit address `0x3C`.
Use pull-ups to 3.3 V on SDA and SCL. Many OLED modules already include
pull-ups; check the module before adding another pair.

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
