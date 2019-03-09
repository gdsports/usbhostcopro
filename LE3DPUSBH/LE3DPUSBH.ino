/*
 * Convert Logitech Extreme 3D Pro Joystick input to JSON output on UART port.
 * This designed to run on an Adafruit Trinket M0 but should work on other
 * SAMD21 and SAMD51 boards.
 *
 * X and Y axes: 0..1023
 * twist: 0..255
 * hat: 8 way direction pad
 *      0=North, Forward
 *      1=North East
 *      2=East,Right
 *      3=South East
 *      4=South, Back
 *      5 South West
 *      6=West,Left
 *      7=North West
 *      8=no direction
 *  throttle: 0..255
 *  buttons_a, buttons_b: press the buttons and see the values
 */

/* Example micropython code tested on SparkFun ESP32 Thing

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
            print("json error")
*/

#include <hid.h>
#include <hiduniversal.h>

// On SAMD boards where the native USB port is also the serial console, use
// Serial1 for the serial console. This applies to all SAMD boards except for
// Arduino Zero and M0 boards.
#if (USB_VID==0x2341 && defined(ARDUINO_SAMD_ZERO)) || (USB_VID==0x2a03 && defined(ARDUINO_SAM_ZERO))
#define SerialDebug SERIAL_PORT_MONITOR
#else
#define SerialDebug Serial1
#endif

#define JSON_OUT    Serial1
// JSON output goes to this UART port which might be different from
// the debug port.
#define jsonbegin(...)    JSON_OUT.begin(__VA_ARGS__)
#define jsonprint(...)    JSON_OUT.print(__VA_ARGS__)
#define jsonwrite(...)    JSON_OUT.write(__VA_ARGS__)
#define jsonprintln(...)  JSON_OUT.println(__VA_ARGS__)

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

uint32_t last_millis;

uint32_t elapsed_mSecs(void)
{
  uint32_t now = millis();
  if (now < last_millis) {
    return (now + 1) + (0xFFFFFFFF - last_millis);
  }
  else {
    return now - last_millis;
  }
}

struct LE3DPEventData
{
  union { //axes and hat switch
    uint32_t axes;
    struct {
      uint32_t x : 10;
      uint32_t y : 10;
      uint32_t hat : 4;
      uint32_t twist : 8;
    };
  };
  uint8_t buttons_a;
  uint8_t slider;
  uint8_t buttons_b;
};

// Thrustmaster T.16000M
struct T16KEventData
{
  uint16_t	buttons;
  uint8_t		hat;
  uint16_t	x;
  uint16_t	y;
  uint8_t		twist;
  uint8_t		slider;
}__attribute__((packed));

struct GamePadEventData
{
  union {
    struct LE3DPEventData le3dp;
    struct T16KEventData  t16k;
  };
};

class JoystickEvents
{
public:
	virtual void OnGamePadChanged(const GamePadEventData *evt, size_t len);
};

#define RPT_GAMEPAD_LEN	sizeof(GamePadEventData)

class JoystickReportParser : public HIDReportParser
{
	JoystickEvents		*joyEvents;

  uint8_t oldPad[RPT_GAMEPAD_LEN];

public:
	JoystickReportParser(JoystickEvents *evt);

	virtual void Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf);
};

USBHost                                         UsbH;
HIDUniversal                                    Hid(&UsbH);
JoystickEvents                                  JoyEvents;
JoystickReportParser                            Joy(&JoyEvents);

JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
	joyEvents(evt)
{}

void JoystickReportParser::Parse(HID *hid, uint32_t is_rpt_id, uint32_t len, uint8_t *buf)
{
	// Checking if there are changes in report since the method was last called
  bool match = (memcmp(oldPad, buf, len) == 0);

  // Calling Game Pad event handler
	if (!match && joyEvents) {
    //dbprint("Parse len="); dbprint(len); dbprint(':');
    for (int i = 0; i < len; i++) {
      dbprint(buf[i], HEX);
      dbprint(' ');
    }
    dbprintln();
    joyEvents->OnGamePadChanged((const GamePadEventData*)buf, len);
    memcpy(oldPad, buf, len);
	}
}



void JoystickEvents::OnGamePadChanged(const GamePadEventData *genevt, size_t len)
{
  char json[128];

  // TODO Check VID/PID instead of report length
  if (len == 9) {
    // Assume Thrustmaster T.16000M
    const T16KEventData *evt=(const T16KEventData*)genevt;
    int retcode = snprintf(json, sizeof(json),
        "{\"X\":%d,\"Y\":%d,\"twist\":%d,\"hat\":%d,\"throttle\":%d,\"buttons\":%d}",
        evt->x, evt->y, evt->twist, evt->hat, evt->slider,
        evt->buttons);
    if (retcode > 0) jsonprintln(json);
    dbprint(F("X: "));
    dbprint(evt->x);
    dbprint(F(" Y: "));
    dbprint(evt->y);
    dbprint(F(" Hat Switch: "));
    dbprint(evt->hat);
    dbprint(F(" Twist: "));
    dbprint(evt->twist);
    dbprint(F(" Throttle: "));
    dbprint(evt->slider);
    dbprint(F(" Buttons: 0x"));
    dbprint(evt->buttons, HEX);
    dbprintln();
  }
  else {
    // Assume Logitech Extreme 3D Pro
    const LE3DPEventData *evt=(const LE3DPEventData*)genevt;
    int retcode = snprintf(json, sizeof(json),
        "{\"X\":%d,\"Y\":%d,\"twist\":%d,\"hat\":%d,\"throttle\":%d,\"buttons_a\":%d,\"buttons_b\":%d}",
        evt->x, evt->y, evt->twist, evt->hat, evt->slider,
        evt->buttons_a, evt->buttons_b);
    if (retcode > 0) jsonprintln(json);
    dbprint(F("X: "));
    dbprint(evt->x);
    dbprint(F(" Y: "));
    dbprint(evt->y);
    dbprint(F(" Hat Switch: "));
    dbprint(evt->hat);
    dbprint(F(" Twist: "));
    dbprint(evt->twist);
    dbprint(F(" Throttle: "));
    dbprint(evt->slider);
    dbprint(F(" Buttons A: 0x"));
    dbprint(evt->buttons_a, HEX);
    dbprint(F(" Buttons B: 0x"));
    dbprint(evt->buttons_b, HEX);
    dbprintln();
  }
}

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

  jsonbegin( 8*115200 );  // New CPUs can handle 921600
  dbbegin( 115200 );
  dbprintln(F("Start"));

  if (UsbH.Init())
      dbprintln(F("USB host did not start."));

  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
      dbprint(F("SetReportParser failed"));

  last_millis = millis();
}

void loop()
{
  if (elapsed_mSecs() > 10) {
    last_millis = millis();
    UsbH.Task();
  }
}
