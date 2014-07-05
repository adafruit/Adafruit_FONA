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

#include "Adafruit_FONA.h"

#if ARDUINO >= 100
Adafruit_FONA::Adafruit_FONA(SoftwareSerial *ss, int8_t rst)
#else
Adafruit_FONA::Adafruit_FONA(NewSoftSerial *ssm, int8_t rst) 
#endif
{
  _rstpin = rst;
  mySerial = ss;
}

boolean Adafruit_FONA::begin(uint16_t baudrate) {
  pinMode(_rstpin, OUTPUT);
  digitalWrite(_rstpin, HIGH);
  delay(10);
  digitalWrite(_rstpin, LOW);
  delay(100);
  digitalWrite(_rstpin, HIGH);

  // give 3 seconds to reboot
  delay(3000);

  mySerial->begin(baudrate);
  delay(500);
  while (mySerial->available()) mySerial->read();

  sendCheckReply("AT", "OK");
  delay(100);
  sendCheckReply("AT", "OK");
  delay(100);
  sendCheckReply("AT", "OK");
  delay(100);

  // turn off Echo!
  sendCheckReply("ATE0", "OK");
  delay(100);
  
  if (! sendCheckReply("ATE0", "OK")) {
    return false;
  }

  return true;
}

/********* BATTERY & ADC ********************************************/

/* returns value in mV (uint16_t) */
boolean Adafruit_FONA::getBattVoltage(uint16_t *v) {
  getReply("AT+CBC");  // query the battery ADC
  char *p = strstr(replybuffer, "+CBC: ");  // check reply
  if (p == 0) return false;
  p = strrchr(replybuffer, ',');
  if (p == 0) return false;
  p++;
  //Serial.println(p);
  *v = atoi(p);

  readline(); // eat 'OK'

  return true;
}

boolean Adafruit_FONA::getADCVoltage(uint16_t *v) {
  getReply("AT+CADC?");  // query the ADC
  char *p = strstr(replybuffer, "+CADC: 1,");  // get the pointer to the voltage
  if (p == 0) return false;
  p+=9;
  //Serial.println(p);
  *v = atoi(p);

  readline(); // eat 'OK'

  return true;
}

/********* SIM ***********************************************************/

uint8_t Adafruit_FONA::unlockSIM(char *pin)
{
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = NULL;
  
  return sendCheckReply(sendbuff, "OK");
}

uint8_t Adafruit_FONA::getSIMCCID(char *ccid) {
   getReply("AT+CCID");
   // up to 20 chars
   strncpy(ccid, replybuffer, 20);
   ccid[20] = 0;

   readline(); // eat 'OK'

   return strlen(ccid);
}

/********* NETWORK *******************************************************/

uint8_t Adafruit_FONA::getNetworkStatus(void) {
  uint8_t status;

  getReply("AT+CREG?");
  char *p = strstr(replybuffer, "+CREG: ");  // get the pointer to the voltage
  if (p == 0) return false;
  p+=9;
  //Serial.println(p);
  status = atoi(p);

  readline(); // eat 'OK'

  return status;
}


uint8_t Adafruit_FONA::getRSSI(void) {
  uint8_t rssi;

  getReply("AT+CSQ");
  char *p = strstr(replybuffer, "+CSQ: ");  // get the pointer to the voltage
  if (p == 0) return false;
  p+=6;
  //Serial.println(p);
  rssi = atoi(p);

  readline(); // eat the "OK"

  return rssi;
}

/********* AUDIO *******************************************************/

boolean Adafruit_FONA::setAudio(uint8_t a) {
  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  char sendbuff[12] = "AT+CHFA=0";
  sendbuff[8] = a + '0';

  return sendCheckReply(sendbuff, "OK");
}

uint8_t Adafruit_FONA::getVolume(void) {
  uint8_t level;

  getReply("AT+CLVL?");
  char *p = strstr(replybuffer, "+CLVL: ");  // get the pointer to the voltage
  if (p == 0) return false;
  p+=7;
  //Serial.println(p);
  level = atoi(p);

  readline();  // eat the "OK"

  return level;
}

