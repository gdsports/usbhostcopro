# USB Host Co-Processor

![Metro M4 Express and USB MIDI controller](images/cpmidiled.jpg)

The USB Host co-processor connects USB devices such as USB keyboards to
devolpment boards without USB host ports or without USB host software. The
connection is made via UART.  Device specific firmware is programmed into the
USB Host co-processor by dragging and dropping a firmware file.

The USB Host co-processor handles the complexities of USB host protocol and
communicates the raw data via UART. For example, the USB MIDI firmware sends
and receives MIDI messages at 115,200 bits/sec via UART. It is bi-directional
and can handle System Exclusive messages up to 1024 bytes.

This open source project is based on the [USB Host Library for
SAMD](https://github.com/gdsports/USB_Host_Library_SAMD) which is a port of the
[USB Host Shield Library 2.0](https://github.com/felis/USB_Host_Shield_2.0).

The hardware is an Adafruit Trinket M0 with a USB OTG to host cable or adapter.
The Trinket M0 has a UF2 bootloader which means it appears as a USB drive.
Firmware is burned into the device by dragging and dropping a firmware file
on to the USB drive. There is no need to install USB serial drivers, IDEs, or
source code unless you want to change the source code.

See the [firmware](./firmware) directory for the latest releases.

Using a UART interface makes it much easier to develop software since all of
the USB complexities are hidden in the co-processor. For example, the following
CircuitPython code shows data arriving on the UART from the USB host
co-processor. It is no more complicated than receiving data from any other
serial device.

```
import board
import busio

uart = busio.UART(board.TX, board.RX, baudrate=115200)

while True:
    data = uart.read(1)

    if data is not None:
        # Show the byte as 2 hex digits then in the default way
		print("%02x " % (data[0]), end='')
        print(data)
```

## USB Host MIDI to UART

MIDIUARTUSBH is a bi-directional converter for USB host MIDI and UART MIDI. The
UART speed is 115,200 bits/sec. Modify the code if MIDI standard 31,250
bits/sec is required. Not all MIDI gear is MIDI class compliant. Also gear with
internal USB hubs is not supported.

This is an example of [Controlling the RGB colors of a NeoPixel using a MIDI
controller](https://github.com/gdsports/circuitpython_usb_host_midi).

## USB Host keyboard to ASCII UART

KBDUARTUSBH when used with a USB keyboard outputs ASCII on the UART. UART input
is ignored. Many keys on a USB keyboard do not have an ASCII code so nothing
will be produced. For example, F1-F12 keys and any combination using the ALT or Win
Logo key do not produce any output.

Some other USB devices emulate USB keyboards such as barcode and RFID readers.
Some barcode readers default to USB keyboard mode but others use USB serial or
USB Point of Sale. Some readers can be configured so consult the reader manual.

USB RFID readers (at least, the cheap ones) can only read the card unique
serial number.

