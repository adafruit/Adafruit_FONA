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
    // next line per http://postwarrior.com/arduino-ethershield-error-prog_char-does-not-name-a-type/

#include "Adafruit_FONA.h"




Adafruit_FONA::Adafruit_FONA(int8_t rst)
{
  _rstpin = new mraa::Gpio(rst);

  apn = F("internet.t-mobile");
  apnusername = "t-mobile";
  apnpassword = "tm";
  mySerial = 0;
  httpsredirect = false;
  useragent = F("FONA");
  ok_reply = F("OK");
}

uint8_t Adafruit_FONA::type(void) {
  return _type;
}

boolean Adafruit_FONA::begin(mraa::Uart &port) {
  mySerial = &port;

  if (_rstpin == NULL) {
    std::cout << "Null pointer reset pin" << std::endl;
      return mraa::ERROR_UNSPECIFIED;
  }
  mraa::Result response = _rstpin->dir(mraa::DIR_OUT);
  if (response != mraa::SUCCESS) {
      mraa::printError(response);
      return 1;
  }

  _rstpin->write(1);
  delay(10);
  _rstpin->write(0);
  delay(100);
  _rstpin->write(1);
  delay(5000);         // wait 5 sec for reset to finish

  DEBUG_PRINTLN(F("Attempting to open comm with ATs"));
  // give 2 seconds to start connection
  int16_t timeout = 2000;
  char dummy;

  while (timeout > 0) {
    while (mySerial->dataAvailable()) {
      mySerial->read(&dummy, 1);
      std::cout << "Flush fona serial before ok_reply: " << dummy << '\n';
    }
    if (sendCheckReply("AT", ok_reply))
      break;
    while (mySerial->dataAvailable()) {
      mySerial->read(&dummy, 1);
      std::cout << "flush fona serial before \"AT\" reply: " << dummy << '\n';
    }
    if (sendCheckReply("AT", "AT"))
      break;
    delay(500);
    timeout-=500;
  }

  if (timeout <= 0) {
#ifdef ADAFRUIT_FONA_DEBUG
    DEBUG_PRINTLN(F("Timeout: No response to AT... last ditch attempt."));
#endif
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
    sendCheckReply(F("AT"), ok_reply);
    delay(100);
  }

  // turn off Echo!
  sendCheckReply(F("ATE0"), ok_reply);
  delay(100);

  if (! sendCheckReply(F("ATE0"), ok_reply, 1000)) {
    return false;
  }

  // turn on hangupitude
  sendCheckReply(F("AT+CVHU=0"), ok_reply);

  delay(100);
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("ATI");

  mySerial->writeStr("ATI\r\n");
  // mySerial->writeStr("ATI");

  readline(500, true);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);



  // if (prog_char_strstr(replybuffer, (prog_char *)F("SIM808 R14")) != 0) {
  if (static_cast<std::string>(replybuffer).find("SIM808 R14") != std::string::npos){
    _type = FONA808_V2;
  } else if (static_cast<std::string>(replybuffer).find("SIM808 R13") != std::string::npos) {
    _type = FONA808_V1;
  } else if (static_cast<std::string>(replybuffer).find("SIM800 R13") != std::string::npos) {
    _type = FONA800L;
  } else if (static_cast<std::string>(replybuffer).find("SIMCOM_SIM5320A") != std::string::npos) {
    _type = FONA3G_A;
  } else if (static_cast<std::string>(replybuffer).find("SIMCOM_SIM5320E") != std::string::npos) {
    _type = FONA3G_E;
  }

  if (_type == FONA800L) {
    // determine if L or H

  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN("AT+GMM");

    mySerial->writeStr("AT+GMM\r\n");
    // mySerial->writeStr("AT+GMM");

    readline(500, true);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);


    if (static_cast<std::string>(replybuffer).find("SIM800H") != std::string::npos) {
      _type = FONA800H;
    }
  }

#if defined(FONA_PREF_SMS_STORAGE)
  sendCheckReply(F("AT+CPMS=\"" FONA_PREF_SMS_STORAGE "\""), ok_reply);
#endif

  return true;
}


/********* Serial port ********************************************/
boolean Adafruit_FONA::setBaudrate(int32_t baud) {
  return sendCheckReply(F("AT+IPREX="), baud, ok_reply);
}

/********* Real Time Clock ********************************************/

// boolean Adafruit_FONA::readRTC(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *hr, uint8_t *min, uint8_t *sec) {
//   uint16_t v;
//   sendParseReply(F("AT+CCLK?"), F("+CCLK: "), &v, '/', 0);
//   *year = v;
//
//   DEBUG_PRINTLN(*year);
// }
//
// boolean Adafruit_FONA::enableRTC(uint8_t i) {
//   if (! sendCheckReply(F("AT+CLTS="), i, ok_reply))
//     return false;
//   return sendCheckReply(F("AT&W"), ok_reply);
// }


/********* BATTERY & ADC ********************************************/

// /* returns value in mV (uint16_t) */
// boolean Adafruit_FONA::getBattVoltage(uint16_t *v) {
//   return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
// }

/* returns value in mV (uint16_t) */
boolean Adafruit_FONA_3G::getBattVoltage(uint16_t *v) {
  float f;
  boolean b = sendParseReply(F("AT+CBC"), F("+CBC: "), &f, ',', 2);
  *v = f*1000;
  return b;
}


/* returns the percentage charge of battery as reported by sim800 */
boolean Adafruit_FONA::getBattPercent(uint16_t *p) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}

boolean Adafruit_FONA::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
}

boolean Adafruit_FONA_3G::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC=2"), F("+CADC:"), v);
}


/********* SIM ***********************************************************/

uint8_t Adafruit_FONA::unlockSIM(char *pin)
{
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = '\0';

  return sendCheckReply(sendbuff, ok_reply);
}

uint8_t Adafruit_FONA::getSIMCCID(char *ccid) {
  getReply(F("AT+CCID"));
  // up to 28 chars for reply, 20 char total ccid
  if (replybuffer[0] == '+') {
    // fona 3g?
    strncpy(ccid, replybuffer+8, 20);
  } else {
    // fona 800 or 800
    strncpy(ccid, replybuffer, 20);
  }
  ccid[20] = 0;

  readline(); // eat 'OK'

  return strlen(ccid);
}

/********* IMEI **********************************************************/

uint8_t Adafruit_FONA::getIMEI(char *imei) {
  getReply(F("AT+GSN"));

  // up to 15 chars
  strncpy(imei, replybuffer, 15);
  imei[15] = 0;

  readline(); // eat 'OK'

  return strlen(imei);
}

/********* NETWORK *******************************************************/

uint8_t Adafruit_FONA::getNetworkStatus(void) {
  uint16_t status;

  if (! sendParseReply(F("AT+CREG?"), F("+CREG: "), &status, ',', 1)) return 0;

  return status;
}


uint8_t Adafruit_FONA::getRSSI(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply) ) return 0;

  return reply;
}

/********* AUDIO *******************************************************/

// boolean Adafruit_FONA::setAudio(uint8_t a) {
//   // 0 is headset, 1 is external audio
//   if (a > 1) return false;
//
//   return sendCheckReply(F("AT+CHFA="), a, ok_reply);
// }
//
// uint8_t Adafruit_FONA::getVolume(void) {
//   uint16_t reply;
//
//   if (! sendParseReply(F("AT+CLVL?"), F("+CLVL: "), &reply) ) return 0;
//
//   return reply;
// }
//
// boolean Adafruit_FONA::setVolume(uint8_t i) {
//   return sendCheckReply(F("AT+CLVL="), i, ok_reply);
// }
//
//
// boolean Adafruit_FONA::playDTMF(char dtmf) {
//   char str[4];
//   str[0] = '\"';
//   str[1] = dtmf;
//   str[2] = '\"';
//   str[3] = 0;
//   return sendCheckReply(F("AT+CLDTMF=3,"), str, ok_reply);
// }

// boolean Adafruit_FONA::playToolkitTone(uint8_t t, uint16_t len) {
//   return sendCheckReply(F("AT+STTONE=1,"), t, len, ok_reply);
// }
//
// boolean Adafruit_FONA_3G::playToolkitTone(uint8_t t, uint16_t len) {
//   if (! sendCheckReply(F("AT+CPTONE="), t, ok_reply))
//     return false;
//   delay(len);
//   return sendCheckReply(F("AT+CPTONE=0"), ok_reply);
// }
//
// boolean Adafruit_FONA::setMicVolume(uint8_t a, uint8_t level) {
//   // 0 is headset, 1 is external audio
//   if (a > 1) return false;
//
//   return sendCheckReply(F("AT+CMIC="), a, level, ok_reply);
// }

/********* FM RADIO *******************************************************/


