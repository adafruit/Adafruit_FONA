/*
 * Author: S M G
 * Date: 03/08/2016
 * Description:
 * Roughly What we need to do in this Sketch
 * Setup Command serial interface through to FONA 3G device
 * Initiate Data Call
 * Connect to Network
 * Start HTTPS Stack
 * HTTPS Send Request
 * Read Recieved Data
 */

 /*
This CODE is still in Progress, please be patient!

Open up the serial console on the Arduino at 115200 baud to interact with FONA

Note that if you need to set a GPRS APN, username, and password scroll down to
the commented section below at the end of the setup() function.

Suggested Code Sequence from Adafruit for opening a http connection.

AT+CGDCONT=1,\"IP\",\"<YOUR APN HERE>\",\"0.0.0.0\" // Define PDP context

AT+CGSOCKCONT=1,\"IP\",\"<YOUR APN HERE>\" // Define PCP socket context

AT+NETOPEN // Open packet data network

AT+CHTTPSSTART // Aquire HTTPS stack

AT+CHTTPSOPSE=\"<YOUR SITE HERE>\",443 // Open HTTPS session

AT+HTTPSSEND=<LENGTH OF REQUEST> // Tell the SIMCOM you want to send a message of certain length

GET /url/ HTTP/1.1
Host: <YOUR SITE>
// (Any other list of http parameters you want) 
// Make sure that the length of the request matches what you stated above. 

AT+CHTTPSSEND // Tell the SIMCOM to send the message.

// Wait for this from the module
+CHTTPS: RECV EVENT

AT+CHTTPSRECV=<length> // Check the response. <length> defines how long the printout is. I normally say something like 1000 and just ignore the garbage. 
*/



#include "Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Use this one for FONA 3G
Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

//declare the variable for the FONA type
uint8_t type;

void setup() {

  // Optionally configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
     fona.setGPRSNetworkSettings(F("live.vodafone.com"), F(""), F(""));
  
  // put your setup code here, to run once:

  while (!Serial);

  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.println(F("???")); break;
      
  
  }
  
  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }

  enableGPRS();

   // read the Network time just to verify we are on the network okay
   char buffer[23];
   fona.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
   Serial.print(F("Time = ")); Serial.println(buffer);
       
  enableHTTPS();

  // Check network registration status
  Serial.println(AT+CGREG);

  // Check Packet Domain attach or detach
  Serial.println(AT+CGATT?);

  //Set the PDP Contect Profile
  Serial.println(AT+CGSOCKCONT=1,"IP","live.vodafone.com");

  Serial.println(AT+CGDCONT=1,"IP","live.vodafone.com","0.0.0.0",0,0);
  Serial.println(AT+CGAUTH=1,1,"","");
  Serial.println(AT+CGATT=1 );

  // Activate or deactivate the specified PDP contect(s)
  Serial.println(AT+CGACT=1,1 );
  Serial.println(AT+CSOCKSETPN=1 );

  // Check network registration status
  Serial.println(AT+CGREG);

  // Check Packet Domain attach or detach
  Serial.println(AT+CGATT?);


   getHTMLPage();

}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println(F("We Turned on the HTTPS Stack, maybe..."));

}

void getHTMLPage() {
  Serial.println(AT+CHTTPACT="www.adafruit.com/testwifi/index.html",80);
  //Serial.println(AT+CHTTPACT=DATA,249,http/1.1 );
}

void enableGPRS() {
  // turn GPRS on
  if (!fona.enableGPRS(true))
    Serial.println(F("Failed to turn on the 3G GPRS module"));
  else
    Serial.println(F("We have turned on the GPRS Function for the 3G FONA module"));
}

void disableGPRS() {
  // turn GPRS on
  if (!fona.enableGPRS(false))
    Serial.println(F("Failed to turn off the 3G GPRS module"));
  
}

void enableHTTPS() {
  // turn HTTPS Stack on
  // how do i send serial and AT commands?

  Serial.println(F("Starting the HTTPS Stack!"));
  fona.println("AT+CHTTPSSTART");
  
}



