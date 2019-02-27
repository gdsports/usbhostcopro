# USB keyboard pass through.
#
# KBDADVUARTUSBH sends the USB keyboard HID report like this
# <STX> <Type|Len> <8 byte HID report> <ETX>
# The first byte is 0x02 <STX>, the last byte is 0x03 <ETX>.
# The second byte is always 0x08 because the keyboard HID report
# is always 8 bytes long.
# The 8 byte HID report is sent out the HID keyboard device.
#

import board
import busio
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode

STX=0x02
ETX=0x03

uart = busio.UART(board.TX, board.RX, baudrate=115200)

keyboard = Keyboard()
keyboard_layout = KeyboardLayoutUS(keyboard)

while True:
    data = uart.read(11)

    if data is not None:
        if (len(data) == 11) and (data[0] == STX) and (data[1] == 0x08) and (data[10] == ETX):
            keyboard.hid_keyboard.send_report(data[2:10])
        else:
            # Scan for STX ... ETX to resync
            print(data)
            report = bytearray(11)
            for i in range(0, len(data)):
                if data[i] == STX:
                    report = data[i:len(data)] + uart.read(11-(len(data)-i))
                    print(report)
                    if (len(report) == 11) and (report[0] == STX) and (report[1] == 0x08) and (report[10] == ETX):
                        keyboard.hid_keyboard.send_report(report[2:10])