// boolean Adafruit_FONA::FMradio(boolean onoff, uint8_t a) {
//   if (! onoff) {
//     return sendCheckReply(F("AT+FMCLOSE"), ok_reply);
//   }
//
//   // 0 is headset, 1 is external audio
//   if (a > 1) return false;
//
//   return sendCheckReply(F("AT+FMOPEN="), a, ok_reply);
// }
//
// boolean Adafruit_FONA::tuneFMradio(uint16_t station) {
//   // Fail if FM station is outside allowed range.
//   if ((station < 870) || (station > 1090))
//     return false;
//
//   return sendCheckReply(F("AT+FMFREQ="), station, ok_reply);
// }
//
// boolean Adafruit_FONA::setFMVolume(uint8_t i) {
//   // Fail if volume is outside allowed range (0-6).
//   if (i > 6) {
//     return false;
//   }
//   // Send FM volume command and verify response.
//   return sendCheckReply(F("AT+FMVOLUME="), i, ok_reply);
// }
//
// int8_t Adafruit_FONA::getFMVolume() {
//   uint16_t level;
//
//   if (! sendParseReply(F("AT+FMVOLUME?"), F("+FMVOLUME: "), &level) ) return 0;
//
//   return level;
// }

// int8_t Adafruit_FONA::getFMSignalLevel(uint16_t station) {
//   // Fail if FM station is outside allowed range.
//   if ((station < 875) || (station > 1080)) {
//     return -1;
//   }
//
//   // Send FM signal level query command.
//   // Note, need to explicitly send timeout so right overload is chosen.
//   getReply(F("AT+FMSIGNAL="), station, FONA_DEFAULT_TIMEOUT_MS);
//   // Check response starts with expected value.
//   char *p = prog_char_strstr(replybuffer, PSTR("+FMSIGNAL: "));
//   if (p == 0) return -1;
//   p+=11;
//   // Find second colon to get start of signal quality.
//   p = strchr(p, ':');
//   if (p == 0) return -1;
//   p+=1;
//   // Parse signal quality.
//   int8_t level = atoi(p);
//   readline();  // eat the "OK"
//   return level;
// }

/********* PWM/BUZZER **************************************************/

// boolean Adafruit_FONA::setPWM(uint16_t period, uint8_t duty) {
//   if (period > 2000) return false;
//   if (duty > 100) return false;
//
//   return sendCheckReply(F("AT+SPWM=0,"), period, duty, ok_reply);
// }

/********* CALL PHONES **************************************************/
// boolean Adafruit_FONA::callPhone(char *number) {
//   char sendbuff[35] = "ATD";
//   strncpy(sendbuff+3, number, min(30, strlen(number)));
//   uint8_t x = strlen(sendbuff);
//   sendbuff[x] = ';';
//   sendbuff[x+1] = 0;
//   //DEBUG_PRINTLN(sendbuff);
//
//   return sendCheckReply(sendbuff, ok_reply);
// }
//
//
// uint8_t Adafruit_FONA::getCallStatus(void) {
//   uint16_t phoneStatus;
//
//   if (! sendParseReply(F("AT+CPAS"), F("+CPAS: "), &phoneStatus))
//     return FONA_CALL_FAILED; // 1, since 0 is actually a known, good reply
//
//   return phoneStatus;  // 0 ready, 2 unkown, 3 ringing, 4 call in progress
// }
//
// boolean Adafruit_FONA::hangUp(void) {
//   return sendCheckReply(F("ATH0"), ok_reply);
// }
//
// boolean Adafruit_FONA_3G::hangUp(void) {
//   getReply(F("ATH"));
//
//   return (prog_char_strstr(replybuffer, (prog_char *)F("VOICE CALL: END")) != 0);
// }
//
// boolean Adafruit_FONA::pickUp(void) {
//   return sendCheckReply(F("ATA"), ok_reply);
// }
//
// boolean Adafruit_FONA_3G::pickUp(void) {
//   return sendCheckReply(F("ATA"), F("VOICE CALL: BEGIN"));
// }


// void Adafruit_FONA::onIncomingCall() {
//
//   DEBUG_PRINT(F("> ")); DEBUG_PRINTLN(F("Incoming call..."));
//
//   Adafruit_FONA::_incomingCall = true;
// }
//
// boolean Adafruit_FONA::_incomingCall = false;
//
// boolean Adafruit_FONA::callerIdNotification(boolean enable, uint8_t interrupt) {
//   if(enable){
//     attachInterrupt(interrupt, onIncomingCall, FALLING);
//     return sendCheckReply(F("AT+CLIP=1"), ok_reply);
//   }
//
//   detachInterrupt(interrupt);
//   return sendCheckReply(F("AT+CLIP=0"), ok_reply);
// }
//
// boolean Adafruit_FONA::incomingCallNumber(char* phonenum) {
//   //+CLIP: "<incoming phone number>",145,"",0,"",0
//   if(!Adafruit_FONA::_incomingCall)
//     return false;
//
//   readline();
//   while(!prog_char_strcmp(replybuffer, (prog_char*)F("RING")) == 0) {
//     flushInput();
//     readline();
//   }
//
//   readline(); //reads incoming phone number line
//
//   parseReply(F("+CLIP: \""), phonenum, '"');
//
//
//   DEBUG_PRINT(F("Phone Number: "));
//   DEBUG_PRINTLN(replybuffer);
//
//
//   Adafruit_FONA::_incomingCall = false;
//   return true;
// }

/********* SMS **********************************************************/

uint8_t Adafruit_FONA::getSMSInterrupt(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CFGRI?"), F("+CFGRI: "), &reply) ) return 0;

  return reply;
}

boolean Adafruit_FONA::setSMSInterrupt(uint8_t i) {
  return sendCheckReply(F("AT+CFGRI="), i, ok_reply);
}

int8_t Adafruit_FONA::getNumSMS(void) {
  uint16_t numsms;

  // get into text mode
  if (! sendCheckReply(F("AT+CMGF=1"), ok_reply)) return -1;

  // ask how many sms are stored
  if (sendParseReply(F("AT+CPMS?"), F("\"ME\","), &numsms))
      return numsms;
  if (sendParseReply(F("AT+CPMS?"), F("\"SM\","), &numsms))
    return numsms;
  if (sendParseReply(F("AT+CPMS?"), F("\"SM_P\","), &numsms))
    return numsms;
  return -1;
}

// Reading SMS's is a bit involved so we don't use helpers that may cause delays or debug
// printouts!
boolean Adafruit_FONA::readSMS(uint8_t i, char *smsbuff,
			       uint16_t maxlen, uint16_t *readlen) {
  // text mode
  if (! sendCheckReply(F("AT+CMGF=1"), ok_reply)) return false;

  // show all text mode parameters
  if (! sendCheckReply(F("AT+CSDH=1"), ok_reply)) return false;

  // parse out the SMS len
  uint16_t thesmslen = 0;


  DEBUG_PRINT(F("AT+CMGR="));
  DEBUG_PRINTLN(i);


  //getReply(F("AT+CMGR="), i, 1000);  //  do not print debug!
  mySerial->writeStr(F("AT+CMGR="));
  mySerial->writeStr(std::to_string(i));
  mySerial->writeStr("\r\n");
  readline(1000); // timeout

  //DEBUG_PRINT(F("Reply: ")); DEBUG_PRINTLN(replybuffer);
  // parse it out...


  DEBUG_PRINTLN(replybuffer);


  if (! parseReply(F("+CMGR:"), &thesmslen, ',', 11)) {
    *readlen = 0;
    return false;
  }

  readRaw(thesmslen);

  flushInput();

  uint16_t thelen = std::min(maxlen, static_cast<uint16_t>(strlen(replybuffer)));
  strncpy(smsbuff, replybuffer, thelen);
  smsbuff[thelen] = 0; // end the string


  DEBUG_PRINTLN(replybuffer);

  *readlen = thelen;
  return true;
}

// Retrieve the sender of the specified SMS message and copy it as a string to
// the sender buffer.  Up to senderlen characters of the sender will be copied
// and a null terminator will be added if less than senderlen charactesr are
// copied to the result.  Returns true if a result was successfully retrieved,
// otherwise false.
boolean Adafruit_FONA::getSMSSender(uint8_t i, char *sender, int senderlen) {
  // Ensure text mode and all text mode parameters are sent.
  if (! sendCheckReply(F("AT+CMGF=1"), ok_reply)) return false;
  if (! sendCheckReply(F("AT+CSDH=1"), ok_reply)) return false;


  DEBUG_PRINT(F("AT+CMGR="));
  DEBUG_PRINTLN(std::to_string(i));


  // Send command to retrieve SMS message and parse a line of response.
  mySerial->writeStr(F("AT+CMGR="));
  // mySerial->println(i);
  mySerial->writeStr(std::to_string(i));
  mySerial->writeStr("\r\n");
  readline(1000);


  DEBUG_PRINTLN(replybuffer);


  // Parse the second field in the response.
  boolean result = parseReplyQuoted(F("+CMGR:"), sender, senderlen, ',', 1);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

boolean Adafruit_FONA::sendSMS(char *smsaddr, char *smsmsg) {
  if (! sendCheckReply(F("AT+CMGF=1"), ok_reply)) return false;

  char sendcmd[30] = "AT+CMGS=\"";
  strncpy(sendcmd+9, smsaddr, 30-9-2);  // 9 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';

  if (! sendCheckReply(sendcmd, F("> "))) return false;

  DEBUG_PRINT(F("> ")); DEBUG_PRINTLN(smsmsg);

  mySerial->writeStr(smsmsg);
  mySerial->writeStr("\r\n\r\n");
  const char c = 0x1A;
  mySerial->write(&c,1);

  DEBUG_PRINTLN("^Z");

  if ( (_type == FONA3G_A) || (_type == FONA3G_E) ) {
    // Eat two sets of CRLF
    readline(200);
    //DEBUG_PRINT("Line 1: "); DEBUG_PRINTLN(strlen(replybuffer));
    readline(200);
    //DEBUG_PRINT("Line 2: "); DEBUG_PRINTLN(strlen(replybuffer));
  }
  readline(10000); // read the +CMGS reply, wait up to 10 seconds!!!
  //DEBUG_PRINT("Line 3: "); DEBUG_PRINTLN(strlen(replybuffer));
  if (strstr(replybuffer, "+CMGS") == 0) {
    return false;
  }
  readline(1000); // read OK
  //DEBUG_PRINT("* "); DEBUG_PRINTLN(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    return false;
  }

  return true;
}