boolean Adafruit_FONA::setVolume(uint8_t i) {
  char sendbuff[12] = "AT+CLVL=";
  itoa(i, sendbuff+8, 10);
  return sendCheckReply(sendbuff, "OK");
}


boolean Adafruit_FONA::playToolkitTone(uint8_t t, uint16_t len) {
  char sendbuff[35] = "AT+STTONE=1,";
  char *p = sendbuff+12;
  itoa(t, p, 10);
  //Serial.println(sendbuff);
  p = sendbuff+strlen(sendbuff);
  *p = ',';
  p++;
  itoa(len, p, 10);
  //Serial.println(sendbuff);
  
  return sendCheckReply(sendbuff, "OK");
}

boolean Adafruit_FONA::setMicVolume(uint8_t a, uint8_t level) {
  char sendbuff[35] = "AT+CMIC=0,";

  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  sendbuff[8] = a + '0';
  char *p = sendbuff+10;
  itoa(level, p, 10);
  
  return sendCheckReply(sendbuff, "OK");
}

/********* FM RADIO *******************************************************/


boolean Adafruit_FONA::FMradio(boolean onoff, uint8_t a) {
  if (! onoff) {
    return sendCheckReply("AT+FMCLOSE", "OK");
  }

  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  char sendbuff[12] = "AT+FMOPEN=0";
  sendbuff[10] = a + '0';

  return sendCheckReply(sendbuff, "OK");
}

boolean Adafruit_FONA::tuneFMradio(uint16_t station) {
  if ((station < 870) || (station > 1090))
    return false;

  char sendbuff[20] = "AT+FMFREQ=";
  itoa(station, sendbuff+10, 10);
  return sendCheckReply(sendbuff, "OK");
}

boolean Adafruit_FONA::setFMVolume(uint8_t i) {
  // Fail if volume is outside allowed range (0-6).
  if (i > 6) {
    return false;
  }
  // Build FM volume set command.
  char sendbuff[14] = "AT+FMVOLUME=";
  itoa(i, sendbuff+12, 10);
  // Send FM volume command and verify response.
  return sendCheckReply(sendbuff, "OK");
}

int8_t Adafruit_FONA::getFMVolume() {
  // Send FM volume read.
  getReply("AT+FMVOLUME?");
  // Check response is expected value.
  char *p = strstr(replybuffer, "+FMVOLUME: ");
  // Parse FM volume from response and return it.
  if (p == 0) return -1;
  p+=11;
  uint8_t level = atoi(p);
  readline();  // eat the "OK"
  return level;
}

/********* PWM/BUZZER **************************************************/

boolean Adafruit_FONA::PWM(uint16_t period, uint8_t duty) {
  if (period > 2000) return false;
  if (duty > 100) return false;


  char sendbuff[35] = "AT+SPWM=0,";

  char *p = sendbuff+10;
  itoa(period, p, 10);
  //Serial.println(sendbuff);
  p = sendbuff+strlen(sendbuff);
  *p = ',';
  p++;
  itoa(duty, p, 10);
  //Serial.println(sendbuff);
  
  return sendCheckReply(sendbuff, "OK");
}

/********* CALL PHONES **************************************************/
boolean Adafruit_FONA::callPhone(char *number) {
  char sendbuff[35] = "ATD";
  strncpy(sendbuff+3, number, min(30, strlen(number)));
  uint8_t x = strlen(sendbuff);
  sendbuff[x] = ';';
  sendbuff[x+1] = 0;
  //Serial.println(sendbuff);

  return sendCheckReply(sendbuff, "OK");
}

boolean Adafruit_FONA::hangUp(void) {
  return sendCheckReply("ATH0", "OK");
}

