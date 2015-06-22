#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"

/*************************** FONA Pins ***********************************/

#define FONA_RX A2
#define FONA_TX A1
#define FONA_RST A0
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

/************************* WiFi Access Point *********************************/

// Optionally configure a GPRS APN, username, and password.
// You might need to do this to access your network's GPRS/data
// network.  Contact your provider for the exact APN, username,
// and password values.  Username and password are optional and
// can be removed, but APN is required.
#define FONA_APN       ""
#define FONA_USERNAME  ""
#define FONA_PASSWORD  ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "your_username"
#define AIO_KEY         "xxxxxxxxxxxxx";

/************ Global State (you don't need to change this!) ******************/

// Store the MQTT server, client ID, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_CLIENTID[] PROGMEM  = AIO_KEY;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, MQTT_SERVER, AIO_SERVERPORT, MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
boolean FONAconnect(void);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>/csv
const char GSMLOC_FEED[] PROGMEM = AIO_USERNAME "/feeds/gsmloc/csv";
Adafruit_MQTT_Publish gsmloc = Adafruit_MQTT_Publish(&mqtt, GSMLOC_FEED);
const char GPSLOC_FEED[] PROGMEM = AIO_USERNAME "/feeds/gps/csv";
Adafruit_MQTT_Publish gpsloc = Adafruit_MQTT_Publish(&mqtt, GPSLOC_FEED);

char gpsbuffer[120];

/*************************** Sketch Code ************************************/

// How many transmission failures in a row we're willing to be ok with before reset
uint8_t txfailures = 0;
#define MAXTXFAILURES 3

void setup() {

  while (!Serial);

  Serial.begin(115200);

  Serial.println(F("Adafruit FONA GPS MQTT demo"));

  // Initialise the FONA module
  while (! FONAconnect()) {
    halt("Retrying FONA");
  }

  Serial.println(F("Connected to Cellular!"));

  fona.enableGPS(true);

  Watchdog.reset();
  delay(1000);  // wait a few seconds to stabilize connection
  Watchdog.reset();

}

uint32_t x=0;

void loop() {

  char sendbuffer[120];

  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  check_connection();
  Watchdog.reset();

  float latitude, longitude, speed_kph, heading, speed_mph, altitude;

  // if you ask for an altitude reading, getGPS will return false if there isn't a 3D fix
  boolean gps_success = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if (gps_success) {

    Serial.print("GPS lat:");
    Serial.println(latitude);
    Serial.print("GPS long:");
    Serial.println(longitude);
    Serial.print("GPS speed KPH:");
    Serial.println(speed_kph);
    Serial.print("GPS speed MPH:");
    speed_mph = speed_kph * 0.621371192;
    Serial.println(speed_mph);
    Serial.print("GPS heading:");
    Serial.println(heading);
    Serial.print("GPS altitude:");
    Serial.println(altitude);

    // snprintf(sendbuffer, 120, "%d,%f,%f,0", x, latitude, longitude);
    // but that doesnt work in arduino
    char *p = sendbuffer;
    // add speed value
    dtostrf(speed_mph, 2, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat latitude
    dtostrf(latitude, 2, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat longitude
    dtostrf(longitude, 3, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat altitude
    dtostrf(altitude, 2, 6, p);
    p += strlen(p);

    // null terminate
    p[0] = 0;

    Serial.print("Sending: "); Serial.println(sendbuffer);

    if (! gpsloc.publish(sendbuffer)) {
      Serial.println(F("Failed"));
      txfailures++;
    } else {
      Serial.println(F("OK!"));
      txfailures = 0;
    }

    Watchdog.reset();

  }

  boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);

  if (gsmloc_success) {

    Serial.print("GSMLoc lat:");
    Serial.println(latitude);
    Serial.print("GSMLoc long:");
    Serial.println(longitude);

    // snprintf(sendbuffer, 120, "%d,%f,%f,0", x, latitude, longitude);
    // but that doesnt work in arduino
    char *p;

    // paste in 'value' first, just an incrementer for GSMLoc
    itoa(x, sendbuffer, 10);
    p = sendbuffer+strlen(sendbuffer);
    p[0] = ','; p++;

    // concat latitude
    dtostrf(latitude, 2, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat longitude
    dtostrf(longitude, 3, 6, p);
    p += strlen(p);
    p[0] = ','; p++;
    p[0] = '0'; p++;  // 0 altitude
    // null terminate
    p[0] = 0;

    Serial.print("Sending: "); Serial.println(sendbuffer);
    if (! gsmloc.publish(sendbuffer)) {
      Serial.println(F("Failed"));
      txfailures++;
    } else {
      Serial.println(F("OK!"));
      txfailures = 0;
    }

  }

  x++;

  // wait a couple seconds before starting over
  Watchdog.reset();
  delay(2000);
  Watchdog.reset();

}

void check_connection() {

  // check if we're still connected
  // and make sure there aren't a bunch
  // of send failures
  if(fona.TCPconnected() && txfailures < MAXTXFAILURES)
    return;

  Serial.println(F("Connecting to MQTT..."));

  int8_t ret, retries = 5;

  while (retries && (ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    Serial.println(F("Retrying MQTT connection"));
    retries--;
    if (retries == 0) halt("Resetting system");
    delay(5000);

  }

  Serial.println(F("MQTT Connected!"));
  txfailures = 0;

}