boolean Adafruit_FONA::deleteSMS(uint8_t i) {
    if (! sendCheckReply(F("AT+CMGF=1"), ok_reply)) return false;
  // read an sms
  char sendbuff[12] = "AT+CMGD=000";
  sendbuff[8] = (i / 100) + '0';
  i %= 100;
  sendbuff[9] = (i / 10) + '0';
  i %= 10;
  sendbuff[10] = i + '0';

  return sendCheckReply(sendbuff, ok_reply, 2000);
}

/********* USSD *********************************************************/

// boolean Adafruit_FONA::sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen, uint16_t *readlen) {
//   if (! sendCheckReply(F("AT+CUSD=1"), ok_reply)) return false;
//
//   char sendcmd[30] = "AT+CUSD=1,\"";
//   strncpy(sendcmd+11, ussdmsg, 30-11-2);  // 11 bytes beginning, 2 bytes for close quote + null
//   sendcmd[strlen(sendcmd)] = '\"';
//
//   if (! sendCheckReply(sendcmd, ok_reply)) {
//     *readlen = 0;
//     return false;
//   } else {
//       readline(10000); // read the +CUSD reply, wait up to 10 seconds!!!
//       //DEBUG_PRINT("* "); DEBUG_PRINTLN(replybuffer);
//       char *p = prog_char_strstr(replybuffer, PSTR("+CUSD: "));
//       if (p == 0) {
//         *readlen = 0;
//         return false;
//       }
//       p+=7; //+CUSD
//       // Find " to get start of ussd message.
//       p = strchr(p, '\"');
//       if (p == 0) {
//         *readlen = 0;
//         return false;
//       }
//       p+=1; //"
//       // Find " to get end of ussd message.
//       char *strend = strchr(p, '\"');
//
//       uint16_t lentocopy = min(maxlen-1, strend - p);
//       strncpy(ussdbuff, p, lentocopy+1);
//       ussdbuff[lentocopy] = 0;
//       *readlen = lentocopy;
//   }
//   return true;
// }


/********* TIME **********************************************************/

// boolean Adafruit_FONA::enableNetworkTimeSync(boolean onoff) {
//   if (onoff) {
//     if (! sendCheckReply(F("AT+CLTS=1"), ok_reply))
//       return false;
//   } else {
//     if (! sendCheckReply(F("AT+CLTS=0"), ok_reply))
//       return false;
//   }
//
//   flushInput(); // eat any 'Unsolicted Result Code'
//
//   return true;
// }

// boolean Adafruit_FONA::enableNTPTimeSync(boolean onoff, std::string ntpserver) {
//   if (onoff) {
//     if (! sendCheckReply(F("AT+CNTPCID=1"), ok_reply))
//       return false;
//
//     mySerial->print(F("AT+CNTP=\""));
//     if (ntpserver != 0) {
//       mySerial->print(ntpserver);
//     } else {
//       mySerial->print(F("pool.ntp.org"));
//     }
//     mySerial->println(F("\",0"));
//     readline(FONA_DEFAULT_TIMEOUT_MS);
//     if (strcmp(replybuffer, "OK") != 0)
//       return false;
//
//     if (! sendCheckReply(F("AT+CNTP"), ok_reply, 10000))
//       return false;
//
//     uint16_t status;
//     readline(10000);
//     if (! parseReply(F("+CNTP:"), &status))
//       return false;
//   } else {
//     if (! sendCheckReply(F("AT+CNTPCID=0"), ok_reply))
//       return false;
//   }
//
//   return true;
// }

// boolean Adafruit_FONA::getTime(char *buff, uint16_t maxlen) {
//   getReply(F("AT+CCLK?"), (uint16_t) 10000);
//   if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
//     return false;
//
//   char *p = replybuffer+7;
//   uint16_t lentocopy = min(maxlen-1, strlen(p));
//   strncpy(buff, p, lentocopy+1);
//   buff[lentocopy] = 0;
//
//   readline(); // eat OK
//
//   return true;
// }

/********* GPS **********************************************************/


// boolean Adafruit_FONA::enableGPS(boolean onoff) {
//   uint16_t state;
//
//   // first check if its already on or off
//
//   if (_type == FONA808_V2) {
//     if (! sendParseReply(F("AT+CGNSPWR?"), F("+CGNSPWR: "), &state) )
//       return false;
//   } else {
//     if (! sendParseReply(F("AT+CGPSPWR?"), F("+CGPSPWR: "), &state))
//       return false;
//   }
//
//   if (onoff && !state) {
//     if (_type == FONA808_V2) {
//       if (! sendCheckReply(F("AT+CGNSPWR=1"), ok_reply))  // try GNS command
// 	return false;
//     } else {
//       if (! sendCheckReply(F("AT+CGPSPWR=1"), ok_reply))
// 	return false;
//     }
//   } else if (!onoff && state) {
//     if (_type == FONA808_V2) {
//       if (! sendCheckReply(F("AT+CGNSPWR=0"), ok_reply)) // try GNS command
// 	return false;
//     } else {
//       if (! sendCheckReply(F("AT+CGPSPWR=0"), ok_reply))
// 	return false;
//     }
//   }
//   return true;
// }



