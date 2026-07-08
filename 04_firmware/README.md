Supported firmware targets:

1. ESP32 NodeMCU prototype
   Location:
     platforms/esp32
   Build:
     pio run -d platforms/esp32
   Upload:
     pio run -d platforms/esp32 -t upload

2. STM32H723 version
   Location:
     platforms/stm32
   Build:
     cmake --preset stm32-debug