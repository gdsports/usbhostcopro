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
#define consflush(...)    SerialConsole.flush(__VA_ARGS__)
#define consprintln(...)  SerialConsole.println(__VA_ARGS__)

#define DEBUG_ON  0
#if DEBUG_ON
#define dbbegin(...)      SerialDebug.begin(__VA_ARGS__)
#define dbprint(...)      SerialDebug.print(__VA_ARGS__)
#define dbflush(...)      SerialDebug.flush(__VA_ARGS__)
#define dbprintln(...)    SerialDebug.println(__VA_ARGS__)
#else
#define dbbegin(...)
#define dbprint(...)
#define dbflush(...)
#define dbprintln(...)
#endif

#ifdef ADAFRUIT_TRINKET_M0
// setup Dotstar LED on Trinket M0
#include <Adafruit_DotStar.h>
#define DATAPIN    7
#define CLOCKPIN   8
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
#endif

const uint8_t STX = 0x02;
const uint8_t ETX = 0x03;
enum msgType_t {KEYBOARD, MOUSE, JOYSTICK, MIDI};

class MouseRaw : public MouseReportParser {
  public:
    MouseRaw(USBHost &usb) : hostMouse(&usb) {
      hostMouse.SetReportParser(0, this);
    };

    void Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf);

  private:
    HIDBoot<HID_PROTOCOL_MOUSE> hostMouse;
};

//#define DEBUG_MOUSE_RAW 1

void MouseRaw::Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf)
{
#ifdef DEBUG_MOUSE_RAW
  dbprint(F("MouseRaw::Parse"));
  // Show USB HID mouse report
  for (uint8_t i = 0; i < len ; i++) {
    dbprint(' '); dbprint(buf[i], HEX);
  }
  dbprintln();
#endif

  // Call parent/super method
  MouseReportParser::Parse(hid, is_rpt_id, len, buf);

  // Ignore duplicate reports
  static uint32_t old_len;
  static uint8_t old_buf[5];
  if ((len == old_len) && (memcmp(buf, old_buf, len) == 0)) {
    dbprintln(F("dup"));
    return;
  }
  dbprint('.');
  dbflush();
  old_len = len;
  if (old_len > sizeof(old_buf)) old_len = sizeof(old_buf);
  memcpy(old_buf, buf, old_len);

  if (len > 2) {
    // <STX> <TypeLen> <4 byte HID report> <ETX>
    //   0       1      2,3,4,5              6
    uint8_t message[7];
    memset(message, 0, sizeof(message));
    message[0] = STX;
    message[1] = (MOUSE << 5) | 4;
    memcpy(&message[2], buf, len);
    message[6] = ETX;
    conswrite(message, sizeof(message));
    consflush();
#ifdef DEBUG_MOUSE_RAW
    dbprint(F("MouseRaw::Parse UART"));
    // Show USB HID output over UART
    for (uint8_t i = 0; i < sizeof(message) ; i++) {
      dbprint(' '); dbprint(message[i], HEX);
    }
    dbprintln();
#endif
  }
}

// Initialize USB Controller
USBHost usb;

// Attach mouse controller to USB
MouseRaw mouse(usb);

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

  dbbegin( 2000000 );
  dbprintln("Start");
  consbegin( 8*115200 );

  if (usb.Init())
    dbprintln("USB host did not start.");

  delay( 200 );
}

void loop()
{
  usb.Task();
  delay(10);
}
