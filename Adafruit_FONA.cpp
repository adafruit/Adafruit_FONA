/*!
 * @file Adafruit_FONA.cpp
 *
 * @mainpage Adafruit FONA
 *
 * @section intro_sec Introduction
 *
 * This is a library for our Adafruit FONA Cellular Module
 *
 * Designed specifically to work with the Adafruit FONA
 * ----> http://www.adafruit.com/products/1946
 * ----> http://www.adafruit.com/products/1963
 *
 * These displays use TTL Serial to communicate, 2 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 *
 */
// next line per
// http://postwarrior.com/arduino-ethershield-error-prog_char-does-not-name-a-type/

#include "Adafruit_FONA.h"

#if defined(ESP8266)
// ESP8266 doesn't have the min and max functions natively available like
// AVR libc seems to provide.  Include the STL algorithm library to get these.
// Unfortunately algorithm isn't available in AVR libc so this is ESP8266
// specific (and likely needed for ARM or other platforms, but they lack
// software serial and are currently incompatible with the FONA library).
#include <algorithm>
using namespace std;
#endif
/**
 * @brief Construct a new Adafruit_FONA object
 *
 * @param rst The reset pin
 */
Adafruit_FONA::Adafruit_FONA(int8_t rst) {
  _rstpin = rst;

  apn = F("FONAnet");
  apnusername = 0;
  apnpassword = 0;
  mySerial = 0;
  httpsredirect = false;
  useragent = F("FONA");
  ok_reply = F("OK");
}

/**
 * @brief Get the module type
 *
 * @return uint8_t The module type
 */
uint8_t Adafruit_FONA::type(void) { return _type; }
/**
 * @brief Connect to the cell module
 *
 * @param port the serial connection to use to connect
 * @return bool true on success, false if a connection cannot be made
 */
bool Adafruit_FONA::begin(Stream &port) {
  mySerial = &port;

  pinMode(_rstpin, OUTPUT);
  digitalWrite(_rstpin, HIGH);
  delay(10);
  digitalWrite(_rstpin, LOW);
  delay(100);
  digitalWrite(_rstpin, HIGH);

  DEBUG_PRINTLN(F("Attempting to open comm with ATs"));
  // give 7 seconds to reboot
  int16_t timeout = 7000;

  while (timeout > 0) {
    while (mySerial->available())
      mySerial->read();
    if (sendCheckReply(F("AT"), ok_reply))
      break;
    while (mySerial->available())
      mySerial->read();
    if (sendCheckReply(F("AT"), F("AT")))
      break;
    delay(500);
    timeout -= 500;
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

  if (!sendCheckReply(F("ATE0"), ok_reply)) {
    return false;
  }

  // turn on hangupitude
  sendCheckReply(F("AT+CVHU=0"), ok_reply);

  delay(100);
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINTLN("ATI");

  mySerial->println("ATI");
  readline(500, true);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  if (prog_char_strstr(replybuffer, (prog_char *)F("SIM808 R14")) != 0) {
    _type = FONA808_V2;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM808 R13")) != 0) {
    _type = FONA808_V1;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIM800 R13")) != 0) {
    _type = FONA800L;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIMCOM_SIM5320A")) !=
             0) {
    _type = FONA3G_A;
  } else if (prog_char_strstr(replybuffer, (prog_char *)F("SIMCOM_SIM5320E")) !=
             0) {
    _type = FONA3G_E;
  }

  if (_type == FONA800L) {
    // determine if L or H

    DEBUG_PRINT(F("\t---> "));
    DEBUG_PRINTLN("AT+GMM");

    mySerial->println("AT+GMM");
    readline(500, true);

    DEBUG_PRINT(F("\t<--- "));
    DEBUG_PRINTLN(replybuffer);

    if (prog_char_strstr(replybuffer, (prog_char *)F("SIM800H")) != 0) {
      _type = FONA800H;
    }
  }

#if defined(FONA_PREF_SMS_STORAGE)
  sendCheckReply(F("AT+CPMS=" FONA_PREF_SMS_STORAGE "," FONA_PREF_SMS_STORAGE
                   "," FONA_PREF_SMS_STORAGE),
                 ok_reply);
#endif

  return true;
}

/********* Serial port ********************************************/

/**
 * @brief  Set the baud rate that the module will use
 *
 * @param baud The new baud rate to set
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setBaudrate(uint16_t baud) {
  return sendCheckReply(F("AT+IPREX="), baud, ok_reply);
}

/********* Real Time Clock ********************************************/
// This RTC setup isn't fully operational
// see https://forums.adafruit.com/viewtopic.php?f=19&t=58002&p=294235#p294235
/**
 * @brief Read the Real Time Clock
 *
 * **NOTE** Currently this function **will only fill the 'year' variable**
 *
 * @param year Pointer to a uint8_t to be set with year data
 * @param month Pointer to a uint8_t to be set with month data **NOT WORKING**
 * @param day Pointer to a uint8_t to be set with day data **NOT WORKING**
 * @param hr Pointer to a uint8_t to be set with hour data **NOT WORKING**
 * @param min Pointer to a uint8_t to be set with minute data **NOT WORKING**
 * @param sec Pointer to a uint8_t to be set with year data **NOT WORKING**
 * @return true: success, false: failure
 */
bool Adafruit_FONA::readRTC(uint8_t *year, uint8_t *month, uint8_t *day,
                            uint8_t *hr, uint8_t *min, uint8_t *sec) {
  uint16_t v;
  sendParseReply(F("AT+CCLK?"), F("+CCLK: "), &v, '/', 0);
  // TODO: https://github.com/adafruit/Adafruit_FONA/issues/111
  *year = v;

  // avoid non-used warning
#if 0
  (void *)month;
  (void *)day;
  (void *)hr;
  (void *)min;
  (void *)sec;
#else
  month = month;
  day = day;
  hr = hr;
  min = min;
  sec = sec;
#endif

  DEBUG_PRINTLN(*year);
  return true;
}

/**
 * @brief Enable the Real Time Clock
 *
 * @param mode  1: Enable 0: Disable
 * @return true: Success , false: failure
 */
bool Adafruit_FONA::enableRTC(uint8_t mode) {
  if (!sendCheckReply(F("AT+CLTS="), mode, ok_reply))
    return false;
  return sendCheckReply(F("AT&W"), ok_reply);
}

/********* BATTERY & ADC ********************************************/

/**
 * @brief Get the current batery voltage
 *
 * @param v battery value pointer to be filled with the current value in mV
 * (uint16_t)
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getBattVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
}

/* returns value in mV (uint16_t) */
/**
 * @brief Get the current batery voltage
 *
 * @param v battery voltage pointer to be filled with the current value in mV
 * (uint16_t)
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::getBattVoltage(uint16_t *v) {
  float f;
  bool b = sendParseReply(F("AT+CBC"), F("+CBC: "), &f, ',', 2);
  *v = f * 1000;
  return b;
}

/*  */
/**
 * @brief Get the percentage charge of battery as reported by sim800
 *
 * @param p Ponter to a uint16_t to set with the battery change as a percentage
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getBattPercent(uint16_t *p) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}
/**
 * @brief Get the current ADC voltage
 *
 * @param v Ponter to a uint16_t to set with the ADC voltage
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
}

/********* SIM ***********************************************************/

/**
 * @brief Unlock the sim with a provided PIN
 *
 * @param pin Pointer to a buffer with the PIN to use to unlock the SIM
 * @return uint8_t
 */
uint8_t Adafruit_FONA::unlockSIM(char *pin) {
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = '\0';

  return sendCheckReply(sendbuff, ok_reply);
}

/**
 * @brief Get the SIM CCID
 *
 * @param ccid Pointer to  a buffer to be filled with the CCID
 * @return uint8_t
 */
uint8_t Adafruit_FONA::getSIMCCID(char *ccid) {
  getReply(F("AT+CCID"));
  // up to 28 chars for reply, 20 char total ccid
  if (replybuffer[0] == '+') {
    // fona 3g?
    strncpy(ccid, replybuffer + 8, 20);
  } else {
    // fona 800 or 800
    strncpy(ccid, replybuffer, 20);
  }
  ccid[20] = 0;

  readline(); // eat 'OK'

  return strlen(ccid);
}

/********* IMEI **********************************************************/

/**
 * @brief Get the IMEI
 *
 * @param imei Pointer to a buffer to be filled with the IMEI
 * @return uint8_t
 */
uint8_t Adafruit_FONA::getIMEI(char *imei) {
  getReply(F("AT+GSN"));

  // up to 15 chars
  strncpy(imei, replybuffer, 15);
  imei[15] = 0;

  readline(); // eat 'OK'

  return strlen(imei);
}

/********* NETWORK *******************************************************/

/**
 * @brief Gets the current network status
 *
 * @return uint8_t The nework status:
 *
 *  * 0: Not registered, MT is not currently searching a new operator to
 * register to
 *  * 1: Registered, home network
 *  * 2: Not registered, but MT is currently searching a newoperator to register
 * to
 *  * 3: Registration denied
 *  * 4: Unknown
 *  * 5: Registered, roaming
 */
uint8_t Adafruit_FONA::getNetworkStatus(void) {
  uint16_t status;

  if (!sendParseReply(F("AT+CREG?"), F("+CREG: "), &status, ',', 1))
    return 0;

  return status;
}
/**
 * @brief get the current Received Signal Strength Indication
 *
 * @return uint8_t The current RSSI
 */
uint8_t Adafruit_FONA::getRSSI(void) {
  uint16_t reply;

  if (!sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply))
    return 0;

  return reply;
}

