/*
  MIT License

  Copyright (c) 2018 gdsports625@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/*
   MIDI UART to MIDI USB host converter for SAMD21 and SAMD51 Arduino and
   Arduino compatible boards.
*/

#include <MIDI.h>       // MIDI Library by Forty Seven Effects
#include <usbh_midi.h>  // https://github.com/gdsports/USB_Host_Library_SAMD
#include <usbhub.h>

// setup Dotstar LED on Trinket M0
#include <Adafruit_DotStar.h>
#define DATAPIN    7
#define CLOCKPIN   8
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

// 1 turns on debug, 0 off
#define DBGSERIAL if (0) SERIAL_PORT_MONITOR

USBHost UsbH;
USBH_MIDI MIDIUSBH(&UsbH);

#define MIDI_SERIAL_PORT Serial1

struct MySettings : public midi::DefaultSettings
{
  static const bool Use1ByteParsing = false;
  static const unsigned SysExMaxSize = 1026; // Accept SysEx messages up to 1024 bytes long.
  static const long BaudRate = 115200;
};

MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, MIDI_SERIAL_PORT, MIDIUART, MySettings);

inline uint8_t writeUARTwait(uint8_t *p, uint16_t size)
{
  // Apparently, not needed. write blocks, if needed
  //  while (MIDI_SERIAL_PORT.availableForWrite() < size) {
  //    delay(1);
  //  }
  return MIDI_SERIAL_PORT.write(p, size);
}

uint16_t sysexSize = 0;

void sysex_end(uint8_t i)
{
  sysexSize += i;
  DBGSERIAL.print(F("sysexSize="));
  DBGSERIAL.println(sysexSize);
  sysexSize = 0;
}


void setup() {
  // Turn off built-in RED LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // Turn off built-in Dotstar RGB LED
  strip.begin();
  strip.clear();
  strip.show();

  DBGSERIAL.begin(115200);

  MIDIUART.begin(MIDI_CHANNEL_OMNI);
  //MIDIUART.turnThruOff();

  if (UsbH.Init()) {
    DBGSERIAL.println(F("USB host failed to start"));
    while (1) delay(1); // halt
  }
}

void USBHost_to_UART()
{
  uint8_t recvBuf[MIDI_EVENT_PACKET_SIZE];
  uint8_t rcode = 0;     //return code
  uint16_t rcvd;
  uint8_t readCount = 0;

  rcode = MIDIUSBH.RecvData( &rcvd, recvBuf);

  //data check
  if (rcode != 0 || rcvd == 0) return;
  if ( recvBuf[0] == 0 && recvBuf[1] == 0 && recvBuf[2] == 0 && recvBuf[3] == 0 ) {
    return;
  }

  uint8_t *p = recvBuf;
  while (readCount < rcvd)  {
    if (*p == 0 && *(p + 1) == 0) break; //data end
    DBGSERIAL.print(F("USB "));
    DBGSERIAL.print(p[0], DEC);
    DBGSERIAL.print(' ');
    DBGSERIAL.print(p[1], DEC);
    DBGSERIAL.print(' ');
    DBGSERIAL.print(p[2], DEC);
    DBGSERIAL.print(' ');
    DBGSERIAL.println(p[3], DEC);
    uint8_t header = *p & 0x0F;
    p++;
    switch (header) {
      case 0x00:  // Misc. Reserved for future extensions.
        break;
      case 0x01:  // Cable events. Reserved for future expansion.
        break;
      case 0x02:  // Two-byte System Common messages
      case 0x0C:  // Program Change
      case 0x0D:  // Channel Pressure
        writeUARTwait(p, 2);
        break;
      case 0x03:  // Three-byte System Common messages
      case 0x08:  // Note-off
      case 0x09:  // Note-on
      case 0x0A:  // Poly-KeyPress
      case 0x0B:  // Control Change
      case 0x0E:  // PitchBend Change
        writeUARTwait(p, 3);
        break;

      case 0x04:  // SysEx starts or continues
        sysexSize += 3;
        writeUARTwait(p, 3);
        break;
      case 0x05:  // Single-byte System Common Message or SysEx ends with the following single byte
        sysex_end(1);
        writeUARTwait(p, 1);
        break;
      case 0x06:  // SysEx ends with the following two bytes
        sysex_end(2);
        writeUARTwait(p, 2);
        break;
      case 0x07:  // SysEx ends with the following three bytes
        sysex_end(3);
        writeUARTwait(p, 3);
        break;
      case 0x0F:  // Single Byte, TuneRequest, Clock, Start, Continue, Stop, etc.
        writeUARTwait(p, 1);
        break;
    }
    p += 3;
    readCount += 4;
  }
}

void UART_to_USBHost()
{
  if (MIDIUART.read()) {
    midi::MidiType msgType = MIDIUART.getType();
    DBGSERIAL.print(F("UART "));
    DBGSERIAL.print(msgType, HEX);
    DBGSERIAL.print(' ');
    DBGSERIAL.print(MIDIUART.getData1(), HEX);
    DBGSERIAL.print(' ');
    DBGSERIAL.println(MIDIUART.getData2(), HEX);
    switch (msgType) {
      case midi::InvalidType:
        break;
      case midi::NoteOff:
      case midi::NoteOn:
      case midi::AfterTouchPoly:
      case midi::ControlChange:
      case midi::ProgramChange:
      case midi::AfterTouchChannel:
      case midi::PitchBend:
        {
          uint8_t tx[4] = {
            (byte)(msgType >> 4),
            (byte)((msgType & 0xF0) | ((MIDIUART.getChannel() - 1) & 0x0F)), /* getChannel() returns values from 1 to 16 */
            MIDIUART.getData1(),
            MIDIUART.getData2()
          };
          MIDIUSBH.SendRawData(sizeof(tx), tx);
          break;
        }
      case midi::SystemExclusive:
        MIDIUSBH.SendSysEx((uint8_t *)MIDIUART.getSysExArray(),
            MIDIUART.getSysExArrayLength(), 0);
        DBGSERIAL.print("sysex size ");
        DBGSERIAL.println(MIDIUART.getSysExArrayLength());
        break;
      case midi::TuneRequest:
      case midi::Clock:
      case midi::Start:
      case midi::Continue:
      case midi::Stop:
      case midi::ActiveSensing:
      case midi::SystemReset:
        {
          uint8_t tx[4] = { 0x0F, (byte)(msgType), 0, 0 };
          MIDIUSBH.SendRawData(sizeof(tx), tx);
          break;
        }
      case midi::TimeCodeQuarterFrame:
      case midi::SongSelect:
        {
          uint8_t tx[4] = { 0x02, (byte)(msgType), MIDIUART.getData1(), 0 };
          MIDIUSBH.SendRawData(sizeof(tx), tx);
          break;
        }
      case midi::SongPosition:
        {
          uint8_t tx[4] = { 0x03, (byte)(msgType), MIDIUART.getData1(), MIDIUART.getData2() };
          MIDIUSBH.SendRawData(sizeof(tx), tx);
          break;
        }
      default:
        break;
    }
  }
}

void loop()
{
  UsbH.Task();

  if (MIDIUSBH) {
    /* MIDI UART -> MIDI USB Host */
    UART_to_USBHost();

    /* MIDI USB Host -> MIDI UART */
    USBHost_to_UART();
  }
}
