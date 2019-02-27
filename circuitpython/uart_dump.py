# Metro M4 USB show data coming in from UART RX

import board
import busio

uart = busio.UART(board.TX, board.RX, baudrate=115200)

while True:
    data = uart.read(1)

    if data is not None:
        # Show the byte as 2 hex digits then in the default way
        print("%02x " % (data[0]), end='')
        print(data)