/********* AUDIO *******************************************************/

/**
 * @brief Set the audio ouput
 *
 * @param audio_output The output to set.
 *  * 0: headset
 *  * 1: external audio
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setAudio(uint8_t audio_output) {
  if (audio_output > 1)
    return false;

  return sendCheckReply(F("AT+CHFA="), audio_output, ok_reply);
}
/**
 * @brief Get the current volume level
 *
 * @return uint8_t The current volume level
 */
uint8_t Adafruit_FONA::getVolume(void) {
  uint16_t reply;

  if (!sendParseReply(F("AT+CLVL?"), F("+CLVL: "), &reply))
    return 0;

  return reply;
}

/**
 * @brief Set the volume
 *
 * @param volume_level The new volume to set
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setVolume(uint8_t volume_level) {
  return sendCheckReply(F("AT+CLVL="), volume_level, ok_reply);
}

/**
 * @brief Play a DTMF/Key tone
 *
 * @param dtmf The tone to play
 * @return true: success, false: failure
 */
bool Adafruit_FONA::playDTMF(char dtmf) {
  char str[4];
  str[0] = '\"';
  str[1] = dtmf;
  str[2] = '\"';
  str[3] = 0;
  return sendCheckReply(F("AT+CLDTMF=3,"), str, ok_reply);
}

/**
 * @brief Play a toolkit tone
 *
 * @param t The tone to play
 * @param len The tone length
 * @return true: success, false: failure
 */
bool Adafruit_FONA::playToolkitTone(uint8_t t, uint16_t len) {
  return sendCheckReply(F("AT+STTONE=1,"), t, len, ok_reply);
}
/**
 * @brief Play a toolkit tone
 *
 * @param t The tone to play
 * @param len The tone length
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::playToolkitTone(uint8_t t, uint16_t len) {
  if (!sendCheckReply(F("AT+CPTONE="), t, ok_reply))
    return false;
  delay(len);
  return sendCheckReply(F("AT+CPTONE=0"), ok_reply);
}

/**
 * @brief Set the microphone gain
 *
 *
 * @param a Which microphone gain to set.
 *  * 0: headset
 *  * 1: external audio
 *
 * @param level The new gain level
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setMicVolume(uint8_t a, uint8_t level) {

  if (a > 1)
    return false;

  return sendCheckReply(F("AT+CMIC="), a, level, ok_reply);
}

/********* FM RADIO *******************************************************/

/**
 * @brief Turn the FM Radio on or off
 *
 * @param onoff true; on false: off
 * @param a The audio source to use:
 *  * 0: headset
 *  * 1: external audio
 * @return true: success, false: failure
 */
bool Adafruit_FONA::FMradio(bool onoff, uint8_t a) {
  if (!onoff) {
    return sendCheckReply(F("AT+FMCLOSE"), ok_reply);
  }

  if (a > 1)
    return false;

  return sendCheckReply(F("AT+FMOPEN="), a, ok_reply);
}

/**
 * @brief Set the FM Radio station
 *
 * @param station The station to set the radio to
 * @return true: success, false: failure
 */
bool Adafruit_FONA::tuneFMradio(uint16_t station) {
  // Fail if FM station is outside allowed range.
  if ((station < 870) || (station > 1090))
    return false;

  return sendCheckReply(F("AT+FMFREQ="), station, ok_reply);
}

/**
 * @brief Set the FM Radio Volume
 *
 * @param i The volume to set
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setFMVolume(uint8_t i) {
  // Fail if volume is outside allowed range (0-6).
  if (i > 6) {
    return false;
  }
  // Send FM volume command and verify response.
  return sendCheckReply(F("AT+FMVOLUME="), i, ok_reply);
}
/**
 * @brief Get the current volume level
 *
 * @return uint8_t The current volume level
 */
int8_t Adafruit_FONA::getFMVolume(void) {
  uint16_t level;

  if (!sendParseReply(F("AT+FMVOLUME?"), F("+FMVOLUME: "), &level))
    return 0;

  return level;
}
/**
 * @brief Gets the current FM signal level
 *
 * @param station The station to chek the signal level of
 * @return int8_t The signal level of the given stations. -1 if FM station is
 * outside allowed range.
 */
int8_t Adafruit_FONA::getFMSignalLevel(uint16_t station) {

  if ((station < 875) || (station > 1080)) {
    return -1;
  }

  // Send FM signal level query command.
  // Note, need to explicitly send timeout so right overload is chosen.
  getReply(F("AT+FMSIGNAL="), station, FONA_DEFAULT_TIMEOUT_MS);
  // Check response starts with expected value.
  char *p = prog_char_strstr(replybuffer, PSTR("+FMSIGNAL: "));
  if (p == 0)
    return -1;
  p += 11;
  // Find second colon to get start of signal quality.
  p = strchr(p, ':');
  if (p == 0)
    return -1;
  p += 1;
  // Parse signal quality.
  int8_t level = atoi(p);
  readline(); // eat the "OK"
  return level;
}

/********* PWM/BUZZER **************************************************/

/**
 * @brief Set the PWM Period and Duty Cycle
 *
 * @param period The PWM period
 * @param duty The PWM duty cycle
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setPWM(uint16_t period, uint8_t duty) {
  if (period > 2000)
    return false;
  if (duty > 100)
    return false;

  return sendCheckReply(F("AT+SPWM=0,"), period, duty, ok_reply);
}

/********* CALL PHONES **************************************************/

/**
 * @brief Call a phone number
 *
 * @param number Pointer to a buffer with the phone number to call
 * @return true: success, false: failure
 */
bool Adafruit_FONA::callPhone(char *number) {
  char sendbuff[35] = "ATD";
  strncpy(sendbuff + 3, number, min(30, (int)strlen(number)));
  uint8_t x = strlen(sendbuff);
  sendbuff[x] = ';';
  sendbuff[x + 1] = 0;
  // DEBUG_PRINTLN(sendbuff);

  return sendCheckReply(sendbuff, ok_reply);
}

/**
 * @brief Get the current call status
 *
 * @return uint8_t The call status:
 * * 0: Ready
 * * 1: Failure
 * * 2: Unknown
 * * 3: Ringing
 * * 4: Call in progress
 */
uint8_t Adafruit_FONA::getCallStatus(void) {
  uint16_t phoneStatus;

  if (!sendParseReply(F("AT+CPAS"), F("+CPAS: "), &phoneStatus))
    return FONA_CALL_FAILED; // 1, since 0 is actually a known, good reply

  return phoneStatus;
}
/**
 * @brief End the current call
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::hangUp(void) { return sendCheckReply(F("ATH0"), ok_reply); }
/**
 * @brief End the current call
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::hangUp(void) {
  getReply(F("ATH"));

  return (prog_char_strstr(replybuffer, (prog_char *)F("VOICE CALL: END")) !=
          0);
}

/**
 * @brief Answer a call
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::pickUp(void) { return sendCheckReply(F("ATA"), ok_reply); }

/**
 * @brief Answer an incoming call
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::pickUp(void) {
  return sendCheckReply(F("ATA"), F("VOICE CALL: BEGIN"));
}
/**
 * @brief On incoming call callback
 *
 */
void Adafruit_FONA::onIncomingCall() {

  DEBUG_PRINT(F("> "));
  DEBUG_PRINTLN(F("Incoming call..."));

  Adafruit_FONA::_incomingCall = true;
}

bool Adafruit_FONA::_incomingCall = false;

/**
 * @brief Enable or disable caller ID
 *
 * @param enable true to enable, false to disable
 * @param interrupt An optional interrupt to attach
 * @return true: success, false: failure
 */
bool Adafruit_FONA::callerIdNotification(bool enable, uint8_t interrupt) {
  if (enable) {
    attachInterrupt(interrupt, onIncomingCall, FALLING);
    return sendCheckReply(F("AT+CLIP=1"), ok_reply);
  }

  detachInterrupt(interrupt);
  return sendCheckReply(F("AT+CLIP=0"), ok_reply);
}

/**
 * @brief Get the number of the incoming call
 *
 * @param phonenum Pointer to a buffer to hold the incoming caller's phone
 * number
 * @return true: success, false: failure
 */
