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
#ifndef ADAFRUIT_FONA_H
#define ADAFRUIT_FONA_H

#include "includes/FONAConfig.h"
#include "includes/FONAExtIncludes.h"
#include "includes/platform/FONAPlatform.h"

#define FONA800L 1
#define FONA800H 6

#define FONA808_V1 2
#define FONA808_V2 3

#define FONA3G_A 4
#define FONA3G_E 5

// Set the preferred SMS storage.
//   Use "SM" for storage on the SIM.
//   Use "ME" for internal storage on the FONA chip
#define FONA_PREF_SMS_STORAGE "\"SM\""
//#define FONA_PREF_SMS_STORAGE "\"ME\""

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

#define FONA_DEFAULT_TIMEOUT_MS 500

#define FONA_HTTP_GET 0
#define FONA_HTTP_POST 1
#define FONA_HTTP_HEAD 2

#define FONA_CALL_READY 0
#define FONA_CALL_FAILED 1
#define FONA_CALL_UNKNOWN 2
#define FONA_CALL_RINGING 3
#define FONA_CALL_INPROGRESS 4

/** Object that controls and keeps state for the FONA module. */
class Adafruit_FONA : public FONAStreamType {
public:
  Adafruit_FONA(int8_t r);
  bool begin(FONAStreamType &port);
  uint8_t type();

  // Stream
  int available(void);
  size_t write(uint8_t x);
  int read(void);
  int peek(void);
  void flush();

  // FONA 3G requirements
  bool setBaudrate(uint16_t baud);

  // RTC
  bool enableRTC(uint8_t mode);
  bool readRTC(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *hr,
               uint8_t *min, uint8_t *sec);

  // Battery and ADC
  bool getADCVoltage(uint16_t *v);
  bool getBattPercent(uint16_t *p);
  bool getBattVoltage(uint16_t *v);

  // SIM query
  uint8_t unlockSIM(char *pin);
  uint8_t getSIMCCID(char *ccid);
  uint8_t getNetworkStatus(void);
  uint8_t getRSSI(void);

  // IMEI
  uint8_t getIMEI(char *imei);

  // set Audio output
  bool setAudio(uint8_t audio_output);
  bool setVolume(uint8_t volume_level);
  uint8_t getVolume(void);
  bool playToolkitTone(uint8_t t, uint16_t len);
  bool setMicVolume(uint8_t a, uint8_t level);
  bool playDTMF(char tone);

  // FM radio functions.
  bool tuneFMradio(uint16_t station);
  bool FMradio(bool onoff, uint8_t a = FONA_HEADSETAUDIO);
  bool setFMVolume(uint8_t i);
  int8_t getFMVolume(void);
  int8_t getFMSignalLevel(uint16_t station);

  // SMS handling
  bool setSMSInterrupt(uint8_t i);
  uint8_t getSMSInterrupt(void);
  int8_t getNumSMS(void);
  bool readSMS(uint8_t message_index, char *smsbuff, uint16_t max,
               uint16_t *readsize);
  bool sendSMS(char *smsaddr, char *smsmsg);
  bool deleteSMS(uint8_t message_index);
  bool getSMSSender(uint8_t message_index, char *sender, int senderlen);
  bool sendUSSD(char *ussdmsg, char *ussdbuff, uint16_t maxlen,
                uint16_t *readlen);

  // Time
  bool enableNetworkTimeSync(bool onoff);
  bool enableNTPTimeSync(bool onoff, FONAFlashStringPtr ntpserver = 0);
  bool getTime(char *time_buffer, uint16_t maxlen);

  // GPRS handling
  bool enableGPRS(bool onoff);
  uint8_t GPRSstate(void);
  bool getGSMLoc(uint16_t *replycode, char *buff, uint16_t maxlen);
  bool getGSMLoc(float *lat, float *lon);
  void setGPRSNetworkSettings(FONAFlashStringPtr apn,
                              FONAFlashStringPtr username = 0,
                              FONAFlashStringPtr password = 0);

  // GPS handling
  bool enableGPS(bool onoff);
  int8_t GPSstatus(void);
  uint8_t getGPS(uint8_t arg, char *buffer, uint8_t maxbuff);
  bool getGPS(float *lat, float *lon, float *speed_kph = 0, float *heading = 0,
              float *altitude = 0);
  bool enableGPSNMEA(uint8_t enable_value);

  // TCP raw connections
  bool TCPconnect(char *server, uint16_t port);
  bool TCPclose(void);
  bool TCPconnected(void);
  bool TCPsend(char *data, uint8_t len);
  uint16_t TCPavailable(void);
  uint16_t TCPread(uint8_t *buff, uint8_t len);

  // HTTP low level interface (maps directly to SIM800 commands).
  bool HTTP_init();
  bool HTTP_term();
  void HTTP_para_start(FONAFlashStringPtr parameter, bool quoted = true);
  bool HTTP_para_end(bool quoted = true);
  bool HTTP_para(FONAFlashStringPtr parameter, const char *value);
  bool HTTP_para(FONAFlashStringPtr parameter, FONAFlashStringPtr value);
  bool HTTP_para(FONAFlashStringPtr parameter, int32_t value);
  bool HTTP_data(uint32_t size, uint32_t maxTime = 10000);
  bool HTTP_action(uint8_t method, uint16_t *status, uint16_t *datalen,
                   int32_t timeout = 10000);
  bool HTTP_readall(uint16_t *datalen);
  bool HTTP_ssl(bool onoff);