// boolean Adafruit_FONA_3G::enableGPS(boolean onoff) {
//   uint16_t state;
//
//   // first check if its already on or off
//   if (! Adafruit_FONA::sendParseReply(F("AT+CGPS?"), F("+CGPS: "), &state) )
//     return false;
//
//   if (onoff && !state) {
//     if (! sendCheckReply(F("AT+CGPS=1"), ok_reply))
//       return false;
//   } else if (!onoff && state) {
//     if (! sendCheckReply(F("AT+CGPS=0"), ok_reply))
//       return false;
//     // this takes a little time
//     readline(2000); // eat '+CGPS: 0'
//   }
//   return true;
// }
//
// int8_t Adafruit_FONA::GPSstatus(void) {
//   if (_type == FONA808_V2) {
//     // 808 V2 uses GNS commands and doesn't have an explicit 2D/3D fix status.
//     // Instead just look for a fix and if found assume it's a 3D fix.
//     getReply(F("AT+CGNSINF"));
//     char *p = prog_char_strstr(replybuffer, (prog_char*)F("+CGNSINF: "));
//     if (p == 0) return -1;
//     p+=10;
//     readline(); // eat 'OK'
//     if (p[0] == '0') return 0; // GPS is not even on!
//
//     p+=2; // Skip to second value, fix status.
//     //DEBUG_PRINTLN(p);
//     // Assume if the fix status is '1' then we have a 3D fix, otherwise no fix.
//     if (p[0] == '1') return 3;
//     else return 1;
//   }
//   if (_type == FONA3G_A || _type == FONA3G_E) {
//     // FONA 3G doesn't have an explicit 2D/3D fix status.
//     // Instead just look for a fix and if found assume it's a 3D fix.
//     getReply(F("AT+CGPSINFO"));
//     char *p = prog_char_strstr(replybuffer, (prog_char*)F("+CGPSINFO:"));
//     if (p == 0) return -1;
//     if (p[10] != ',') return 3; // if you get anything, its 3D fix
//     return 0;
//   }
//   else {
//     // 808 V1 looks for specific 2D or 3D fix state.
//     getReply(F("AT+CGPSSTATUS?"));
//     char *p = prog_char_strstr(replybuffer, (prog_char*)F("SSTATUS: Location "));
//     if (p == 0) return -1;
//     p+=18;
//     readline(); // eat 'OK'
//     //DEBUG_PRINTLN(p);
//     if (p[0] == 'U') return 0;
//     if (p[0] == 'N') return 1;
//     if (p[0] == '2') return 2;
//     if (p[0] == '3') return 3;
//   }
//   // else
//   return 0;
// }
//
// uint8_t Adafruit_FONA::getGPS(uint8_t arg, char *buffer, uint8_t maxbuff) {
//   int32_t x = arg;
//
//   if ( (_type == FONA3G_A) || (_type == FONA3G_E) ) {
//     getReply(F("AT+CGPSINFO"));
//   } else if (_type == FONA808_V1) {
//     getReply(F("AT+CGPSINF="), x);
//   } else {
//     getReply(F("AT+CGNSINF"));
//   }
//
//   char *p = prog_char_strstr(replybuffer, (prog_char*)F("SINF"));
//   if (p == 0) {
//     buffer[0] = 0;
//     return 0;
//   }
//
//   p+=6;
//
//   uint8_t len = max(maxbuff-1, strlen(p));
//   strncpy(buffer, p, len);
//   buffer[len] = 0;
//
//   readline(); // eat 'OK'
//   return len;
// }
//
// boolean Adafruit_FONA::getGPS(float *lat, float *lon, float *speed_kph, float *heading, float *altitude) {
//
//   char gpsbuffer[120];
//
//   // we need at least a 2D fix
//   if (GPSstatus() < 2)
//     return false;
//
//   // grab the mode 2^5 gps csv from the sim808
//   uint8_t res_len = getGPS(32, gpsbuffer, 120);
//
//   // make sure we have a response
//   if (res_len == 0)
//     return false;
//
//   if (_type == FONA3G_A || _type == FONA3G_E) {
//     // Parse 3G respose
//     // +CGPSINFO:4043.000000,N,07400.000000,W,151015,203802.1,-12.0,0.0,0
//     // skip beginning
//     char *tok;
//
//    // grab the latitude
//     char *latp = strtok(gpsbuffer, ",");
//     if (! latp) return false;
//
//     // grab latitude direction
//     char *latdir = strtok(NULL, ",");
//     if (! latdir) return false;
//
//     // grab longitude
//     char *longp = strtok(NULL, ",");
//     if (! longp) return false;
//
//     // grab longitude direction
//     char *longdir = strtok(NULL, ",");
//     if (! longdir) return false;
//
//     // skip date & time
//     tok = strtok(NULL, ",");
//     tok = strtok(NULL, ",");
//
//    // only grab altitude if needed
//     if (altitude != NULL) {
//       // grab altitude
//       char *altp = strtok(NULL, ",");
//       if (! altp) return false;
//       *altitude = atof(altp);
//     }
//
//     // only grab speed if needed
//     if (speed_kph != NULL) {
//       // grab the speed in km/h
//       char *speedp = strtok(NULL, ",");
//       if (! speedp) return false;
//
//       *speed_kph = atof(speedp);
//     }
//
//     // only grab heading if needed
//     if (heading != NULL) {
//
//       // grab the speed in knots
//       char *coursep = strtok(NULL, ",");
//       if (! coursep) return false;
//
//       *heading = atof(coursep);
//     }
//
//     double latitude = atof(latp);
//     double longitude = atof(longp);
//
//     // convert latitude from minutes to decimal
//     float degrees = floor(latitude / 100);
//     double minutes = latitude - (100 * degrees);
//     minutes /= 60;
//     degrees += minutes;
//
//     // turn direction into + or -
//     if (latdir[0] == 'S') degrees *= -1;
//
//     *lat = degrees;
//
//     // convert longitude from minutes to decimal
//     degrees = floor(longitude / 100);
//     minutes = longitude - (100 * degrees);
//     minutes /= 60;
//     degrees += minutes;
//
//     // turn direction into + or -
//     if (longdir[0] == 'W') degrees *= -1;
//
//     *lon = degrees;
//
//   } else if (_type == FONA808_V2) {
//     // Parse 808 V2 response.  See table 2-3 from here for format:
//     // http://www.adafruit.com/datasheets/SIM800%20Series_GNSS_Application%20Note%20V1.00.pdf
//
//     // skip GPS run status
//     char *tok = strtok(gpsbuffer, ",");
//     if (! tok) return false;
//
//     // skip fix status
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // skip date
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // grab the latitude
//     char *latp = strtok(NULL, ",");
//     if (! latp) return false;
//
//     // grab longitude
//     char *longp = strtok(NULL, ",");
//     if (! longp) return false;
//
//     *lat = atof(latp);
//     *lon = atof(longp);
//
//     // only grab altitude if needed
//     if (altitude != NULL) {
//       // grab altitude
//       char *altp = strtok(NULL, ",");
//       if (! altp) return false;
//
//       *altitude = atof(altp);
//     }
//
//     // only grab speed if needed
//     if (speed_kph != NULL) {
//       // grab the speed in km/h
//       char *speedp = strtok(NULL, ",");
//       if (! speedp) return false;
//
//       *speed_kph = atof(speedp);
//     }
//
//     // only grab heading if needed
//     if (heading != NULL) {
//
//       // grab the speed in knots
//       char *coursep = strtok(NULL, ",");
//       if (! coursep) return false;
//
//       *heading = atof(coursep);
//     }
//   }
//   else {
//     // Parse 808 V1 response.
//
//     // skip mode
//     char *tok = strtok(gpsbuffer, ",");
//     if (! tok) return false;
//
//     // skip date
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // skip fix
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // grab the latitude
//     char *latp = strtok(NULL, ",");
//     if (! latp) return false;
//
//     // grab latitude direction
//     char *latdir = strtok(NULL, ",");
//     if (! latdir) return false;
//
//     // grab longitude
//     char *longp = strtok(NULL, ",");
//     if (! longp) return false;
//
//     // grab longitude direction
//     char *longdir = strtok(NULL, ",");
//     if (! longdir) return false;
//
//     double latitude = atof(latp);
//     double longitude = atof(longp);
//
//     // convert latitude from minutes to decimal
//     float degrees = floor(latitude / 100);
//     double minutes = latitude - (100 * degrees);
//     minutes /= 60;
//     degrees += minutes;
//
//     // turn direction into + or -
//     if (latdir[0] == 'S') degrees *= -1;
//
//     *lat = degrees;
//
//     // convert longitude from minutes to decimal
//     degrees = floor(longitude / 100);
//     minutes = longitude - (100 * degrees);
//     minutes /= 60;
//     degrees += minutes;
//
//     // turn direction into + or -
//     if (longdir[0] == 'W') degrees *= -1;
//
//     *lon = degrees;
//
//     // only grab speed if needed
//     if (speed_kph != NULL) {
//
//       // grab the speed in knots
//       char *speedp = strtok(NULL, ",");
//       if (! speedp) return false;
//
//       // convert to kph
//       *speed_kph = atof(speedp) * 1.852;
//
//     }
//
//     // only grab heading if needed
//     if (heading != NULL) {
//
//       // grab the speed in knots
//       char *coursep = strtok(NULL, ",");
//       if (! coursep) return false;
//
//       *heading = atof(coursep);
//
//     }
//
//     // no need to continue
//     if (altitude == NULL)
//       return true;
//
//     // we need at least a 3D fix for altitude
//     if (GPSstatus() < 3)
//       return false;
//
//     // grab the mode 0 gps csv from the sim808
//     res_len = getGPS(0, gpsbuffer, 120);
//
//     // make sure we have a response
//     if (res_len == 0)
//       return false;
//
//     // skip mode
//     tok = strtok(gpsbuffer, ",");
//     if (! tok) return false;
//
//     // skip lat
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // skip long
//     tok = strtok(NULL, ",");
//     if (! tok) return false;
//
//     // grab altitude
//     char *altp = strtok(NULL, ",");
//     if (! altp) return false;
//
//     *altitude = atof(altp);
//   }
//
//   return true;
//
// }

// boolean Adafruit_FONA::enableGPSNMEA(uint8_t i) {
//
//   char sendbuff[15] = "AT+CGPSOUT=000";
//   sendbuff[11] = (i / 100) + '0';
//   i %= 100;
//   sendbuff[12] = (i / 10) + '0';
//   i %= 10;
//   sendbuff[13] = i + '0';
//
//   if (_type == FONA808_V2) {
//     if (i)
//       return sendCheckReply(F("AT+CGNSTST=1"), ok_reply);
//     else
//       return sendCheckReply(F("AT+CGNSTST=0"), ok_reply);
//   } else {
//     return sendCheckReply(sendbuff, ok_reply, 2000);
//   }
// }


/********* GPRS **********************************************************/