bool Adafruit_FONA::incomingCallNumber(char *phonenum) {
  //+CLIP: "<incoming phone number>",145,"",0,"",0
  if (!Adafruit_FONA::_incomingCall)
    return false;

  readline();
  while (!prog_char_strcmp(replybuffer, (prog_char *)F("RING")) == 0) {
    flushInput();
    readline();
  }

  readline(); // reads incoming phone number line

  parseReply(F("+CLIP: \""), phonenum, '"');

  DEBUG_PRINT(F("Phone Number: "));
  DEBUG_PRINTLN(replybuffer);

  Adafruit_FONA::_incomingCall = false;
  return true;
}

/********* SMS **********************************************************/

/**
 * @brief Get the current SMS Iterrupt
 *
 * @return uint8_t The SMS interrupt or 0 on failure
 */
uint8_t Adafruit_FONA::getSMSInterrupt(void) {
  uint16_t reply;

  if (!sendParseReply(F("AT+CFGRI?"), F("+CFGRI: "), &reply))
    return 0;

  return reply;
}

/**
 * @brief Attach an interrupt for SMS notifications
 *
 * @param i The interrupt to set
 * @return true: success, false: failure
 */
bool Adafruit_FONA::setSMSInterrupt(uint8_t i) {
  return sendCheckReply(F("AT+CFGRI="), i, ok_reply);
}
/**
 * @brief Get the number of SMS
 *
 * @return int8_t The SMS count. -1 on error
 */
int8_t Adafruit_FONA::getNumSMS(void) {
  uint16_t numsms;

  // get into text mode
  if (!sendCheckReply(F("AT+CMGF=1"), ok_reply))
    return -1;

  // ask how many sms are stored
  if (sendParseReply(F("AT+CPMS?"), F(FONA_PREF_SMS_STORAGE ","), &numsms))
    return numsms;
  if (sendParseReply(F("AT+CPMS?"), F("\"SM\","), &numsms))
    return numsms;
  if (sendParseReply(F("AT+CPMS?"), F("\"SM_P\","), &numsms))
    return numsms;
  return -1;
}

// Reading SMS's is a bit involved so we don't use helpers that may cause delays
// or debug printouts!

/**
 * @brief Read an SMS message into a provided buffer
 *
 * @param message_index The SMS message index to retrieve
 * @param smsbuff SMS message buffer
 * @param maxlen The maximum read length
 * @param readlen The length read
 * @return true: success, false: failure
 */
bool Adafruit_FONA::readSMS(uint8_t message_index, char *smsbuff,
                            uint16_t maxlen, uint16_t *readlen) {
  // text mode
  if (!sendCheckReply(F("AT+CMGF=1"), ok_reply))
    return false;

  // show all text mode parameters
  if (!sendCheckReply(F("AT+CSDH=1"), ok_reply))
    return false;

  // parse out the SMS len
  uint16_t thesmslen = 0;

  DEBUG_PRINT(F("AT+CMGR="));
  DEBUG_PRINTLN(message_index);

  // getReply(F("AT+CMGR="), message_index, 1000);  //  do not print debug!
  mySerial->print(F("AT+CMGR="));
  mySerial->println(message_index);
  readline(1000); // timeout

  // DEBUG_PRINT(F("Reply: ")); DEBUG_PRINTLN(replybuffer);
  // parse it out...

  DEBUG_PRINTLN(replybuffer);

  if (!parseReply(F("+CMGR:"), &thesmslen, ',', 11)) {
    *readlen = 0;
    return false;
  }

  readRaw(thesmslen);

  flushInput();

  uint16_t thelen = min(maxlen, (uint16_t)strlen(replybuffer));
  strncpy(smsbuff, replybuffer, thelen);
  smsbuff[thelen] = 0; // end the string

  DEBUG_PRINTLN(replybuffer);

  *readlen = thelen;
  return true;
}

/**
 * @brief Retrieve the sender of the specified SMS message and copy it as a
 *    string to the sender buffer.  Up to senderlen characters of the sender
 * will be copied and a null terminator will be added if less than senderlen
 *    characters are copied to the result.
 *
 * @param message_index The SMS message index to retrieve the sender for
 * @param sender Pointer to a buffer to fill with the sender
 * @param senderlen The maximum length to read
 * @return true: a result was successfully retrieved, false: failure
 */