/********* LOW LEVEL *******************************************/
uint8_t Adafruit_FONA::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;
  
  while (timeout--) {
    if (replyidx > 254) {
      //Serial.println(F("SPACE"));
      break;
    }

    while(mySerial->available()) {
      char c =  mySerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;
        
        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }
    
    if (timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

int8_t Adafruit_FONA::getNumSMS(void) {
  if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  // ask how many sms are stored
  getReply("AT+CPMS?");
  char *p = strstr(replybuffer, "+CPMS: \"SM_P\",");
  if (! p) return -1;
  p += 14;
#ifdef ADAFRUIT_FONA_DEBUG
  Serial.println(p);
#endif
  uint8_t used = atoi(p);
  
  return used;
}

uint8_t Adafruit_FONA::readSMS(uint8_t i, char *smsbuff, uint8_t maxlen) {
  if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  // read an sms
  char sendbuff[12] = "AT+CMGR=000";
  sendbuff[8] = (i / 100) + '0';
  i %= 100;
  sendbuff[9] = (i / 10) + '0';
  i %= 10;
  sendbuff[10] = i + '0';

  getReply(sendbuff);  // ignore first line (status)
  uint8_t len = readline(1000, true);  // 2s timeout, multiline read
  
  /*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
    Serial.print(replybuffer[i], HEX); Serial.print(" ");
  }
  Serial.println();
  */
  //Serial.println(replybuffer);
  // clear out ending OK
  char *p = strstr(replybuffer, "\n\nOK\n");
  if (p) {
    p[0] = 0;
  }

  uint8_t thelen = min(maxlen, strlen(replybuffer));
  strncpy(smsbuff, replybuffer, thelen);
  smsbuff[thelen] = 0; // end the string


#ifdef ADAFRUIT_FONA_DEBUG
  Serial.println(replybuffer);
#endif
  return strlen(replybuffer);
}

boolean Adafruit_FONA::sendSMS(char *smsaddr, char *smsmsg) {
  if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  
  char sendcmd[30] = "AT+CMGS=\"";
  strncpy(sendcmd+9, smsaddr, 30-9-2);  // 9 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';
  
  if (! sendCheckReply(sendcmd, "> ")) return false;
#ifdef ADAFRUIT_FONA_DEBUG
  Serial.print("> "); Serial.println(smsmsg);
#endif
  mySerial->println(smsmsg);
  mySerial->println();
  mySerial->write(0x1A);
#ifdef ADAFRUIT_FONA_DEBUG
  Serial.println("^Z");
#endif
  readline(10000); // read the +CMGS reply, wait up to 10 seconds!!!
  //Serial.print("* "); Serial.println(replybuffer);
  if (strstr(replybuffer, "+CMGS") == 0) {
    return false;
  }
  readline(1000); // read OK
  //Serial.print("* "); Serial.println(replybuffer);
  
  if (strcmp(replybuffer, "OK") != 0) {
    return false;
  }
  
  return true;
}


boolean Adafruit_FONA::deleteSMS(uint8_t i) {
    if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  // read an sms
  char sendbuff[12] = "AT+CMGD=000";
  sendbuff[8] = (i / 100) + '0';
  i %= 100;
  sendbuff[9] = (i / 10) + '0';
  i %= 10;
  sendbuff[10] = i + '0';
  
  return sendCheckReply(sendbuff, "OK", 2000);
}

uint8_t Adafruit_FONA::getReply(char *send, uint16_t timeout) {
  // flush input
  while(mySerial->available()) {
     mySerial->read();
  }
  
#ifdef ADAFRUIT_FONA_DEBUG
  Serial.print("\t---> "); Serial.println(send); 
#endif

  mySerial->println(send);
  
  uint8_t l = readline(timeout);  
#ifdef ADAFRUIT_FONA_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

boolean Adafruit_FONA::sendCheckReply(char *send, char *reply, uint16_t timeout) {
  getReply(send, timeout);

/*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
    Serial.print(replybuffer[i], HEX); Serial.print(" ");
  }
  Serial.println();
  for (uint8_t i=0; i<strlen(reply); i++) {
    Serial.print(reply[i], HEX); Serial.print(" ");
  }
  Serial.println();
  */
  return (strcmp(replybuffer, reply) == 0);
}