// boolean Adafruit_FONA::enableGPRS(boolean onoff) {
//
//   if (onoff) {
//     // disconnect all sockets
//     sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000);
//
//     if (! sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))
//       return false;
//
//     // set bearer profile! connection type GPRS
//     if (! sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""),
// 			   ok_reply, 10000))
//       return false;
//
//     // set bearer profile access point name
//     if (apn) {
//       // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the configured APN value.
//       if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), apn, ok_reply, 10000))
//         return false;
//
//       // send AT+CSTT,"apn","user","pass"
//       flushInput();
//
//       mySerial->print(F("AT+CSTT=\""));
//       mySerial->print(apn);
//       if (apnusername) {
// 	mySerial->print("\",\"");
// 	mySerial->print(apnusername);
//       }
//       if (apnpassword) {
// 	mySerial->print("\",\"");
// 	mySerial->print(apnpassword);
//       }
//       mySerial->println("\"");
//
//       DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(F("AT+CSTT=\""));
//       DEBUG_PRINT(apn);
//
//       if (apnusername) {
// 	DEBUG_PRINT("\",\"");
// 	DEBUG_PRINT(apnusername);
//       }
//       if (apnpassword) {
// 	DEBUG_PRINT("\",\"");
// 	DEBUG_PRINT(apnpassword);
//       }
//       DEBUG_PRINTLN("\"");
//
//       if (! expectReply(ok_reply)) return false;
//
//       // set username/password
//       if (apnusername) {
//         // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the configured APN username.
//         if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), apnusername, ok_reply, 10000))
//           return false;
//       }
//       if (apnpassword) {
//         // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the configured APN password.
//         if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), apnpassword, ok_reply, 10000))
//           return false;
//       }
//     }
//
//     // open GPRS context
//     if (! sendCheckReply(F("AT+SAPBR=1,1"), ok_reply, 30000))
//       return false;
//
//     // bring up wireless connection
//     if (! sendCheckReply(F("AT+CIICR"), ok_reply, 10000))
//       return false;
//
//   } else {
//     // disconnect all sockets
//     if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000))
//       return false;
//
//     // close GPRS context
//     if (! sendCheckReply(F("AT+SAPBR=0,1"), ok_reply, 10000))
//       return false;
//
//     if (! sendCheckReply(F("AT+CGATT=0"), ok_reply, 10000))
//       return false;
//
//   }
//   return true;
// }

boolean Adafruit_FONA_3G::enableGPRS(boolean onoff) {

  if (onoff) {
    // disconnect all sockets
    //sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000);

    if (! sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))
      return false;


    // set bearer profile access point name
    if (! apn.empty()) {
      // Send command AT+CGSOCKCONT=1,"IP","<apn value>" where <apn value> is the configured APN name.
      if (! sendCheckReplyQuoted(F("AT+CGSOCKCONT=1,\"IP\","), apn, ok_reply, 10000))
        return false;

      // set username/password
      if (! apnusername.empty()) {
        std::string authstring = "AT+CGAUTH=1,1,\"" + apnusername + '\"';

      	if (! apnpassword.empty()) {
          authstring += ",\"" + apnpassword + "\"";
      	}

      	if (! sendCheckReply(authstring, ok_reply, 10000))
      	  return false;
      }
    }

    // connect in transparent
    if (! sendCheckReply(F("AT+CIPMODE=1"), ok_reply, 10000))
      return false;
    // open network

    getReply("AT+NETOPEN", static_cast<uint16_t>(5000));
    if(replybuffer == ok_reply){
      if(!expectReply("+NETOPEN: 0", 10000))
        return false;
    }
    else if (static_cast<std::string>(replybuffer) != "+NETOPEN: 0"){
        return false;
    }
    else {
      readOut(); // read OK
    }

    //
    // if (! sendCheckReply(F("AT+NETOPEN"), ok_reply, 10000))
    //   DEBUG_PRINTLN("OK expected but not get!");
    //   return false;
    //
    // // readline(10000); // status
    // readOut(5000, false);
    // if(static_cast<std::string>(replybuffer) != "+NETOPEN: 0"){
    //   DEBUG_PRINT("+NETOPEN:0 expected but not get! Got: "); DEBUG_PRINTLN(replybuffer);
    //   return false;
    // }
  } else {
    // close GPRS context
    if (! sendCheckReply(F("AT+NETCLOSE"), ok_reply, 10000))
      return false;
    readOut(); // eat 'OK'
  }

  return true;
}

// uint8_t Adafruit_FONA::GPRSstate(void) {
//   uint16_t state;
//
//   if (! sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state) )
//     return -1;
//
//   return state;
// }
//
// void Adafruit_FONA::setGPRSNetworkSettings(std::string apn,
//               std::string username, std::string password) {
//   this->apn = apn;
//   this->apnusername = username;
//   this->apnpassword = password;
// }

// boolean Adafruit_FONA::getGSMLoc(uint16_t *errorcode, char *buff, uint16_t maxlen) {
//
//   getReply(F("AT+CIPGSMLOC=1,1"), (uint16_t)10000);
//
//   if (! parseReply(F("+CIPGSMLOC: "), errorcode))
//     return false;
//
//   char *p = replybuffer+14;
//   uint16_t lentocopy = min(maxlen-1, strlen(p));
//   strncpy(buff, p, lentocopy+1);
//
//   readline(); // eat OK
//
//   return true;
// }

// boolean Adafruit_FONA::getGSMLoc(float *lat, float *lon) {
//
//   uint16_t returncode;
//   char gpsbuffer[120];
//
//   // make sure we could get a response
//   if (! getGSMLoc(&returncode, gpsbuffer, 120))
//     return false;
//
//   // make sure we have a valid return code
//   if (returncode != 0)
//     return false;
//
//   // +CIPGSMLOC: 0,-74.007729,40.730160,2015/10/15,19:24:55
//   // tokenize the gps buffer to locate the lat & long
//   char *longp = strtok(gpsbuffer, ",");
//   if (! longp) return false;
//
//   char *latp = strtok(NULL, ",");
//   if (! latp) return false;
//
//   *lat = atof(latp);
//   *lon = atof(longp);
//
//   return true;
//
// }
/********* TCP FUNCTIONS  ************************************/


// boolean Adafruit_FONA::TCPconnect(char *server, uint16_t port) {
//   flushInput();
//
//   // close all old connections
//   if (! sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000) ) return false;
//
//   // single connection at a time
//   if (! sendCheckReply(F("AT+CIPMUX=0"), ok_reply) ) return false;
//
//   // manually read data
//   if (! sendCheckReply(F("AT+CIPRXGET=1"), ok_reply) ) return false;
//
//
//   DEBUG_PRINT(F("AT+CIPSTART=\"TCP\",\""));
//   DEBUG_PRINT(server);
//   DEBUG_PRINT(F("\",\""));
//   DEBUG_PRINT(port);
//   DEBUG_PRINTLN(F("\""));
//
//
//   mySerial->print(F("AT+CIPSTART=\"TCP\",\""));
//   mySerial->print(server);
//   mySerial->print(F("\",\""));
//   mySerial->print(port);
//   mySerial->println(F("\""));
//
//   if (! expectReply(ok_reply)) return false;
//   if (! expectReply(F("CONNECT OK"))) return false;
//
//   // looks like it was a success (?)
//   return true;
// }

// boolean Adafruit_FONA::TCPclose(void) {
//   return sendCheckReply(F("AT+CIPCLOSE"), ok_reply);
// }

// boolean Adafruit_FONA::TCPconnected(void) {
//   if (! sendCheckReply(F("AT+CIPSTATUS"), ok_reply, 100) ) return false;
//   readline(100);
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//   return (strcmp(replybuffer, "STATE: CONNECT OK") == 0);
// }

// boolean Adafruit_FONA::TCPsend(char *packet, uint8_t len) {
//
//   DEBUG_PRINT(F("AT+CIPSEND="));
//   DEBUG_PRINTLN(len);
// #ifdef ADAFRUIT_FONA_DEBUG
//   for (uint16_t i=0; i<len; i++) {
//   DEBUG_PRINT(F(" 0x"));
//   DEBUG_PRINT(packet[i], HEX);
//   }
// #endif
//   DEBUG_PRINTLN();
//
//
//   mySerial->print(F("AT+CIPSEND="));
//   mySerial->println(len);
//   readline();
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//   if (replybuffer[0] != '>') return false;
//
//   mySerial->write(packet, len);
//   readline(3000); // wait up to 3 seconds to send the data
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//
//   return (strcmp(replybuffer, "SEND OK") == 0);
// }

// uint16_t Adafruit_FONA::TCPavailable(void) {
//   uint16_t avail;
//
//   if (! sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0) ) return false;
//
//
//   DEBUG_PRINT (avail); DEBUG_PRINTLN(F(" bytes available"));
//
//
//   return avail;
// }


// uint16_t Adafruit_FONA::TCPread(uint8_t *buff, uint8_t len) {
//   uint16_t avail;
//
//   mySerial->print(F("AT+CIPRXGET=2,"));
//   mySerial->println(len);
//   readline();
//   if (! parseReply(F("+CIPRXGET: 2,"), &avail, ',', 0)) return false;
//
//   readRaw(avail);
//
// #ifdef ADAFRUIT_FONA_DEBUG
//   DEBUG_PRINT (avail); DEBUG_PRINTLN(F(" bytes read"));
//   for (uint8_t i=0;i<avail;i++) {
//   DEBUG_PRINT(F(" 0x")); DEBUG_PRINT(replybuffer[i], HEX);
//   }
//   DEBUG_PRINTLN();
// #endif
//
//   memcpy(buff, replybuffer, avail);
//
//   return avail;
// }



