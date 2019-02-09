# Compiled releases for Trinket M0

Compiled programs can be burned into the Trinket M0 just by dragging and
dropping a UF2 file on to the Trinket M0 USB drive. There is no need to install
the Arduino IDE, source code, or USB serial device driver.

* Download the UF2 file of your choice.
* Plug in the Trinket M0 to the computer.
* Double tap the Trinket M0 reset button.
* When the TRINKETBOOT USB drive appears, drop the UF2 file on to the drive.
* Wait until the Trinket M0 reboots.

At this point, the Trinket M0 is now in USB host mode so it no longer talks
with the computer. Unplug the Trinket M0 then plug in a USB OTG to host cable
or adapter then plug in a USB device such as a keyboard. Wire up the Trinket M0
UART Tx/Rx, USB (5V) and ground pins.

Since the Trinket M0 appears as UART device, it can be used with any other
board with a UART. The board could be running Arduino, CircuitPython,
MicroPython, or Espruino. As long as the other board has a UART, it can talk to
the USB device through the Trinket M0.
