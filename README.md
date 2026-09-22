# nucleo_G431RB_oled_128x64

Tinkering with an 
**Adafruit FeatherWing OLED - 128x64** 
display attached to an 
STMicro **Nucleo-G431RB** evaluation board.

## Requirements
 - "Nucleo-G431RB" Developement Board from ST Microelectronics
   ([https://www.st.com/stm32](https://www.st.com/stm32)).
 - "Adafruit FeatherWing OLED - 128x64" from Adafruit.com, Product ID 4650 
   ([https://www.adafruit.com/product/4650](https://www.adafruit.com/product/4650)).

## Setup
| Nucleo Pin     | Nucleo Signal | Adafruit #4650 Pin |
| ---------------| ------------- | ------------------- |
| **Power**      |               |                     |
| CN7 Pin 16     | 3V3           | 3V                  |
| CN7 Pin 20     | GND           | GND                 |
| **I2C Port**   |               |                     |
| CN10 Pin 10    | PB8-Boot0     | SCL                 |
| CN10 Pin 5     | PB9           | SDA                 |

### ST-LINK 
 - ST-LINK serial port is set to 115,200 baud, 8 bits, no parity, 1 stop bit (115.2K, 8N1)
   

## License
MIT License; see LICENSE file.