/********* HTTP LOW LEVEL FUNCTIONS  ************************************/

// boolean Adafruit_FONA::HTTP_init() {
//   return sendCheckReply(F("AT+HTTPINIT"), ok_reply);
// }
//
// boolean Adafruit_FONA::HTTP_term() {
//   return sendCheckReply(F("AT+HTTPTERM"), ok_reply);
// }
//
// void Adafruit_FONA::HTTP_para_start(std::string parameter,
//                                     boolean quoted) {
//   flushInput();
//
//
//   DEBUG_PRINT(F("\t---> "));
//   DEBUG_PRINT(F("AT+HTTPPARA=\""));
//   DEBUG_PRINT(parameter);
//   DEBUG_PRINTLN('"');
//
//
//   mySerial->print(F("AT+HTTPPARA=\""));
//   mySerial->print(parameter);
//   if (quoted)
//     mySerial->print(F("\",\""));
//   else
//     mySerial->print(F("\","));
// }
//
// boolean Adafruit_FONA::HTTP_para_end(boolean quoted) {
//   if (quoted)
//     mySerial->println('"');
//   else
//     mySerial->println();
//
//   return expectReply(ok_reply);
// }
//
// boolean Adafruit_FONA::HTTP_para(std::string parameter,
//                                  const char *value) {
//   HTTP_para_start(parameter, true);
//   mySerial->print(value);
//   return HTTP_para_end(true);
// }
//
// boolean Adafruit_FONA::HTTP_para(std::string parameter,
//                                  std::string value) {
//   HTTP_para_start(parameter, true);
//   mySerial->print(value);
//   return HTTP_para_end(true);
// }
//
// boolean Adafruit_FONA::HTTP_para(std::string parameter,
//                                  int32_t value) {
//   HTTP_para_start(parameter, false);
//   mySerial->print(value);
//   return HTTP_para_end(false);
// }
//
// boolean Adafruit_FONA::HTTP_data(uint32_t size, uint32_t maxTime) {
//   flushInput();
//
//
//   DEBUG_PRINT(F("\t---> "));
//   DEBUG_PRINT(F("AT+HTTPDATA="));
//   DEBUG_PRINT(size);
//   DEBUG_PRINT(',');
//   DEBUG_PRINTLN(maxTime);
//
//
//   mySerial->print(F("AT+HTTPDATA="));
//   mySerial->print(size);
//   mySerial->print(",");
//   mySerial->println(maxTime);
//
//   return expectReply(F("DOWNLOAD"));
// }
//
// boolean Adafruit_FONA::HTTP_action(uint8_t method, uint16_t *status,
//                                    uint16_t *datalen, int32_t timeout) {
//   // Send request.
//   if (! sendCheckReply(F("AT+HTTPACTION="), method, ok_reply))
//     return false;
//
//   // Parse response status and size.
//   readline(timeout);
//   if (! parseReply(F("+HTTPACTION:"), status, ',', 1))
//     return false;
//   if (! parseReply(F("+HTTPACTION:"), datalen, ',', 2))
//     return false;
//
//   return true;
// }
//
// boolean Adafruit_FONA::HTTP_readall(uint16_t *datalen) {
//   getReply(F("AT+HTTPREAD"));
//   if (! parseReply(F("+HTTPREAD:"), datalen, ',', 0))
//     return false;
//
//   return true;
// }
//
// boolean Adafruit_FONA::HTTP_ssl(boolean onoff) {
//   return sendCheckReply(F("AT+HTTPSSL="), onoff ? 1 : 0, ok_reply);
// }

/********* HTTP HIGH LEVEL FUNCTIONS ***************************/

// boolean Adafruit_FONA::HTTP_GET_start(char *url,
//               uint16_t *status, uint16_t *datalen){
//   if (! HTTP_setup(url))
//     return false;
//
//   // HTTP GET
//   if (! HTTP_action(FONA_HTTP_GET, status, datalen, 30000))
//     return false;
//
//   DEBUG_PRINT(F("Status: ")); DEBUG_PRINTLN(*status);
//   DEBUG_PRINT(F("Len: ")); DEBUG_PRINTLN(*datalen);
//
//   // HTTP response data
//   if (! HTTP_readall(datalen))
//     return false;
//
//   return true;
// }

// start HTTPS session
boolean Adafruit_FONA_3G::HTTPS_start(std::string host, boolean https){
  getReply("AT+CHTTPSSTART");
  std::string port = https ? "443" : "80";
  char HTTPSon     = https ? '2' : '1';
  std::string HTTPSOpenCmd = "AT+CHTTPSOPSE=\"" + host + "\"," + port + ',' + HTTPSon;

  uint16_t waitForReply = 5000;

  return sendCheckReply(HTTPSOpenCmd, ok_reply , waitForReply);
}

boolean Adafruit_FONA_3G::HTTPS_GET(std::string host, std::string get_uri, std::string extra_line){
  // send get request
  std::string getRequest =
  static_cast<std::string>("GET ") + get_uri + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  extra_line +
  // "User-Agent: Sensor Hub\r\n" +
  // "Content-Length: 0\r\n"
  "\r\n\r\r";
  if(! HTTPS_request(getRequest)){
    DEBUG_PRINTLN("Get Request not sucessful.");
    return false;
  }
  return true;
}

boolean Adafruit_FONA_3G::HTTPS_POST(std::string host){

  std::string stringToPost = "I want this string to be posted";

  std::string postRequest =
  static_cast<std::string>("POST /post HTTP/1.1\r\n") +
  "Host: " + host + "\r\n" +
  "Content-Length: " + std::to_string(stringToPost.size()) + "\r\n" +
  "\r\n" +
  stringToPost +
  "\r\r";

  if(! HTTPS_request(postRequest)){
    DEBUG_PRINTLN("Post Request not sucessful.");
    return false;
  }
  return true;
}

boolean Adafruit_FONA_3G::HTTPS_PUT(std::string host, std::string put_uri, const char *payload, const size_t payload_size, std::string auth_line){

  // Fill out request header
  std::string putRequest =
  static_cast<std::string>("PUT ") + put_uri + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  auth_line +
  "Content-Type: text/plain\r\n" +
  "Content-Length: " + std::to_string(payload_size) + "\r\n" +
  "\r\n";


  if(! HTTPS_request(putRequest, payload, payload_size)){
    DEBUG_PRINTLN("PUT Request not sucessful.");
    return false;
  }

  return true;
}