  // HTTP high level interface (easier to use, less flexible).
  bool HTTP_GET_start(char *url, uint16_t *status, uint16_t *datalen);
  void HTTP_GET_end(void);
  bool HTTP_POST_start(char *url, FONAFlashStringPtr contenttype,
                       const uint8_t *postdata, uint16_t postdatalen,
                       uint16_t *status, uint16_t *datalen);
  void HTTP_POST_end(void);
  void setUserAgent(FONAFlashStringPtr useragent);

  // HTTPS
  void setHTTPSRedirect(bool onoff);

  // PWM (buzzer)
  bool setPWM(uint16_t period, uint8_t duty = 50);

  // Phone calls
  bool callPhone(char *phonenum);
  uint8_t getCallStatus(void);
  bool hangUp(void);
  bool pickUp(void);
  bool callerIdNotification(bool enable, uint8_t interrupt = 0);
  bool incomingCallNumber(char *phonenum);

  // Helper functions to verify responses.
  bool expectReply(FONAFlashStringPtr reply, uint16_t timeout = 10000);
  bool sendCheckReply(char *send, char *reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  bool sendCheckReply(FONAFlashStringPtr send, FONAFlashStringPtr reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  bool sendCheckReply(char *send, FONAFlashStringPtr reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

protected:
  int8_t _rstpin; ///< Reset pin
  uint8_t _type;  ///< Module type

  char replybuffer[255];  ///< buffer for holding replies from the module
  FONAFlashStringPtr apn; ///< Access point name
  FONAFlashStringPtr apnusername; ///< Access point username
  FONAFlashStringPtr apnpassword; ///< Access point password
  bool httpsredirect;             ///< HTTPS redirect state
  FONAFlashStringPtr useragent;   ///< User agent used when making requests
  FONAFlashStringPtr ok_reply;    ///< OK reply for successful requests

  // HTTP helpers
  bool HTTP_setup(char *url);

  void flushInput();
  uint16_t readRaw(uint16_t read_length);
  uint8_t readline(uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS,
                   bool multiline = false);
  uint8_t getReply(char *send, uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FONAFlashStringPtr send,
                   uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FONAFlashStringPtr prefix, char *suffix,
                   uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FONAFlashStringPtr prefix, int32_t suffix,
                   uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  uint8_t getReply(FONAFlashStringPtr prefix, int32_t suffix1, int32_t suffix2,
                   uint16_t timeout); // Don't set default value or else
                                      // function call is ambiguous.
  uint8_t getReplyQuoted(FONAFlashStringPtr prefix, FONAFlashStringPtr suffix,
                         uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

  bool sendCheckReply(FONAFlashStringPtr prefix, char *suffix,
                      FONAFlashStringPtr reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  bool sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix,
                      FONAFlashStringPtr reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  bool sendCheckReply(FONAFlashStringPtr prefix, int32_t suffix,
                      int32_t suffix2, FONAFlashStringPtr reply,
                      uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);
  bool sendCheckReplyQuoted(FONAFlashStringPtr prefix,
                            FONAFlashStringPtr suffix, FONAFlashStringPtr reply,
                            uint16_t timeout = FONA_DEFAULT_TIMEOUT_MS);

  bool parseReply(FONAFlashStringPtr toreply, uint16_t *v, char divider = ',',
                  uint8_t index = 0);
  bool parseReply(FONAFlashStringPtr toreply, char *v, char divider = ',',
                  uint8_t index = 0);
  bool parseReplyQuoted(FONAFlashStringPtr toreply, char *v, int maxlen,
                        char divider, uint8_t index);

  bool sendParseReply(FONAFlashStringPtr tosend, FONAFlashStringPtr toreply,
                      uint16_t *v, char divider = ',', uint8_t index = 0);

  static bool _incomingCall; ///< Incoming call state var
  static void onIncomingCall();

  FONAStreamType *mySerial; ///< Serial connection
};
/** Object that controls and keeps state for a 3G FONA module. */
class Adafruit_FONA_3G : public Adafruit_FONA {

public:
  /**
   * @brief Construct a new Adafruit_FONA_3G object
   *
   * @param reset_pin
   */
  Adafruit_FONA_3G(int8_t reset_pin) : Adafruit_FONA(reset_pin) {
    _type = FONA3G_A;
  }

  bool getBattVoltage(uint16_t *v);
  bool playToolkitTone(uint8_t t, uint16_t len);
  bool hangUp(void);
  bool pickUp(void);
  bool enableNetworkTimeSync(bool onoff);
  bool enableGPRS(bool onoff);
  bool enableGPS(bool onoff);

protected:
  bool parseReply(FONAFlashStringPtr toreply, float *f, char divider,
                  uint8_t index);
  bool parseReply(FONAFlashStringPtr toreply, uint16_t *v, char divider,
                  uint8_t index);

  bool sendParseReply(FONAFlashStringPtr tosend, FONAFlashStringPtr toreply,
                      float *f, char divider = ',', uint8_t index = 0);
  bool sendParseReply(FONAFlashStringPtr tosend, FONAFlashStringPtr toreply,
                      uint16_t *v, char divider = ',', uint8_t index = 0);
};

#endif
