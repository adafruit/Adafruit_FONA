#include <ArduinoUnitTests.h>
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ci/DeviceUsingBytes.h>
#include "../Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

bool bigEndian = false;  // RS232 says so

// DeviceUsingBytes extends DataStreamObserver,
// so we will be able to attach this class to an
// ObservableDataStream object, of which the pin
// history (soft-serial) and HardwareSerial
// objects are.
class FakeFona : public DeviceUsingBytes {
  public:
    String mLast;

    FakeFona() : DeviceUsingBytes() {
      mLast = "";
      addResponseCRLF("AT", "OK");
      addResponseCRLF("ATE0", "OK");
      addResponseCRLF("AT+CVHU=0", "OK");
      addResponseCRLF("ATI", "OK");
      addResponseCRLF("AT+CPMS=\"SM\",\"SM\",\"SM\"", "OK");
      addResponseCRLF("AT+GSN", "1234567890abcde\r\nOK");
    }

    virtual ~FakeFona() {}

    virtual void onMatchInput(String output) {
      mLast = output;
      state->digitalPin[FONA_TX].fromAscii(output, bigEndian);
    }
};


class Adafruit_FONA_debug : public Adafruit_FONA {
  public:
    Adafruit_FONA_debug(int8_t r) : Adafruit_FONA(r) {}


    String debug_replybuffer() const {
      return String(replybuffer);
    }

    uint8_t debug_readline(uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS, boolean multiline = false) { return readline(timeout, multiline); }

};

unittest(ARDUINO_100)
{
  assertEqual(100, ARDUINO);
}

// basic test of the readline capability
unittest(fona_readline) {

  GodmodeState* state = GODMODE();
  state->reset();

  Adafruit_FONA_debug fona = Adafruit_FONA_debug(FONA_RST);
  SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
  SoftwareSerial *fonaSerial = &fonaSS;
  fonaSerial->begin(115200);
  fona.begin(*fonaSerial);

  assertTrue(fonaSS.isListening());

  assertEqual(0, fonaSS.available());
  state->digitalPin[FONA_TX].fromAscii("You were over the line, Smokey\nMark it 8, Dude", bigEndian);
  assertNotEqual(0, fonaSS.available());

  uint8_t rl = fona.debug_readline(300, false);
  assertNotEqual(0, rl);
  assertEqual("You were over the line, Smokey", fona.debug_replybuffer());
}

// retrieve the IMEI from a serial device faked by the FakeFona class
unittest(fona_imei) {

  GodmodeState* state = GODMODE();
  state->reset();

  // set up FONA on serial port
  Adafruit_FONA_debug fona = Adafruit_FONA_debug(FONA_RST);
  SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
  SoftwareSerial *fonaSerial = &fonaSS;
  fonaSerial->begin(115200);

  // set up our fake modem on the other end.
  // note that the digitalPin we attach to is an ObservableDataStream
  FakeFona fakeFona;
  fakeFona.attach(&state->digitalPin[FONA_RX]);

  // fire it up
  assertTrue(fona.begin(*fonaSerial));

  // These asserts will just dump out the input/output log as a test failure
  // assertEqual("", state->digitalPin[FONA_RX].toAscii(1, bigEndian));
  // assertEqual("", state->digitalPin[FONA_TX].toAscii(1, bigEndian));

  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  assertNotEqual(0, imeiLen);
  //assertEqual("AT+GSN\n", state->digitalPin[FONA_RX].toAscii(1, bigEndian));
  assertEqual("1234567890abcde", imei);
}

unittest_main()