bool Adafruit_FONA::getSMSSender(uint8_t message_index, char *sender,
                                 int senderlen) {
  // Ensure text mode and all text mode parameters are sent.
  if (!sendCheckReply(F("AT+CMGF=1"), ok_reply))
    return false;
  if (!sendCheckReply(F("AT+CSDH=1"), ok_reply))
    return false;

  DEBUG_PRINT(F("AT+CMGR="));
  DEBUG_PRINTLN(message_index);

  // Send command to retrieve SMS message and parse a line of response.
  mySerial->print(F("AT+CMGR="));
  mySerial->println(message_index);
  readline(1000);

  DEBUG_PRINTLN(replybuffer);

  // Parse the second field in the response.
  bool result = parseReplyQuoted(F("+CMGR:"), sender, senderlen, ',', 1);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

/**
 * @brief Send an SMS Message from a buffer provided
 *
 * @param smsaddr The SMS address buffer
 * @param smsmsg The SMS message buffer
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendSMS(char *smsaddr, char *smsmsg) {
  if (!sendCheckReply(F("AT+CMGF=1"), ok_reply))
    return false;

  char sendcmd[30] = "AT+CMGS=\"";
  strncpy(sendcmd + 9, smsaddr,
          30 - 9 - 2); // 9 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';

  if (!sendCheckReply(sendcmd, F("> ")))
    return false;

  DEBUG_PRINT(F("> "));
  DEBUG_PRINTLN(smsmsg);

  mySerial->println(smsmsg);
  mySerial->println();
  mySerial->write(0x1A);

  DEBUG_PRINTLN("^Z");

  if ((_type == FONA3G_A) || (_type == FONA3G_E)) {
    // Eat two sets of CRLF
    readline(200);
    // DEBUG_PRINT("Line 1: "); DEBUG_PRINTLN(strlen(replybuffer));
    readline(200);
    // DEBUG_PRINT("Line 2: "); DEBUG_PRINTLN(strlen(replybuffer));
  }
  readline(10000); // read the +CMGS reply, wait up to 10 seconds!!!
  // DEBUG_PRINT("Line 3: "); DEBUG_PRINTLN(strlen(replybuffer));
  if (strstr(replybuffer, "+CMGS") == 0) {
    return false;
  }
  readline(1000); // read OK
  // DEBUG_PRINT("* "); DEBUG_PRINTLN(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    return false;
  }

  return true;
}

/**
 * @brief Delete an SMS Message
 *
 * @param message_index The message to delete
 * @return true: success, false: failure
 */
bool Adafruit_FONA::deleteSMS(uint8_t message_index) {
  if (!sendCheckReply(F("AT+CMGF=1"), ok_reply))
    return false;
  // read an sms
  char sendbuff[12] = "AT+CMGD=000";
  sendbuff[8] = (message_index / 100) + '0';
  message_index %= 100;
  sendbuff[9] = (message_index / 10) + '0';
  message_index %= 10;
  sendbuff[10] = message_index + '0';

  return sendCheckReply(sendbuff, ok_reply, 2000);
}

/********* USSD *********************************************************/

/**
 * @brief Send USSD
 *
 * @param ussdmsg The USSD message buffer
 * @param ussdbuff The USSD bufer
 * @param maxlen The maximum read length
 * @param readlen The length actually read
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen,
                             uint16_t *readlen) {
  if (!sendCheckReply(F("AT+CUSD=1"), ok_reply))
    return false;

  char sendcmd[30] = "AT+CUSD=1,\"";
  strncpy(sendcmd + 11, ussdmsg,
          30 - 11 - 2); // 11 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';

  if (!sendCheckReply(sendcmd, ok_reply)) {
    *readlen = 0;
    return false;
  } else {
    readline(10000); // read the +CUSD reply, wait up to 10 seconds!!!
    // DEBUG_PRINT("* "); DEBUG_PRINTLN(replybuffer);
    char *p = prog_char_strstr(replybuffer, PSTR("+CUSD: "));
    if (p == 0) {
      *readlen = 0;
      return false;
    }
    p += 7; //+CUSD
    // Find " to get start of ussd message.
    p = strchr(p, '\"');
    if (p == 0) {
      *readlen = 0;
      return false;
    }
    p += 1; //"
    // Find " to get end of ussd message.
    char *strend = strchr(p, '\"');

    uint16_t lentocopy = min(maxlen - 1, strend - p);
    strncpy(ussdbuff, p, lentocopy + 1);
    ussdbuff[lentocopy] = 0;
    *readlen = lentocopy;
  }
  return true;
}

/********* TIME **********************************************************/

/**
 * @brief Enable network time sync
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA::enableNetworkTimeSync(bool onoff) {
  if (onoff) {
    if (!sendCheckReply(F("AT+CLTS=1"), ok_reply))
      return false;
  } else {
    if (!sendCheckReply(F("AT+CLTS=0"), ok_reply))
      return false;
  }

  flushInput(); // eat any 'Unsolicted Result Code'

  return true;
}

/**
 * @brief Enable NTP time sync
 *
 * @param onoff true: enable false: disable
 * @param ntpserver The NTP server buffer
 * @return true: success, false: failure
 */
bool Adafruit_FONA::enableNTPTimeSync(bool onoff,
                                      FONAFlashStringPtr ntpserver) {
  if (onoff) {
    if (!sendCheckReply(F("AT+CNTPCID=1"), ok_reply))
      return false;

    mySerial->print(F("AT+CNTP=\""));
    if (ntpserver != 0) {
      mySerial->print(ntpserver);
    } else {
      mySerial->print(F("pool.ntp.org"));
    }
    mySerial->println(F("\",0"));
    readline(FONA_DEFAULT_TIMEOUT_MS);
    if (strcmp(replybuffer, "OK") != 0)
      return false;

    if (!sendCheckReply(F("AT+CNTP"), ok_reply, 10000))
      return false;

    uint16_t status;
    readline(10000);
    if (!parseReply(F("+CNTP:"), &status))
      return false;
  } else {
    if (!sendCheckReply(F("AT+CNTPCID=0"), ok_reply))
      return false;
  }

  return true;
}

/**
 * @brief Get the current time
 *
 * @param time_buffer Pointer to a buffer to hold the time
 * @param maxlen maximum read length
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getTime(char *time_buffer, uint16_t maxlen) {
  getReply(F("AT+CCLK?"), (uint16_t)10000);
  if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
    return false;

  char *p = replybuffer + 7;
  uint16_t lentocopy = min(maxlen - 1, (int)strlen(p));
  strncpy(time_buffer, p, lentocopy + 1);
  time_buffer[lentocopy] = 0;

  readline(); // eat OK

  return true;
}

/********* GPS **********************************************************/

/**
 * @brief Enable GPS
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA::enableGPS(bool onoff) {
  uint16_t state;

  // first check if its already on or off

  if (_type == FONA808_V2) {
    if (!sendParseReply(F("AT+CGNSPWR?"), F("+CGNSPWR: "), &state))
      return false;
  } else {
    if (!sendParseReply(F("AT+CGPSPWR?"), F("+CGPSPWR: "), &state))
      return false;
  }

  if (onoff && !state) {
    if (_type == FONA808_V2) {
      if (!sendCheckReply(F("AT+CGNSPWR=1"), ok_reply)) // try GNS command
        return false;
    } else {
      if (!sendCheckReply(F("AT+CGPSPWR=1"), ok_reply))
        return false;
    }
  } else if (!onoff && state) {
    if (_type == FONA808_V2) {
      if (!sendCheckReply(F("AT+CGNSPWR=0"), ok_reply)) // try GNS command
        return false;
    } else {
      if (!sendCheckReply(F("AT+CGPSPWR=0"), ok_reply))
        return false;
    }
  }
  return true;
}

/**
 * @brief Enable GPS
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::enableGPS(bool onoff) {
  uint16_t state;

  // first check if its already on or off
  if (!Adafruit_FONA::sendParseReply(F("AT+CGPS?"), F("+CGPS: "), &state))
    return false;

  if (onoff && !state) {
    if (!sendCheckReply(F("AT+CGPS=1"), ok_reply))
      return false;
  } else if (!onoff && state) {
    if (!sendCheckReply(F("AT+CGPS=0"), ok_reply))
      return false;
    // this takes a little time
    readline(2000); // eat '+CGPS: 0'
  }
  return true;
}
/**
 * @brief Get teh GPS status
 *
 * @return int8_t The GPS status:
 * * 0: GPS off
 * 1: No fix
 * 2: 2D fix
 * 3: 3D fix
 */
int8_t Adafruit_FONA::GPSstatus(void) {
  if (_type == FONA808_V2) {
    // 808 V2 uses GNS commands and doesn't have an explicit 2D/3D fix status.
    // Instead just look for a fix and if found assume it's a 3D fix.
    getReply(F("AT+CGNSINF"));
    char *p = prog_char_strstr(replybuffer, (prog_char *)F("+CGNSINF: "));
    if (p == 0)
      return -1;
    p += 10;
    readline(); // eat 'OK'
    if (p[0] == '0')
      return 0; // GPS is not even on!

    p += 2; // Skip to second value, fix status.
    // DEBUG_PRINTLN(p);
    // Assume if the fix status is '1' then we have a 3D fix, otherwise no fix.
    if (p[0] == '1')
      return 3;
    else
      return 1;
  }
  if (_type == FONA3G_A || _type == FONA3G_E) {
    // FONA 3G doesn't have an explicit 2D/3D fix status.
    // Instead just look for a fix and if found assume it's a 3D fix.
    getReply(F("AT+CGPSINFO"));
    char *p = prog_char_strstr(replybuffer, (prog_char *)F("+CGPSINFO:"));
    if (p == 0)
      return -1;
    if (p[10] != ',')
      return 3; // if you get anything, its 3D fix
    return 0;
  } else {
    // 808 V1 looks for specific 2D or 3D fix state.
    getReply(F("AT+CGPSSTATUS?"));
    char *p =
        prog_char_strstr(replybuffer, (prog_char *)F("SSTATUS: Location "));
    if (p == 0)
      return -1;
    p += 18;
    readline(); // eat 'OK'
    // DEBUG_PRINTLN(p);
    if (p[0] == 'U')
      return 0;
    if (p[0] == 'N')
      return 1;
    if (p[0] == '2')
      return 2;
    if (p[0] == '3')
      return 3;
  }
  // else
  return 0;
}

/**
 * @brief Fill a buffer with the current GPS reading
 *
 * @param arg The mode to use, where applicable
 * @param buffer The buffer to be filled with the reading
 * @param maxbuff the maximum amound to write into the buffer
 * @return uint8_t The number of bytes read
 */
uint8_t Adafruit_FONA::getGPS(uint8_t arg, char *buffer, uint8_t maxbuff) {
  int32_t x = arg;

  if ((_type == FONA3G_A) || (_type == FONA3G_E)) {
    getReply(F("AT+CGPSINFO"));
  } else if (_type == FONA808_V1) {
    getReply(F("AT+CGPSINF="), x);
  } else {
    getReply(F("AT+CGNSINF"));
  }

  char *p = prog_char_strstr(replybuffer, (prog_char *)F("SINF"));
  if (p == 0) {
    buffer[0] = 0;
    return 0;
  }

  p += 6;

  uint8_t len = max(maxbuff - 1, (int)strlen(p));
  strncpy(buffer, p, len);
  buffer[len] = 0;

  readline(); // eat 'OK'
  return len;
}

/**
 * @brief Get a GPS reading
 *
 * @param lat  Pointer to a buffer to be filled with thelatitude
 * @param lon Pointer to a buffer to be filled with the longitude
 * @param speed_kph Pointer to a buffer to be filled with the speed in
 * kilometers per hour
 * @param heading Pointer to a buffer to be filled with the heading
 * @param altitude Pointer to a buffer to be filled with the altitude
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getGPS(float *lat, float *lon, float *speed_kph,
                           float *heading, float *altitude) {

  char gpsbuffer[120];

  // we need at least a 2D fix
  if (GPSstatus() < 2)
    return false;

  // grab the mode 2^5 gps csv from the sim808
  uint8_t res_len = getGPS(32, gpsbuffer, 120);

  // make sure we have a response
  if (res_len == 0)
    return false;

  if (_type == FONA3G_A || _type == FONA3G_E) {
    // Parse 3G respose
    // +CGPSINFO:4043.000000,N,07400.000000,W,151015,203802.1,-12.0,0.0,0
    // skip beginning

    // grab the latitude
    char *latp = strtok(gpsbuffer, ",");
    if (!latp)
      return false;

    // grab latitude direction
    char *latdir = strtok(NULL, ",");
    if (!latdir)
      return false;

    // grab longitude
    char *longp = strtok(NULL, ",");
    if (!longp)
      return false;

    // grab longitude direction
    char *longdir = strtok(NULL, ",");
    if (!longdir)
      return false;

    // skip date & time
    (void)strtok(NULL, ",");
    (void)strtok(NULL, ",");

    // only grab altitude if needed
    if (altitude != NULL) {
      // grab altitude
      char *altp = strtok(NULL, ",");
      if (!altp)
        return false;
      *altitude = atof(altp);
    }

    // only grab speed if needed
    if (speed_kph != NULL) {
      // grab the speed in km/h
      char *speedp = strtok(NULL, ",");
      if (!speedp)
        return false;

      *speed_kph = atof(speedp);
    }

    // only grab heading if needed
    if (heading != NULL) {

      // grab the speed in knots
      char *coursep = strtok(NULL, ",");
      if (!coursep)
        return false;

      *heading = atof(coursep);
    }

    double latitude = atof(latp);
    double longitude = atof(longp);

    // convert latitude from minutes to decimal
    float degrees = floor(latitude / 100);
    double minutes = latitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (latdir[0] == 'S')
      degrees *= -1;

    *lat = degrees;

    // convert longitude from minutes to decimal
    degrees = floor(longitude / 100);
    minutes = longitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (longdir[0] == 'W')
      degrees *= -1;

    *lon = degrees;

  } else if (_type == FONA808_V2) {
    // Parse 808 V2 response.  See table 2-3 from here for format:
    // http://www.adafruit.com/datasheets/SIM800%20Series_GNSS_Application%20Note%20V1.00.pdf

    // skip GPS run status
    char *tok = strtok(gpsbuffer, ",");
    if (!tok)
      return false;

    // skip fix status
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // skip date
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // grab the latitude
    char *latp = strtok(NULL, ",");
    if (!latp)
      return false;

    // grab longitude
    char *longp = strtok(NULL, ",");
    if (!longp)
      return false;

    *lat = atof(latp);
    *lon = atof(longp);

    // only grab altitude if needed
    if (altitude != NULL) {
      // grab altitude
      char *altp = strtok(NULL, ",");
      if (!altp)
        return false;

      *altitude = atof(altp);
    }

    // only grab speed if needed
    if (speed_kph != NULL) {
      // grab the speed in km/h
      char *speedp = strtok(NULL, ",");
      if (!speedp)
        return false;

      *speed_kph = atof(speedp);
    }

    // only grab heading if needed
    if (heading != NULL) {

      // grab the speed in knots
      char *coursep = strtok(NULL, ",");
      if (!coursep)
        return false;

      *heading = atof(coursep);
    }
  } else {
    // Parse 808 V1 response.

    // skip mode
    char *tok = strtok(gpsbuffer, ",");
    if (!tok)
      return false;

    // skip date
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // skip fix
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // grab the latitude
    char *latp = strtok(NULL, ",");
    if (!latp)
      return false;

    // grab latitude direction
    char *latdir = strtok(NULL, ",");
    if (!latdir)
      return false;

    // grab longitude
    char *longp = strtok(NULL, ",");
    if (!longp)
      return false;

    // grab longitude direction
    char *longdir = strtok(NULL, ",");
    if (!longdir)
      return false;

    double latitude = atof(latp);
    double longitude = atof(longp);

    // convert latitude from minutes to decimal
    float degrees = floor(latitude / 100);
    double minutes = latitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (latdir[0] == 'S')
      degrees *= -1;

    *lat = degrees;

    // convert longitude from minutes to decimal
    degrees = floor(longitude / 100);
    minutes = longitude - (100 * degrees);
    minutes /= 60;
    degrees += minutes;

    // turn direction into + or -
    if (longdir[0] == 'W')
      degrees *= -1;

    *lon = degrees;

    // only grab speed if needed
    if (speed_kph != NULL) {

      // grab the speed in knots
      char *speedp = strtok(NULL, ",");
      if (!speedp)
        return false;

      // convert to kph
      *speed_kph = atof(speedp) * 1.852;
    }

    // only grab heading if needed
    if (heading != NULL) {

      // grab the speed in knots
      char *coursep = strtok(NULL, ",");
      if (!coursep)
        return false;

      *heading = atof(coursep);
    }

    // no need to continue
    if (altitude == NULL)
      return true;

    // we need at least a 3D fix for altitude
    if (GPSstatus() < 3)
      return false;

    // grab the mode 0 gps csv from the sim808
    res_len = getGPS(0, gpsbuffer, 120);

    // make sure we have a response
    if (res_len == 0)
      return false;

    // skip mode
    tok = strtok(gpsbuffer, ",");
    if (!tok)
      return false;

    // skip lat
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // skip long
    tok = strtok(NULL, ",");
    if (!tok)
      return false;

    // grab altitude
    char *altp = strtok(NULL, ",");
    if (!altp)
      return false;

    *altitude = atof(altp);
  }

  return true;
}

/**
 * @brief Enable GPS NMEA output

 * @param enable_value
 * * FONA 808 v2:
 *  * 0: Disable
 *  * 1: Enable
 * * For Others see the application note for information on the
 * `AT+CGPSOUT=` command:
 *  *
 https://cdn-shop.adafruit.com/datasheets/SIM808_GPS_Application_Note_V1.00.pdf
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::enableGPSNMEA(uint8_t enable_value) {
  char sendbuff[15] = "AT+CGPSOUT=000";
  sendbuff[11] = (enable_value / 100) + '0';
  enable_value %= 100;
  sendbuff[12] = (enable_value / 10) + '0';
  enable_value %= 10;
  sendbuff[13] = enable_value + '0';

  if (_type == FONA808_V2) {
    if (enable_value)
      return sendCheckReply(F("AT+CGNSTST=1"), ok_reply);
    else
      return sendCheckReply(F("AT+CGNSTST=0"), ok_reply);
  } else {
    return sendCheckReply(sendbuff, ok_reply, 2000);
  }
}

/********* GPRS **********************************************************/

/**
 * @brief Enable GPRS
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA::enableGPRS(bool onoff) {

  if (onoff) {
    // disconnect all sockets
    sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000);

    if (!sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))
      return false;

    // set bearer profile! connection type GPRS
    if (!sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""), ok_reply,
                        10000))
      return false;

    // set bearer profile access point name
    if (apn) {
      // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the
      // configured APN value.
      if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), apn, ok_reply,
                                10000))
        return false;

      // send AT+CSTT,"apn","user","pass"
      flushInput();

      mySerial->print(F("AT+CSTT=\""));
      mySerial->print(apn);
      if (apnusername) {
        mySerial->print("\",\"");
        mySerial->print(apnusername);
      }
      if (apnpassword) {
        mySerial->print("\",\"");
        mySerial->print(apnpassword);
      }
      mySerial->println("\"");

      DEBUG_PRINT(F("\t---> "));
      DEBUG_PRINT(F("AT+CSTT=\""));
      DEBUG_PRINT(apn);

      if (apnusername) {
        DEBUG_PRINT("\",\"");
        DEBUG_PRINT(apnusername);
      }
      if (apnpassword) {
        DEBUG_PRINT("\",\"");
        DEBUG_PRINT(apnpassword);
      }
      DEBUG_PRINTLN("\"");

      if (!expectReply(ok_reply))
        return false;

      // set username/password
      if (apnusername) {
        // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the
        // configured APN username.
        if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), apnusername,
                                  ok_reply, 10000))
          return false;
      }
      if (apnpassword) {
        // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the
        // configured APN password.
        if (!sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), apnpassword,
                                  ok_reply, 10000))
          return false;
      }
    }

    // open GPRS context
    if (!sendCheckReply(F("AT+SAPBR=1,1"), ok_reply, 30000))
      return false;

    // bring up wireless connection
    if (!sendCheckReply(F("AT+CIICR"), ok_reply, 10000))
      return false;

  } else {
    // disconnect all sockets
    if (!sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000))
      return false;

    // close GPRS context
    if (!sendCheckReply(F("AT+SAPBR=0,1"), ok_reply, 10000))
      return false;

    if (!sendCheckReply(F("AT+CGATT=0"), ok_reply, 10000))
      return false;
  }
  return true;
}

/**
 * @brief Enable GPRS
 *
 * @param onoff true: enable false; disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::enableGPRS(bool onoff) {

  if (onoff) {
    // disconnect all sockets
    // sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 5000);

    if (!sendCheckReply(F("AT+CGATT=1"), ok_reply, 10000))
      return false;

    // set bearer profile access point name
    if (apn) {
      // Send command AT+CGSOCKCONT=1,"IP","<apn value>" where <apn value> is
      // the configured APN name.
      if (!sendCheckReplyQuoted(F("AT+CGSOCKCONT=1,\"IP\","), apn, ok_reply,
                                10000))
        return false;

      // set username/password
      if (apnusername) {
        char authstring[100] = "AT+CGAUTH=1,1,\"";
        char *strp = authstring + strlen(authstring);
        prog_char_strcpy(strp, (prog_char *)apnusername);
        strp += prog_char_strlen((prog_char *)apnusername);
        strp[0] = '\"';
        strp++;
        strp[0] = 0;

        if (apnpassword) {
          strp[0] = ',';
          strp++;
          strp[0] = '\"';
          strp++;
          prog_char_strcpy(strp, (prog_char *)apnpassword);
          strp += prog_char_strlen((prog_char *)apnpassword);
          strp[0] = '\"';
          strp++;
          strp[0] = 0;
        }

        if (!sendCheckReply(authstring, ok_reply, 10000))
          return false;
      }
    }

    // connect in transparent
    if (!sendCheckReply(F("AT+CIPMODE=1"), ok_reply, 10000))
      return false;
    // open network (?)
    if (!sendCheckReply(F("AT+NETOPEN=,,1"), F("Network opened"), 10000))
      return false;

    readline(); // eat 'OK'
  } else {
    // close GPRS context
    if (!sendCheckReply(F("AT+NETCLOSE"), F("Network closed"), 10000))
      return false;

    readline(); // eat 'OK'
  }

  return true;
}
/**
 * @brief Get the GPRS state
 *
 * @return uint8_t The GPRS state:
 * * 0: Attached
 * * 1: Detached
 */
uint8_t Adafruit_FONA::GPRSstate(void) {
  uint16_t state;

  if (!sendParseReply(F("AT+CGATT?"), F("+CGATT: "), &state))
    return -1;

  return state;
}
/**
 * @brief Set the GPRS network settings
 *
 * @param apn Pointer to the Access point name buffer
 * @param username Pointer to the Username buffer
 * @param password Pointer to the Password buffer
 */
void Adafruit_FONA::setGPRSNetworkSettings(FONAFlashStringPtr apn,
                                           FONAFlashStringPtr username,
                                           FONAFlashStringPtr password) {
  this->apn = apn;
  this->apnusername = username;
  this->apnpassword = password;
}

/**
 * @brief Get GSM location
 *
 * @param errorcode Pointer to a uint16_t to hold any resulting error code
 * @param buff Pointer to a buffer to hold the location
 * @param maxlen maximum read length
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getGSMLoc(uint16_t *errorcode, char *buff,
                              uint16_t maxlen) {

  getReply(F("AT+CIPGSMLOC=1,1"), (uint16_t)10000);

  if (!parseReply(F("+CIPGSMLOC: "), errorcode))
    return false;

  char *p = replybuffer + 14;
  uint16_t lentocopy = min(maxlen - 1, (int)strlen(p));
  strncpy(buff, p, lentocopy + 1);

  readline(); // eat OK

  return true;
}

/**
 * @brief Get GSM Location
 *
 * @param lat Pointer to a buffer to hold the latitude
 * @param lon Pointer to a buffer to hold the longitude
 * @return true: success, false: failure
 */
bool Adafruit_FONA::getGSMLoc(float *lat, float *lon) {

  uint16_t returncode;
  char gpsbuffer[120];

  // make sure we could get a response
  if (!getGSMLoc(&returncode, gpsbuffer, 120))
    return false;

  // make sure we have a valid return code
  if (returncode != 0)
    return false;

  // +CIPGSMLOC: 0,-74.007729,40.730160,2015/10/15,19:24:55
  // tokenize the gps buffer to locate the lat & long
  char *longp = strtok(gpsbuffer, ",");
  if (!longp)
    return false;

  char *latp = strtok(NULL, ",");
  if (!latp)
    return false;

  *lat = atof(latp);
  *lon = atof(longp);

  return true;
}
/********* TCP FUNCTIONS  ************************************/

/**
 * @brief Start a TCP connection
 *
 * @param server Pointer to a buffer with the server to connect to
 * @param port Pointer to a buffer witht the port to connect to
 * @return true: success, false: failure
 */

bool Adafruit_FONA::TCPconnect(char *server, uint16_t port) {
  flushInput();

  // close all old connections
  if (!sendCheckReply(F("AT+CIPSHUT"), F("SHUT OK"), 20000))
    return false;

  // single connection at a time
  if (!sendCheckReply(F("AT+CIPMUX=0"), ok_reply))
    return false;

  // manually read data
  if (!sendCheckReply(F("AT+CIPRXGET=1"), ok_reply))
    return false;

  DEBUG_PRINT(F("AT+CIPSTART=\"TCP\",\""));
  DEBUG_PRINT(server);
  DEBUG_PRINT(F("\",\""));
  DEBUG_PRINT(port);
  DEBUG_PRINTLN(F("\""));

  mySerial->print(F("AT+CIPSTART=\"TCP\",\""));
  mySerial->print(server);
  mySerial->print(F("\",\""));
  mySerial->print(port);
  mySerial->println(F("\""));

  if (!expectReply(ok_reply))
    return false;
  if (!expectReply(F("CONNECT OK")))
    return false;

  // looks like it was a success (?)
  return true;
}

/**
 * @brief Close the TCP connection
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::TCPclose(void) {
  return sendCheckReply(F("AT+CIPCLOSE"), ok_reply);
}

/**
 * @brief Check the TCP connection status
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::TCPconnected(void) {
  if (!sendCheckReply(F("AT+CIPSTATUS"), ok_reply, 100))
    return false;
  readline(100);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return (strcmp(replybuffer, "STATE: CONNECT OK") == 0);
}

/**
 * @brief Send data via TCP
 *
 * @param data Pointer to a buffer with the data to send
 * @param len length of the data to send
 * @return true: success, false: failure
 */
bool Adafruit_FONA::TCPsend(char *data, uint8_t len) {

  DEBUG_PRINT(F("AT+CIPSEND="));
  DEBUG_PRINTLN(len);
#ifdef ADAFRUIT_FONA_DEBUG
  for (uint16_t i = 0; i < len; i++) {
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINT(data[i], HEX);
  }
#endif
  DEBUG_PRINTLN();

  mySerial->print(F("AT+CIPSEND="));
  mySerial->println(len);
  readline();

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  if (replybuffer[0] != '>')
    return false;

  mySerial->write(data, len);
  readline(3000); // wait up to 3 seconds to send the data

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return (strcmp(replybuffer, "SEND OK") == 0);
}
/**
 * @brief Check if TCP bytes are available
 *
 * @return uint16_t The number of available bytes
 */
uint16_t Adafruit_FONA::TCPavailable(void) {
  uint16_t avail;

  if (!sendParseReply(F("AT+CIPRXGET=4"), F("+CIPRXGET: 4,"), &avail, ',', 0))
    return false;

  DEBUG_PRINT(avail);
  DEBUG_PRINTLN(F(" bytes available"));

  return avail;
}
/**
 * @brief Read from a TCP socket
 *
 * @param buff Pointer to a buffer to read into
 * @param len  The number of bytes to read
 * @return uint16_t The number of bytes read
 */
uint16_t Adafruit_FONA::TCPread(uint8_t *buff, uint8_t len) {
  uint16_t avail;

  mySerial->print(F("AT+CIPRXGET=2,"));
  mySerial->println(len);
  readline();
  if (!parseReply(F("+CIPRXGET: 2,"), &avail, ',', 0))
    return false;

  readRaw(avail);

#ifdef ADAFRUIT_FONA_DEBUG
  DEBUG_PRINT(avail);
  DEBUG_PRINTLN(F(" bytes read"));
  for (uint8_t i = 0; i < avail; i++) {
    DEBUG_PRINT(F(" 0x"));
    DEBUG_PRINT(replybuffer[i], HEX);
  }
  DEBUG_PRINTLN();
#endif

  memcpy(buff, replybuffer, avail);

  return avail;
}

/********* HTTP LOW LEVEL FUNCTIONS  ************************************/

/**
 * @brief Initialize HTTP
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_init() {
  return sendCheckReply(F("AT+HTTPINIT"), ok_reply);
}

/**
 * @brief Terminate HTTP
 *
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_term() {
  return sendCheckReply(F("AT+HTTPTERM"), ok_reply);
}

/**
 * @brief Start sending an HTTP parameter
 *
 * @param parameter Pointer to a buffer with the parameter to send
 * @param quoted true if the parameter should be quoted
 */
void Adafruit_FONA::HTTP_para_start(FONAFlashStringPtr parameter, bool quoted) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(F("AT+HTTPPARA=\""));
  DEBUG_PRINT(parameter);
  DEBUG_PRINTLN('"');

  mySerial->print(F("AT+HTTPPARA=\""));
  mySerial->print(parameter);
  if (quoted)
    mySerial->print(F("\",\""));
  else
    mySerial->print(F("\","));
}

