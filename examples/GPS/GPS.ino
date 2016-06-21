 /**
 *  ____  _____  _  _    __      ___   ___     _     ___   ___   ___     ___  ____  ___ 
 * ( ___)(  _  )( \( )  /__\    (__ ) / __)   ( )   ( _ ) / _ \ ( _ )   / __)(  _ \/ __)
 *  )__)  )(_)(  )  (  /(__)\    (_ \( (_-.   /_\/  / _ \( (_) )/ _ \  ( (_-. )___/\__ \ 
 * (__)  (_____)(_)\_)(__)(__)  (___/ \___/  (__/\  \___/ \___/ \___/   \___/(__)  (___/
 * This example is meant to work with the Adafruit
 * FONA 808 or 3G Shield or Breakout
 *
 * Copyright: 2016 Adafruit
 * Author: Todd Treece
 * Licence: MIT
 *
 */
#include "Adafruit_FONA.h"

// standard pins for the shield, adjust as necessary
#define FONA_RX 2   // Pin 2 goes to RX on the FONA
#define FONA_TX 3   // Pin 3 goes to TX on the FONA
#define FONA_RST 4  // Pin 4 goes to RST on the FONA

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

// Have a FONA 3G? use this object type
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

// We default to using the newer FONA 3g module, got the older 808 module? use this object instead.
// Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

void setup() {

  while (! Serial);

  Serial.begin(115200);
  Serial.println(F("Adafruit FONA 808 & 3G GPS demo"));
  Serial.println(F("Initializing FONA... (May take a few seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));
  // Try to enable GPRS
  

  Serial.println(F("Enabling GPS..."));
  fona.enableGPS(true);
}

void loop() {
  delay(5000);

  float latitude, longitude, speed_kph, heading, speed_mph, altitude;

  // if you ask for an altitude reading, getGPS will return false if there isn't a 3D fix
  boolean gps_success = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);

  if (gps_success) {

    Serial.print("GPS lat:");
    Serial.println(latitude, 6);
    Serial.print("GPS long:");
    Serial.println(longitude, 6);
    Serial.print("GPS speed KPH:");
    Serial.println(speed_kph);
    Serial.print("GPS speed MPH:");
    speed_mph = speed_kph * 0.621371192;
    Serial.println(speed_mph);
    Serial.print("GPS heading:");
    Serial.println(heading);
    Serial.print("GPS altitude:");
    Serial.println(altitude);

  } else {
    Serial.println("Waiting for FONA GPS 3D fix...");
  }

  // Fona 3G doesnt have GPRSlocation :/
  if ((fona.type() == FONA3G_A) || (fona.type() == FONA3G_E))
    return;
  // Check for network, then GPRS 
  Serial.println(F("Checking for Cell network..."));
  if (fona.getNetworkStatus() == 1) {
    // network & GPRS? Great! Print out the GSM location to compare
    boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);

    if (gsmloc_success) {
      Serial.print("GSMLoc lat:");
      Serial.println(latitude, 6);
      Serial.print("GSMLoc long:");
      Serial.println(longitude, 6);
    } else {
      Serial.println("GSM location failed...");
      Serial.println(F("Disabling GPRS"));
      fona.enableGPRS(false);
      Serial.println(F("Enabling GPRS"));
      if (!fona.enableGPRS(true)) {
        Serial.println(F("Failed to turn GPRS on"));  
      }
    }
  }
}

