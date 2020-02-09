# Microchip PIC Library

## Modules

- [**BM78**](modules/bm78.c): Bluetooth module
  (https://www.microchip.com/wwwproducts/en/BM78).
- [**DHT11**](modules/dht11.c): Temperature & Humidity sensor
  (https://learn.adafruit.com/dht).
- [**I2C**](modules/i2c.c): Registry read/write library.
- [**LCD**](modules/lcd.c): Character display over I2C interface.
- [**MCP22xx**](modules/mcp22xx.c): MCP2200/MCP2221 USB Bridge Module
  (https://www.microchip.com/wwwproducts/en/en546923),
  (https://www.microchip.com/wwwproducts/en/MCP2221).
- [**MCP23017**](modules/mcp23017.c): 16-Bit I2C I/O expander with serial
  interface (https://www.microchip.com/wwwproducts/en/MCP23017).
- [**RGB**](modules/rgb.c): Strip module with lighting patterns.
- [**State Machine**](modules/state_machine.c): Interpreter.
- [**UART**](modules/uart.c): Wrapper over different PIC implementations
  (UART/EUSART).
- [**WS281x**](modules/ws281x.c): Module for controlling distinct LEDs on a
  single strip.
- [**WS281x Light**](modules/ws281x_light.c): A special WS281x Strip setup to
  function as a [enhanced light](https://blog.kubovy.eu/2018/02/11/status-light-with-raspberry-pi-zero-and-w2812-led-strip/).

## Components

- [**BM78 EEPROM**](components/bm78_eeprom.c): EEPROM compontent for BM78
  module.
- [**BM78 Pairing**](components/bm78_pairing.c): Pairing component for BM78
  module.
- [**POC**](components/poc.c): Proof of concept testing and example component.
- [**Serial Communication**](components/serial_communication.c): 
  [Serial protocol](SerialCommunication.md) to control different peripherals.
- [**Setup Mode**](components/setup_mode.c): Simple setup mode for embedded
  devices. 
- [**State Machine Interaction**](components/state_machine_interaction.c): 
  Enables interaction between [State Machine module](module/state_machine.c) and
  other modules.
- [**State Machine Transfer**](components/state_machine_transfer.c): Component
  for transferring State Machine between devices over
  [serial interface](SerialCommunication.md).