/**
 * @brief Finish sending an HTTP parameter
 *
 * @param quoted true if the parameter should be quoted
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_para_end(bool quoted) {
  if (quoted)
    mySerial->println('"');
  else
    mySerial->println();

  return expectReply(ok_reply);
}

/**
 * @brief Send HTTP parameter
 *
 * @param parameter Pointer to a buffer with the parameter to send
 * @param value Pointer to a buffer with the parameter value
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter, const char *value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

/**
 * @brief Send HTTP parameter
 *
 * @param parameter Pointer to a buffer with the parameter to send
 * @param value Pointer to a buffer with the parameter value
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter,
                              FONAFlashStringPtr value) {
  HTTP_para_start(parameter, true);
  mySerial->print(value);
  return HTTP_para_end(true);
}

/**
 * @brief Send HTTP parameter
 *
 * @param parameter Pointer to a buffer with the parameter to send
 * @param value The parameter value
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_para(FONAFlashStringPtr parameter, int32_t value) {
  HTTP_para_start(parameter, false);
  mySerial->print(value);
  return HTTP_para_end(false);
}

/**
 * @brief Begin sending data via HTTP
 *
 * @param size The amount of data to be sent in bytes
 * @param maxTime The maximum amount of time in which to send the data, in
 * milliseconds
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_data(uint32_t size, uint32_t maxTime) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(F("AT+HTTPDATA="));
  DEBUG_PRINT(size);
  DEBUG_PRINT(',');
  DEBUG_PRINTLN(maxTime);

  mySerial->print(F("AT+HTTPDATA="));
  mySerial->print(size);
  mySerial->print(",");
  mySerial->println(maxTime);

  return expectReply(F("DOWNLOAD"));
}

/**
 * @brief Make an HTTP Request
 *
 * @param method The request method:
 * * 0: GET
 * * 1: POST
 * * 2: HEAD
 * @param status Pointer to a uint16_t to hold the request status as an RFC2616
 * HTTP response code: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 *
 * @param datalen Pointer to the  a `uint16_t` to hold the length of the data
 * read
 * @param timeout Timeout for waiting for response
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_action(uint8_t method, uint16_t *status,
                                uint16_t *datalen, int32_t timeout) {
  // Send request.
  if (!sendCheckReply(F("AT+HTTPACTION="), method, ok_reply))
    return false;

  // Parse response status and size.
  readline(timeout);
  if (!parseReply(F("+HTTPACTION:"), status, ',', 1))
    return false;
  if (!parseReply(F("+HTTPACTION:"), datalen, ',', 2))
    return false;

  return true;
}

/**
 * @brief Read all available HTTP data
 *
 * @param datalen Pointer to the  a `uint16_t` to hold the length of the data
 * read
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_readall(uint16_t *datalen) {
  getReply(F("AT+HTTPREAD"));
  if (!parseReply(F("+HTTPREAD:"), datalen, ',', 0))
    return false;

  return true;
}

/**
 * @brief Enable or disable SSL
 *
 * @param onoff true: enable false: disable
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_ssl(bool onoff) {
  return sendCheckReply(F("AT+HTTPSSL="), onoff ? 1 : 0, ok_reply);
}

/********* HTTP HIGH LEVEL FUNCTIONS ***************************/

