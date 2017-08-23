/***************************************************
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

/*
THIS CODE IS STILL IN PROGRESS!

Open up the serial console on the Arduino at 115200 baud to interact with FONA

Note that if you need to set a GPRS APN, username, and password scroll down to
the commented section below at the end of the setup() function.
*/
#include "Adafruit_FONA.h"

//The atmega is wired as follows:
#define TMP 0
#define PS_LIGHT 5
#define FONA_KEY 6
#define FONA_PS 7
#define FONA_NS 8
#define FONA_TX 9
#define FONA_RX 10
#define FONA_RST 11
#define FONA_VIO 12

//TMP is the TMP36
//PS_LIGHT Is the power LED. This is used to indicate when power is on, a connection fails, or a message fails to send.

//#define FONA_RX 2
//#define FONA_TX 3
//#define FONA_RST 4

bool FonaWake() {
   digitalWrite(FONA_VIO, HIGH);
   if (digitalRead(FONA_PS) == LOW)
   {
      digitalWrite(FONA_KEY, LOW);
      delay(4000);
   }

   digitalWrite(FONA_KEY, HIGH);

   while (!Serial);
   Serial.begin(115200);
   Serial.println(F("Booting up FONA"));

   fonaSerial->begin(4800);

   if(! fona.begin(*fonaSerial)) {
      failBlink(200);

      return false;
   }

   Serial.println(F("FONA is OK"));

   if (digitalRead(FONA_NS) == HIGH) {
      return true;
   }

   // Give it a second to finish booting
   delay(500);

   return digitalRead(FONA_NS) == HIGH;
}

void FonaSleep() {
   if (digitalRead(FONA_PS) == HIGH)
   {
      digitalWrite(FONA_KEY, LOW);
      delay(4000);
      digitalWrite(FONA_KEY, HIGH);
   }

   digitalWrite(FONA_VIO, LOW);

   if (digitalRead(FONA_PS == LOW)) {
      Serial.println("FONA OFF");
   }
}

bool sendSms(char *phoneNumber, char *message) {
   // send an SMS!
   flushSerial();

   if (!fona.sendSMS(phoneNumber, message)) {
      failBlink(500);
      return false;
   }

   return true;
}
