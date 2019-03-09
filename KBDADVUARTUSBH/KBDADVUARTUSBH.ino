#include <hidboot.h>

// On SAMD boards where the native USB port is also the serial console, use
// Serial1 for the serial console. This applies to all SAMD boards except for
// Arduino Zero and M0 boards.
#if (USB_VID==0x2341 && defined(ARDUINO_SAMD_ZERO)) || (USB_VID==0x2a03 && defined(ARDUINO_SAM_ZERO))
#define SerialDebug   SERIAL_PORT_MONITOR
#define SerialConsole Serial1
#else
#define SerialDebug   Serial1
#define SerialConsole Serial1
#endif

// ASCII output goes to this console UART port which might be different from
// the debug port.
#define consbegin(...)    SerialConsole.begin(__VA_ARGS__)
#define consprint(...)    SerialConsole.print(__VA_ARGS__)
#define conswrite(...)    SerialConsole.write(__VA_ARGS__)
#define consprintln(...)  SerialConsole.println(__VA_ARGS__)

#define DEBUG_ON  0
#if DEBUG_ON
#define dbbegin(...)      SerialDebug.begin(__VA_ARGS__)
#define dbprint(...)      SerialDebug.print(__VA_ARGS__)
#define dbprintln(...)    SerialDebug.println(__VA_ARGS__)
#else
#define dbbegin(...)
#define dbprint(...)
#define dbprintln(...)
#endif

#ifdef ADAFRUIT_TRINKET_M0
// setup Dotstar LED on Trinket M0
#include <Adafruit_DotStar.h>
#define DATAPIN    7
#define CLOCKPIN   8
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
#endif

const uint8_t STX =0x02;
const uint8_t ETX =0x03;

class KeyboardRaw : public KeyboardReportParser {
public:
  KeyboardRaw(USBHost &usb) : hostKeyboard(&usb) {
    hostKeyboard.SetReportParser(0, this);
  };

  void Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf);

private:
  HIDBoot<HID_PROTOCOL_KEYBOARD> hostKeyboard;
};

void KeyboardRaw::Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf)
{
#ifdef DEBUG_KEYBOARD_RAW
  dbprint(F("KeyboardRaw::Parse"));
  // Show USB HID keyboard report
  for (uint8_t i = 0; i < len ; i++) {
    dbprint(' '); dbprint(buf[i], HEX);
  }
  dbprintln();
#endif

  // Call parent/super method
  KeyboardReportParser::Parse(hid, is_rpt_id, len, buf);

  // On error - return
  if (buf[2] == 1)
    return;

  if (len == 8) {
    // <STX> <TypeLen> <8 byte HID report> <ETX>
    uint8_t message[11];
    message[0] = STX;
    message[1] = 0x08;
    memcpy(&message[2], buf, 8);
    message[10] = ETX;
    conswrite(message, sizeof(message));
  }
}

// Initialize USB Controller
USBHost usb;

// Attach keyboard controller to USB
KeyboardRaw keyboard(usb);

void setup()
{
  // Turn off built-in RED LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#ifdef ADAFRUIT_TRINKET_M0
  // Turn off built-in Dotstar RGB LED
  strip.begin();
  strip.clear();
  strip.show();
#endif

  dbbegin( 115200 );
  dbprintln("Start");
  consbegin( 115200 );

  if (usb.Init())
    dbprintln("USB host did not start.");

  delay( 200 );
}

void loop()
{
  usb.Task();
}