/**
 * @brief Start a HTTP GET request
 *
 * @param url Pointer to a buffer with the URL to request
 * @param status Pointer to a uint16_t to hold the request status as an RFC2616
 * HTTP response code: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 *
 * @param datalen Pointer to the  a `uint16_t` to hold the length of the data
 * read
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_GET_start(char *url, uint16_t *status,
                                   uint16_t *datalen) {
  if (!HTTP_setup(url))
    return false;

  // HTTP GET
  if (!HTTP_action(FONA_HTTP_GET, status, datalen, 30000))
    return false;

  DEBUG_PRINT(F("Status: "));
  DEBUG_PRINTLN(*status);
  DEBUG_PRINT(F("Len: "));
  DEBUG_PRINTLN(*datalen);

  // HTTP response data
  if (!HTTP_readall(datalen))
    return false;

  return true;
}

/*
bool Adafruit_FONA_3G::HTTP_GET_start(char *ipaddr, char *path, uint16_t port
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

/**
 * @brief End an HTTP GET
 *
 */
void Adafruit_FONA::HTTP_GET_end(void) { HTTP_term(); }

/**
 * @brief Start an HTTP POST request
 *
 * @param url Pointer to a buffer with the URL to POST
 * @param contenttype The message content type
 * @param postdata Pointer to a buffer with the POST data to be sent
 * @param postdatalen The length of the POST data
 * @param status Pointer to a uint16_t to hold the request status as an RFC2616
 * HTTP response code: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 * @param datalen
 * @return true
 * @return false
 */
