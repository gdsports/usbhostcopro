# Example micropython code tested on SparkFun ESP32 Thing
# Use with a Trinket M0 running the LE3DPUSBH sketch and connected to a
# Logitech Extreme 3D Pro joystick

from machine import UART
import ujson

uart = UART(2, tx=17, rx=16)
uart.init(8*115200, bits=8, parity=None, stop=1)

while True:
    # Read line of JSON from Trinket M0 connected to the joystick
    data = uart.readline()
    if data is not None:
        try:
            # Convert JSON string to Python dictionary
            joy = ujson.loads(data)
            print(joy)
            print(joy['X'])
            # Insert code to change motor or servo direction and speed based
            # on joy['X'], joy['Y'], joy['hat'], etc.
        except:
            print(data)
            print("JSON error")
