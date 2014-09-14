#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

// PIN 2 (interrupt 0) used for FONA_RI
#define FONA_RX 3
#define FONA_TX 4
#define FONA_RST 5

// this is a large buffer for replies
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(&fonaSS, FONA_RST);

void setup() {
  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  
  // See if the FONA is responding
  if (! fona.begin(4800)) {  // make it slow so its easy to read!
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  Serial.println(F("FONA is OK"));
  
  if(fona.callerIdNotification(true))
    Serial.println(F("Caller id notification enabled."));
  else
    Serial.println(F("Caller id notification disabled"));
}

void loop(){
  char phone[32];
  if(fona.incomingCallNumber(phone)){
    Serial.println(F("RING!"));
    Serial.print(F("Phone Number: "));
    Serial.println(phone);
  }
}