bool Adafruit_FONA::HTTP_POST_start(char *url, FONAFlashStringPtr contenttype,
                                    const uint8_t *postdata,
                                    uint16_t postdatalen, uint16_t *status,
                                    uint16_t *datalen) {
  if (!HTTP_setup(url))
    return false;

  if (!HTTP_para(F("CONTENT"), contenttype)) {
    return false;
  }

  // HTTP POST data
  if (!HTTP_data(postdatalen, 10000))
    return false;
  mySerial->write(postdata, postdatalen);
  if (!expectReply(ok_reply))
    return false;

  // HTTP POST
  if (!HTTP_action(FONA_HTTP_POST, status, datalen))
    return false;

  DEBUG_PRINT(F("Status: "));
  DEBUG_PRINTLN(*status);
  DEBUG_PRINT(F("Len: "));
  DEBUG_PRINTLN(*datalen);

  // HTTP response data
  if (!HTTP_readall(datalen))
    return false;

  return true;
}
/**
 * @brief End an HTTP POST request
 *
 */
void Adafruit_FONA::HTTP_POST_end(void) { HTTP_term(); }

/**
 * @brief Set the User Agent for HTTP requests
 *
 * @param useragent Pointer to a buffer with the user agent string to set
 */
void Adafruit_FONA::setUserAgent(FONAFlashStringPtr useragent) {
  this->useragent = useragent;
}
/**
 * @brief Set the HTTPS redirect flag
 *
 * @param onoff
 */
void Adafruit_FONA::setHTTPSRedirect(bool onoff) { httpsredirect = onoff; }

/********* HTTP HELPERS ****************************************/

/**
 * @brief Configure an HTTP request
 *
 * @param url Pointer to a buffer with the URL to POST
 * @return true: success, false: failure
 */
bool Adafruit_FONA::HTTP_setup(char *url) {
  // Handle any pending
  HTTP_term();

  // Initialize and set parameters
  if (!HTTP_init())
    return false;
  if (!HTTP_para(F("CID"), 1))
    return false;
  if (!HTTP_para(F("UA"), useragent))
    return false;
  if (!HTTP_para(F("URL"), url))
    return false;

  // HTTPS redirect
  if (httpsredirect) {
    if (!HTTP_para(F("REDIR"), 1))
      return false;

    if (!HTTP_ssl(true))
      return false;
  }

  return true;
}

/********* HELPERS *********************************************/

/**
 * @brief Check if the received reply matches the expectation
 *
 * @param reply The expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::expectReply(FONAFlashStringPtr reply, uint16_t timeout) {
  readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

/********* LOW LEVEL *******************************************/

/**
 * @brief Serial data available
 *
 * @return int
 */
inline int Adafruit_FONA::available(void) { return mySerial->available(); } ///<
/**
 * @brief Serial write
 *
 * @param x
 * @return size_t
 */
inline size_t Adafruit_FONA::write(uint8_t x) {
  return mySerial->write(x);
} ///<
/**
 * @brief Serial read
 *
 * @return int
 */
inline int Adafruit_FONA::read(void) { return mySerial->read(); } ///<
/**
 * @brief Serial peek
 *
 * @return int
 */
inline int Adafruit_FONA::peek(void) { return mySerial->peek(); } ///<
/**
 * @brief Flush the serial data
 *
 */
inline void Adafruit_FONA::flush() { mySerial->flush(); } ///<
/**
 * @brief Read all available serial input to flush pending data.
 *
 */
void Adafruit_FONA::flushInput() {
  uint16_t timeoutloop = 0;
  while (timeoutloop++ < 40) {
    while (available()) {
      read();
      timeoutloop = 0; // If char was received reset the timer
    }
    delay(1);
  }
}
/**
 * @brief Read directly into the reply buffer
 *
 * @param read_length The number of bytes to return
 * @return uint16_t The number of bytes read
 */
uint16_t Adafruit_FONA::readRaw(uint16_t read_length) {
  uint16_t idx = 0;

  while (read_length && (idx < sizeof(replybuffer) - 1)) {
    if (mySerial->available()) {
      replybuffer[idx] = mySerial->read();
      idx++;
      read_length--;
    }
  }
  replybuffer[idx] = 0;

  return idx;
}

/**
 * @brief Read a single line or up to 254 bytes
 *
 * @param timeout Reply timeout
 * @param multiline true: read the maximum amount. false: read up to the second
 * newline
 * @return uint8_t the number of bytes read
 */
