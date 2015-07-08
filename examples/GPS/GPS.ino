#ifdef __AVR__
  #include <SoftwareSerial.h>
#endif
#include "Adafruit_FONA.h"

/*************************** FONA Pins ***********************************/

#define FONA_RX 3
#define FONA_TX 4
#define FONA_RST 5

#ifdef __AVR__
  SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
#endif

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

void setup() {

  while (! Serial);

  Serial.begin(115200);

  Serial.println(F("Adafruit FONA 808 GPS demo"));

  Serial.println(F("Initializing FONA....(May take 3 seconds)"));

  #if not defined (_VARIANT_ARDUINO_DUE_X_)

    fonaSS.begin(4800); // if you're using software serial

    if (! fona.begin(fonaSS)) {
      Serial.println(F("Couldn't find FONA"));
      return;
    }

  #else

    if (! fona.begin(Serial1)) {
      Serial.println(F("Couldn't find FONA"));
      return;
    }

  #endif

  Serial.println(F("FONA is OK"));

  fona.enableGPS(true);

}

void loop() {

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

  }

  boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);

  if (gsmloc_success) {

    Serial.print("GSMLoc lat:");
    Serial.println(latitude);
    Serial.print("GSMLoc long:");
    Serial.println(longitude);

  }

  delay(2000);

}
