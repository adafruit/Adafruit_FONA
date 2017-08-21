#
# This code provided by Adafruit Support Rick, it is an example of how to do something upon an event, in this case
# How to do an action upon recieving a phone call (ringing).
#
# In your sketch, attach your own interrupt routine to the RING line. In the routine, set a boolean flag to true.
# Poll the boolean in your loop routine. Whenever it becomes true, do a pickup and then set the flag back to false.
#
# Let's assume you're using pin 2 for your RI line. (if you're on a Uno, you can only use pins 0, 1, 2, 3 for RI.
# But 0 and 1 are already used for the serial monitor). I've moved the other pins as necessary

#include "Adafruit_FONA.h"
#include "SoftwareSerial.h"

#define RING_PIN 2
#define FONA_TX 3
#define FONA_RX 4
#define FONA_RST 5

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

volatile bool ringing = false;
void RingInterrupt()
{
  ringing = true;
}

void setup() {
  Serial.begin(115200);
  
  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }

  attachInterrupt(RING_PIN, RingInterrupt, FALLING);
}

void loop() {

  if (ringing)
  {
    delay(1000);  //let it ring for a second
    // pick up!
    if (! fona.pickUp()) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
    ringing = false;
  }
}