uint8_t Adafruit_FONA::readline(uint16_t timeout, bool multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      // DEBUG_PRINTLN(F("SPACE"));
      break;
    }

    while (mySerial->available()) {
      char c = mySerial->read();
      if (c == '\r')
        continue;
      if (c == 0xA) {
        if (replyidx == 0) // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0; // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      // DEBUG_PRINT(c, HEX); DEBUG_PRINT("#"); DEBUG_PRINTLN(c);
      replyidx++;
    }

    if (timeout == 0) {
      // DEBUG_PRINTLN(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0; // null term
  return replyidx;
}
/**
 * @brief Send a command and return the reply
 *
 * @param send The char* command to send
 * @param timeout Timeout for reading a  response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReply(char *send, uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINTLN(send);

  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

/**
 * @brief Send a command and return the reply
 *
 * @param send The FONAFlashStringPtr command to send
 * @param timeout Timeout for reading a response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr send, uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINTLN(send);

  mySerial->println(send);

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer
// with response).
/**
 * @brief Send a command as prefix and suffix
 *
 * @param prefix Pointer to a buffer with the command prefix
 * @param suffix Pointer to a buffer with the command suffix
 * @param timeout Timeout for reading a response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, char *suffix,
                                uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(prefix);
  DEBUG_PRINTLN(suffix);

  mySerial->print(prefix);
  mySerial->println(suffix);

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer
// with response).
/**
 * @brief Send a command with
 *
 * @param prefix Pointer to a buffer with the command prefix
 * @param suffix The command suffix
 * @param timeout Timeout for reading a response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, int32_t suffix,
                                uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(prefix);
  DEBUG_PRINTLN(suffix, DEC);

  mySerial->print(prefix);
  mySerial->println(suffix, DEC);

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set
// replybuffer with response).
/**
 * @brief Send command with prefix and two suffixes
 *
 * @param prefix Pointer to a buffer with the command prefix
 * @param suffix1 The comannd first suffix
 * @param suffix2 The command second suffix
 * @param timeout Timeout for reading a response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReply(FONAFlashStringPtr prefix, int32_t suffix1,
                                int32_t suffix2, uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(prefix);
  DEBUG_PRINT(suffix1, DEC);
  DEBUG_PRINT(',');
  DEBUG_PRINTLN(suffix2, DEC);

  mySerial->print(prefix);
  mySerial->print(suffix1);
  mySerial->print(',');
  mySerial->println(suffix2, DEC);

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set
// replybuffer with response).
/**
 * @brief Send command prefix and suffix, returning the response length
 *
 * @param prefix Pointer to a buffer with the command prefix
 * @param suffix Pointer to a buffer with the command suffix
 * @param timeout Timeout for reading a response
 * @return uint8_t The response length
 */
uint8_t Adafruit_FONA::getReplyQuoted(FONAFlashStringPtr prefix,
                                      FONAFlashStringPtr suffix,
                                      uint16_t timeout) {
  flushInput();

  DEBUG_PRINT(F("\t---> "));
  DEBUG_PRINT(prefix);
  DEBUG_PRINT('"');
  DEBUG_PRINT(suffix);
  DEBUG_PRINTLN('"');

  mySerial->print(prefix);
  mySerial->print('"');
  mySerial->print(suffix);
  mySerial->println('"');

  uint8_t l = readline(timeout);

  DEBUG_PRINT(F("\t<--- "));
  DEBUG_PRINTLN(replybuffer);

  return l;
}

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param send Pointer to the buffer of data to send
 * @param reply Buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(char *send, char *reply, uint16_t timeout) {
  if (!getReply(send, timeout))
    return false;
  /*
    for (uint8_t i=0; i<strlen(replybuffer); i++) {
    DEBUG_PRINT(replybuffer[i], HEX); DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();
    for (uint8_t i=0; i<strlen(reply); i++) {
      DEBUG_PRINT(reply[i], HEX); DEBUG_PRINT(" ");
    }
    DEBUG_PRINTLN();
    */
  return (strcmp(replybuffer, reply) == 0);
}

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param send Pointer to the buffer of data to send
 * @param reply Pointer to a buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(FONAFlashStringPtr send,
                                   FONAFlashStringPtr reply, uint16_t timeout) {
  if (!getReply(send, timeout))
    return false;

  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param send Pointer to the buffer of data to send
 * @param reply Pointer to a buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(char *send, FONAFlashStringPtr reply,
                                   uint16_t timeout) {
  if (!getReply(send, timeout))
    return false;
  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply
// parameter.

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param prefix Pointer to a buffer with the prefix send
 * @param suffix Pointer to a buffer with the suffix send
 * @param reply Pointer to a buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, char *suffix,
                                   FONAFlashStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify FONA response matches reply
// parameter.

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param prefix Pointer to a buffer with the prefix to send
 * @param suffix The suffix to send
 * @param reply Pointer to a buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix,
                                   FONAFlashStringPtr reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param prefix Ponter to a buffer with the prefix to send
 * @param suffix1 The first suffix to send
 * @param suffix2 The second suffix to send
 * @param reply Pointer to a buffer with the expected reply
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix1,
                                   int32_t suffix2, FONAFlashStringPtr reply,
                                   uint16_t timeout) {
  getReply(prefix, suffix1, suffix2, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

/**
 * @brief Send data and verify the response matches an expected response
 *
 * @param prefix Pointer to a buffer with the prefix to send
 * @param suffix Pointer to a buffer with the suffix to send
 * @param reply Pointer to a buffer with the expected response
 * @param timeout Read timeout
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendCheckReplyQuoted(FONAFlashStringPtr prefix,
                                         FONAFlashStringPtr suffix,
                                         FONAFlashStringPtr reply,
                                         uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (prog_char_strcmp(replybuffer, (prog_char *)reply) == 0);
}

/**
 * @brief Parse a string in the response fields using a designated separator
 * and copy the value at the specified index in to the supplied buffer.
 *
 * @param toreply Pointer to a buffer with reply with the field being parsed
 * @param v Pointer to a buffer to fill with the the value from the parsed field
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA::parseReply(FONAFlashStringPtr toreply, uint16_t *v,
                               char divider, uint8_t index) {
  char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
  if (p == 0)
    return false;
  p += prog_char_strlen((prog_char *)toreply);
  // DEBUG_PRINTLN(p);
  for (uint8_t i = 0; i < index; i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p)
      return false;
    p++;
    // DEBUG_PRINTLN(p);
  }
  *v = atoi(p);

  return true;
}

/**

 * @brief Parse a string in the response fields using a designated separator
 * and copy the string at the specified index in to the supplied char buffer.
 *
 * @param toreply Pointer to a buffer with reply with the field being parsed
 * @param v Pointer to a buffer to fill with the string
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA::parseReply(FONAFlashStringPtr toreply, char *v,
                               char divider, uint8_t index) {
  uint8_t i = 0;
  char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
  if (p == 0)
    return false;
  p += prog_char_strlen((prog_char *)toreply);

  for (i = 0; i < index; i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p)
      return false;
    p++;
  }

  for (i = 0; i < strlen(p); i++) {
    if (p[i] == divider)
      break;
    v[i] = p[i];
  }

  v[i] = '\0';

  return true;
}

//
/**
 *
 * @brief Parse a string in the response fields using a designated separator
 * and copy the string (without quotes) at the specified index in to the
 * supplied char buffer.
 *
 * @param toreply Pointer to a buffer with reply with the field being parsed
 * @param v Pointer to a buffer to fill with the string. Make sure to supply a
 * buffer large enough to retrieve the expected value
 * @param maxlen The maximum read length
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA::parseReplyQuoted(FONAFlashStringPtr toreply, char *v,
                                     int maxlen, char divider, uint8_t index) {
  uint8_t i = 0, j;
  // Verify response starts with toreply.
  char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
  if (p == 0)
    return false;
  p += prog_char_strlen((prog_char *)toreply);

  // Find location of desired response field.
  for (i = 0; i < index; i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p)
      return false;
    p++;
  }

  // Copy characters from response field into result string.
  for (i = 0, j = 0; j < maxlen && i < strlen(p); ++i) {
    // Stop if a divier is found.
    if (p[i] == divider)
      break;
    // Skip any quotation marks.
    else if (p[i] == '"')
      continue;
    v[j++] = p[i];
  }

  // Add a null terminator if result string buffer was not filled.
  if (j < maxlen)
    v[j] = '\0';

  return true;
}

/**
 * @brief Send data and parse the reply
 *
 * @param tosend Pointer to the data buffer to send
 * @param toreply Pointer to a buffer with the expected reply string
 * @param v Pointer to a uint16_t buffer to hold the value of the parsed
 * response
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA::sendParseReply(FONAFlashStringPtr tosend,
                                   FONAFlashStringPtr toreply, uint16_t *v,
                                   char divider, uint8_t index) {
  getReply(tosend);

  if (!parseReply(toreply, v, divider, index))
    return false;

  readline(); // eat 'OK'

  return true;
}

// needed for CBC and others

/**
 * @brief Send data and parse the reply
 *
 * @param tosend Pointer to he data buffer to send
 * @param toreply The expected reply string
 * @param f Pointer to a float buffer to hold value of the parsed field
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::sendParseReply(FONAFlashStringPtr tosend,
                                      FONAFlashStringPtr toreply, float *f,
                                      char divider, uint8_t index) {
  getReply(tosend);

  if (!parseReply(toreply, f, divider, index))
    return false;

  readline(); // eat 'OK'

  return true;
}

/**
 * @brief Parse a reply
 *
 * @param toreply Pointer to a buffer with the expected reply string
 * @param f Pointer to a float buffer to hold the value of the parsed field
 * @param divider The divider character
 * @param index The index of the parsed field to retrieve
 * @return true: success, false: failure
 */
bool Adafruit_FONA_3G::parseReply(FONAFlashStringPtr toreply, float *f,
                                  char divider, uint8_t index) {
  char *p = prog_char_strstr(replybuffer, (prog_char *)toreply);
  if (p == 0)
    return false;
  p += prog_char_strlen((prog_char *)toreply);
  // DEBUG_PRINTLN(p);
  for (uint8_t i = 0; i < index; i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p)
      return false;
    p++;
    // DEBUG_PRINTLN(p);
  }
  *f = atof(p);

  return true;
}