boolean Adafruit_FONA_3G::HTTPS_request(std::string request, const char *filebuffer, const size_t file_size){

  int32_t request_size = (filebuffer == nullptr) ? request.size() : (request.size() + file_size);
  DEBUG_PRINT("request_size: "); DEBUG_PRINTLN(request_size);
  DEBUG_PRINT("sizeof(filebuffer): "); DEBUG_PRINTLN(sizeof(filebuffer));

  // Sennd request with payload (if existing)
  std::chrono::steady_clock::time_point time_before_sending = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point time_after_sending;
  std::chrono::steady_clock::time_point time_after_RECV;

  if(sendCheckReply("AT+CHTTPSSEND=", request_size, ">")){
    mySerial->writeStr(request);
    if(filebuffer != nullptr){
      mySerial->write(filebuffer, file_size);
      mySerial->writeStr("\r\r");
    }
    time_after_sending = std::chrono::steady_clock::now();
    DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(request);
  }
  else {return false;}


  // while (available()) {
  //   // Serial.write(read());
  //   std::cout << static_cast<char>(read());
  // }
  // delay(5000);
  // receive answer for the request

  // read the answer from after request

  // the previous command shall return an "OK" but might issue "+CHTTPS: RECV EVENT" before or after that 
  readOut(5000,false);

  boolean RECV_EVENT = false;
  if (replybuffer == ok_reply){
    RECV_EVENT = expectReply("+CHTTPS: RECV EVENT", 5000);
  }
  else if (static_cast<std::string>(replybuffer) == "+CHTTPS: RECV EVENT"){
    RECV_EVENT = true;
  }
  time_after_RECV = std::chrono::steady_clock::now();
  std::cout << "Time to send request = " << std::chrono::duration_cast<std::chrono::microseconds>(time_after_RECV - time_before_sending).count()/1000.0 << "ms." <<std::endl;
  std::cout << "Time to wait for RECV_EVENT = " << std::chrono::duration_cast<std::chrono::microseconds>(time_after_RECV - time_after_sending).count()/1000.0 << "ms." <<std::endl;



  if(RECV_EVENT){
    uint16_t waitForReply = 5000;
    uint16_t answerLen = 0;
    if(sendParseReply("AT+CHTTPSRECV?", "+CHTTPSRECV: ", &answerLen, ',', 1)){
      waitForReply = 1500;
      // readOut(waitForReply); // TODO: not sure why this is necessary

      if(answerLen && sendCheckReply("AT+CHTTPSRECV=", static_cast<int32_t>(answerLen), ok_reply, waitForReply)){
        readline();
        DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

        uint16_t lenLine = 0;
        uint16_t bytesRead = 0;
        uint16_t status_code = 0;
        std::string content_type;
        uint16_t header_length = answerLen;
        uint16_t content_length = 0;
        std::string const CONTENT_TYPE = "Content-Type: ";
        std::string const CONTENT_LENGTH = "Content-Length: ";
        size_t pos;

        if (filebuffer == nullptr) DEBUG_PRINTLN("Header\n------");

        while(available()){

          // we add 2 for the \n\n that doesn't get returned by readline
          // Header Part
          if(bytesRead == header_length-2){
            bytesRead += 2;
            readRaw(2); //read the empty line
            if (filebuffer == nullptr) DEBUG_PRINTLN("\nContent\n-------");
            replybuffer[0] = 0;
            // DEBUG_PRINT("bytesRead: "); DEBUG_PRINT(bytesRead); DEBUG_PRINT(", lenLine: "); DEBUG_PRINTLN(lenLine);
          }
          else if(bytesRead < header_length){
            lenLine = readline();
            if(bytesRead == 0){
              parseReply("HTTP/1.1 ", &status_code, ' ', 0);
            }
            bytesRead += lenLine+2;
            if((pos = static_cast<std::string>(replybuffer).find(CONTENT_TYPE)) != std::string::npos){
              pos += CONTENT_TYPE.size();
              content_type = static_cast<std::string>(&(replybuffer[pos]));
            }
            else if((pos = static_cast<std::string>(replybuffer).find(CONTENT_LENGTH)) != std::string::npos){
              pos += CONTENT_LENGTH.size();
              content_length = atoi(&(replybuffer[pos]));
              header_length = answerLen - content_length;
            }
          }
          // after the header there is one empty line which is ignored by readline
          // Content parameter
          else if(bytesRead == header_length){
            //TODO: process content
            uint16_t idx = 0;
            idx = readRaw(content_length);
            replybuffer[idx+1] = 0;
            bytesRead += idx+1;
          }
          else{
            // DEBUG_PRINT("bytesRead: ");DEBUG_PRINT(bytesRead); DEBUG_PRINT(", lenLine: "); DEBUG_PRINTLN(lenLine);
            break;
          }
          if (filebuffer == nullptr) DEBUG_PRINTLN(replybuffer);
        }
        DEBUG_PRINT("status_code: "); DEBUG_PRINTLN(status_code);
        DEBUG_PRINT("content_type: "); DEBUG_PRINTLN(content_type);
        DEBUG_PRINT("content_length: "); DEBUG_PRINTLN(content_length);
      }
    }
  }
  else {
    DEBUG_PRINTLN("NO +CHTTPS: RECV EVENT was received.");
    DEBUG_PRINT("replybuffer: "); DEBUG_PRINTLN(replybuffer);
  }

  return true;
}

// end HTTPS session
boolean Adafruit_FONA_3G::HTTPS_end(){
  uint16_t waitForReply = FONA_DEFAULT_TIMEOUT_MS;
  getReply("AT+CHTTPSCLSE");

  readOut(waitForReply);

  getReply("AT+CHTTPSSTOP");

  readOut(waitForReply);

  return true;
}


/*
boolean Adafruit_FONA_3G::HTTP_GET_start(char *ipaddr, char *path, uint16_t port
				      uint16_t *status, uint16_t *datalen){
  char send[100] = "AT+CHTTPACT=\"";
  char *sendp = send + strlen(send);
  memset(sendp, 0, 100 - strlen(send));

  strcpy(sendp, ipaddr);
  sendp+=strlen(ipaddr);
  sendp[0] = '\"';
  sendp++;
  sendp[0] = ',';
  itoa(sendp, port);
  getReply(send, 500);

  return;

  if (! HTTP_setup(url))

    return false;

  // HTTP GET
  if (! HTTP_action(FONA_HTTP_GET, status, datalen))
    return false;

  DEBUG_PRINT("Status: "); DEBUG_PRINTLN(*status);
  DEBUG_PRINT("Len: "); DEBUG_PRINTLN(*datalen);

  // HTTP response data
  if (! HTTP_readall(datalen))
    return false;

  return true;
}
*/

// void Adafruit_FONA::HTTP_GET_end(void) {
//   HTTP_term();
// }
//
// boolean Adafruit_FONA::HTTP_POST_start(char *url,
//               std::string contenttype,
//               const uint8_t *postdata, uint16_t postdatalen,
//               uint16_t *status, uint16_t *datalen){
//   if (! HTTP_setup(url))
//     return false;
//
//   if (! HTTP_para(F("CONTENT"), contenttype)) {
//     return false;
//   }
//
//   // HTTP POST data
//   if (! HTTP_data(postdatalen, 10000))
//     return false;
//   mySerial->write(postdata, postdatalen);
//   if (! expectReply(ok_reply))
//     return false;
//
//   // HTTP POST
//   if (! HTTP_action(FONA_HTTP_POST, status, datalen))
//     return false;
//
//   DEBUG_PRINT(F("Status: ")); DEBUG_PRINTLN(*status);
//   DEBUG_PRINT(F("Len: ")); DEBUG_PRINTLN(*datalen);
//
//   // HTTP response data
//   if (! HTTP_readall(datalen))
//     return false;
//
//   return true;
// }
//
// void Adafruit_FONA::HTTP_POST_end(void) {
//   HTTP_term();
// }
//
// void Adafruit_FONA::setUserAgent(std::string useragent) {
//   this->useragent = useragent;
// }
//
// void Adafruit_FONA::setHTTPSRedirect(boolean onoff) {
//   httpsredirect = onoff;
// }

/********* HTTP HELPERS ****************************************/

// boolean Adafruit_FONA::HTTP_setup(char *url) {
//   // Handle any pending
//   HTTP_term();
//
//   // Initialize and set parameters
//   if (! HTTP_init())
//     return false;
//   if (! HTTP_para(F("CID"), 1))
//     return false;
//   if (! HTTP_para(F("UA"), useragent))
//     return false;
//   if (! HTTP_para(F("URL"), url))
//     return false;
//
//   // HTTPS redirect
//   if (httpsredirect) {
//     if (! HTTP_para(F("REDIR"),1))
//       return false;
//
//     if (! HTTP_ssl(true))
//       return false;
//   }
//
//   return true;
// }

/********* HELPERS *********************************************/

boolean Adafruit_FONA::expectReply(std::string reply, uint16_t timeout) {
  readline(timeout);

  DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return (replybuffer == reply);
}

