# ESP32 UART2 dump incoming data in hex

from machine import UART

uart = UART(2, tx=17, rx=16)
uart.init(115200, bits=8, parity=None, stop=1)

while True:
    data = uart.read(1)

    if data is not None:
        # Show the byte as 2 hex digits then in the default way
        print("%02x " % (data[0]), end='')
        print(data)
