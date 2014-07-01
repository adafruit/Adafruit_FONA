/*************************************************** 
  This is a library for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA 
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963

  These displays use TTL Serial to communicate, 2 pins are required to 
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#if (ARDUINO >= 100)
 #include "Arduino.h"
 #include <SoftwareSerial.h>
#else
 #include "WProgram.h"
 #include <NewSoftSerial.h>
#endif


#define FONA_HEADSETAUDIO 0
#define FONA_EXTAUDIO 1

#define FONA_STTONE_DIALTONE 1
#define FONA_STTONE_BUSY 2
#define FONA_STTONE_CONGESTION 3
#define FONA_STTONE_PATHACK 4 
#define FONA_STTONE_DROPPED 5
#define FONA_STTONE_ERROR 6  
#define FONA_STTONE_CALLWAIT 7 
#define FONA_STTONE_RINGING 8
#define FONA_STTONE_BEEP 16
#define FONA_STTONE_POSTONE 17
#define FONA_STTONE_ERRTONE 18
#define FONA_STTONE_INDIANDIALTONE 19
#define FONA_STTONE_USADIALTONE 20

class Adafruit_FONA {
 public:
#if ARDUINO >= 100
  Adafruit_FONA(SoftwareSerial *, int8_t r);
#else
  Adafruit_FONA(NewSoftSerial *, int8_t r);
#endif
  boolean begin(uint16_t baud);

  // Battery and ADC
  boolean getADCVoltage(uint16_t *v);
  boolean getBattVoltage(uint16_t *v);

  // SIM query
  uint8_t getSIMCCID(char *ccid);
  uint8_t getNetworkStatus(void);
  uint8_t getRSSI(void);

  // set Audio output
  boolean setAudio(uint8_t a);
  boolean setVolume(uint8_t i);
  uint8_t getVolume(void);
  boolean playToolkitTone(uint8_t t, uint16_t len);

  // SMS handling
  int8_t getNumSMS(void);
  uint8_t readSMS(uint8_t i, char *smsbuff, uint8_t max);
  boolean sendSMS(char *smsaddr, char *smsmsg);
  boolean deleteSMS(uint8_t i);

 private: 
  int8_t _rstpin;

  char replybuffer[255];

  uint8_t readline(uint16_t timeout = 500, boolean multiline = false);
  uint8_t getReply(char *send, uint16_t timeout = 500);
  boolean sendCheckReply(char *send, char *reply, uint16_t timeout = 500);

#if ARDUINO >= 100
  SoftwareSerial *mySerial;
#else
  NewSoftSerial *mySerial;
#endif
};