boolean Adafruit_FONA::expectReplies(std::string reply1, std::string reply2, uint16_t timeout) {
  readline(timeout);

  DEBUG_PRINT(F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return (replybuffer == reply1 || replybuffer == reply2);
}

/********* LOW LEVEL *******************************************/

inline boolean Adafruit_FONA::available(void) {
  // original arduino function returned number of bytes to read (int)
  return mySerial->dataAvailable();
}

inline size_t Adafruit_FONA::write(uint8_t x) {
  const char c = static_cast<const char>(x);
  return mySerial->write(&c,1);
}

inline int Adafruit_FONA::read(void) {
  // returns first byte of stream or -1 if no data available (int)
  // return mySerial->read();

  char c;
  int result;
  result = mySerial->read(&c, 1);
  if(result == -1){
    return result;
  } else {
    return static_cast<int>(c);
  }
}

// inline int Adafruit_FONA::peek(void) {
//   return mySerial->peek();
// }

inline void Adafruit_FONA::flush() {
  mySerial->flush();
}

void Adafruit_FONA::flushInput() {
    // Read all available serial input to flush pending data.
    uint16_t timeoutloop = 0;
    while (timeoutloop++ < 40) {
        while(available()) {
            read();
            timeoutloop = 0;  // If char was received reset the timer
        }
        delay(1);
    }
}

uint16_t Adafruit_FONA::readRaw(uint16_t b) {
  uint16_t idx = 0;

  // while (b && (idx < sizeof(replybuffer)-1)) {
  //   if (mySerial->dataAvailable()) {
  //      mySerial->read(&(replybuffer[idx]),1);
  //     idx++;
  //     b--;
  //   }
  // }

  idx = (b < (sizeof(replybuffer) -2)) ? static_cast<int>(b) : sizeof(replybuffer)-2;
  // DEBUG_PRINT("idx: ");DEBUG_PRINTLN(idx);
  idx = mySerial->read(replybuffer,idx);
  replybuffer[idx] = 0;

  // DEBUG_PRINT("idx: ");DEBUG_PRINT(idx);DEBUG_PRINT(", b: ");DEBUG_PRINT(b);DEBUG_PRINT(", sizeofbuff-1: "); DEBUG_PRINTLN((sizeof(replybuffer)-1));

  return idx;
}

uint8_t Adafruit_FONA::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      //DEBUG_PRINTLN(F("SPACE"));
      break;
    }

    while(mySerial->dataAvailable()) {
      // char c =  mySerial->read();
      char c;
      mySerial->read(&c, 1);
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
      //DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
      replyidx++;
    }

    if (timeout == 0) {
      //DEBUG_PRINTLN(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

uint16_t Adafruit_FONA_3G::readOut(uint16_t timeout, boolean multiline){
  uint16_t str_len = 0;
  uint8_t line_len = 0;
  while((line_len = readline(timeout))){
    str_len += line_len;
    DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
    if(! multiline) break;
  }
  return str_len;
}

// uint8_t Adafruit_FONA::getReply(char *send, uint16_t timeout) {
//   flushInput();
//
//
//   DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);
//
//
//   mySerial->println(send);
//
//   uint8_t l = readline(timeout);
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//   return l;
// }

uint8_t Adafruit_FONA::getReply(std::string send, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINTLN(send);


  mySerial->writeStr(send);
  mySerial->writeStr("\r\n");

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// // Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
// uint8_t Adafruit_FONA::getReply(std::string prefix, char *suffix, uint16_t timeout) {
//   flushInput();
//
//
//   DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix);
//
//
//   mySerial->print(prefix);
//   mySerial->println(suffix);
//
//   uint8_t l = readline(timeout);
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//   return l;
// }

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Adafruit_FONA::getReply(std::string prefix, int32_t suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix); DEBUG_PRINTLN(suffix);


  // mySerial->print(prefix);
  // mySerial->println(suffix, DEC);
  mySerial->writeStr(prefix);
  mySerial->writeStr(std::to_string(suffix));
  mySerial->writeStr("\r\n");

  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// // Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
// uint8_t Adafruit_FONA::getReply(std::string prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
//   flushInput();
//
//
//   DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix);
//   DEBUG_PRINT(suffix1, DEC); DEBUG_PRINT(','); DEBUG_PRINTLN(suffix2, DEC);
//
//
//   mySerial->print(prefix);
//   mySerial->print(suffix1);
//   mySerial->print(',');
//   mySerial->println(suffix2, DEC);
//
//   uint8_t l = readline(timeout);
//
//   DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);
//
//   return l;
// }

// Send prefix, ", suffix, ", and newline. Return response (and also set replybuffer with response).
uint8_t Adafruit_FONA::getReplyQuoted(std::string prefix, std::string suffix, uint16_t timeout) {
  flushInput();


  DEBUG_PRINT(F("\t---> ")); DEBUG_PRINT(prefix);
  DEBUG_PRINT('"'); DEBUG_PRINT(suffix); DEBUG_PRINTLN('"');


  // mySerial->print(prefix);
  // mySerial->print('"');
  // mySerial->print(suffix);
  // mySerial->println('"');
  mySerial->writeStr(prefix);
  mySerial->writeStr("\"");
  mySerial->writeStr(suffix);
  mySerial->writeStr("\"");
  mySerial->writeStr("\r\n");
  // mySerial->writeSTR(prefix + "\"" + suffix + "\"" + "\r\n");


  uint8_t l = readline(timeout);

  DEBUG_PRINT (F("\t<--- ")); DEBUG_PRINTLN(replybuffer);

  return l;
}

// boolean Adafruit_FONA::sendCheckReply(char *send, char *reply, uint16_t timeout) {
//   if (! getReply(send, timeout) )
// 	  return false;
// /*
//   for (uint8_t i=0; i<strlen(replybuffer); i++) {
//   DEBUG_PRINT(replybuffer[i], HEX); DEBUG_PRINT(" ");
//   }
//   DEBUG_PRINTLN();
//   for (uint8_t i=0; i<strlen(reply); i++) {
//     DEBUG_PRINT(reply[i], HEX); DEBUG_PRINT(" ");
//   }
//   DEBUG_PRINTLN();
//   */
//   return (strcmp(replybuffer, reply) == 0);
// }

boolean Adafruit_FONA::sendCheckReply(std::string send, std::string reply, uint16_t timeout) {
	if (! getReply(send, timeout) )
		return false;

  return (replybuffer == reply);
}

// boolean Adafruit_FONA::sendCheckReply(char* send, std::string reply, uint16_t timeout) {
//   if (! getReply(send, timeout) )
// 	  return false;
//   return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
// }


// // Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
// boolean Adafruit_FONA::sendCheckReply(std::string prefix, char *suffix, std::string reply, uint16_t timeout) {
//   getReply(prefix, suffix, timeout);
//   return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
// }

// Send prefix, suffix, and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReply(std::string prefix, int32_t suffix, std::string reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (replybuffer == reply);
}

// // Send prefix, suffix, suffix2, and newline.  Verify FONA response matches reply parameter.
// boolean Adafruit_FONA::sendCheckReply(std::string prefix, int32_t suffix1, int32_t suffix2, std::string reply, uint16_t timeout) {
//   getReply(prefix, suffix1, suffix2, timeout);
//   return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
// }

// Send prefix, ", suffix, ", and newline.  Verify FONA response matches reply parameter.
boolean Adafruit_FONA::sendCheckReplyQuoted(std::string prefix, std::string suffix, std::string reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  // return (prog_char_strcmp(replybuffer, (prog_char*)reply) == 0);
  return(static_cast<std::string>(replybuffer) == reply);
}


boolean Adafruit_FONA::parseReply(std::string toreply,
          uint16_t *v, char divider, uint8_t index) {
  // char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  size_t pos = static_cast<std::string>(replybuffer).find(toreply);
  if (pos == std::string::npos) return false;

  // p+=prog_char_strlen((prog_char*)toreply);
  pos += toreply.size();
  //DEBUG_PRINTLN(pos);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    // p = strchr(p, divider);
    pos = static_cast<std::string>(replybuffer).find(divider, pos);
    if (pos == std::string::npos) return false;
    pos++;
    //DEBUG_PRINTLN(p);

  }
  *v = atoi(&(replybuffer[pos]));

  return true;
}

// boolean Adafruit_FONA::parseReply(std::string toreply,
//           char *v, char divider, uint8_t index) {
//   uint8_t i=0;
//   char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
//   if (p == 0) return false;
//   p+=prog_char_strlen((prog_char*)toreply);
//
//   for (i=0; i<index;i++) {
//     // increment dividers
//     p = strchr(p, divider);
//     if (!p) return false;
//     p++;
//   }
//
//   for(i=0; i<strlen(p);i++) {
//     if(p[i] == divider)
//       break;
//     v[i] = p[i];
//   }
//
//   v[i] = '\0';
//
//   return true;
// }

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean Adafruit_FONA::parseReplyQuoted(std::string toreply,
          char *v, int maxlen, char divider, uint8_t index) {
  uint8_t i=0, j;
  // Verify response starts with toreply.
  // char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);
  // if (p == 0) return false;
  size_t pos = static_cast<std::string>(replybuffer).find(toreply);
  if (pos == std::string::npos) return false;

  // p+=prog_char_strlen((prog_char*)toreply);
  pos += toreply.size();
  // Find location of desired response field.
  for (i=0; i<index;i++) {
    // increment dividers
    // p = strchr(p, divider);
    // if (!p) return false;
    // p++;
    pos = static_cast<std::string>(replybuffer).find(divider, pos);
    if (pos == std::string::npos) return false;
    pos++;
  }

  // Copy characters from response field into result string.
  for(i=0, j=0; j<maxlen && i<strlen(&(replybuffer[pos])); ++i) {
    // Stop if a divier is found.
    // if(p[i] == divider)
    if(replybuffer[pos+i] == divider)
      break;
    // Skip any quotation marks.
    // else if(p[i] == '"')
    else if(replybuffer[pos+i] == '"')
      continue;
    v[j++] = replybuffer[pos+i];
  }

  // Add a null terminator if result string buffer was not filled.
  if (j < maxlen)
    v[j] = '\0';

  return true;
}

boolean Adafruit_FONA::sendParseReply(std::string tosend,
				      std::string toreply,
				      uint16_t *v, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, v, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}


// needed for CBC and others
// originally 3G
boolean Adafruit_FONA::sendParseReply(std::string tosend,
				      std::string toreply,
				      float *f, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, f, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}

// originally 3G
boolean Adafruit_FONA::parseReply(std::string toreply,
          float *f, char divider, uint8_t index) {

  // char *p = prog_char_strstr(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  size_t pos = static_cast<std::string>(replybuffer).find(toreply);
  if (pos == std::string::npos) return false;

  // p+=prog_char_strlen((prog_char*)toreply);
  pos += toreply.size();
  //DEBUG_PRINTLN(pos);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    // p = strchr(p, divider);
    pos = static_cast<std::string>(replybuffer).find(divider, pos);
    if (pos == std::string::npos) return false;
    pos++;
    //DEBUG_PRINTLN(p);

  }
  *f = atof(&(replybuffer[pos]));

  return true;
}

// send raw AT commands to FONA3G
boolean Adafruit_FONA_3G::execCommand(std::string tosend){
  return getReply(tosend);
}

void delay(int ms){
  timespec sleepTime;
  sleepTime.tv_sec = ms/1000L;
  sleepTime.tv_nsec = (ms % 1000L)*1000000L;
  nanosleep(&sleepTime, NULL);
}
