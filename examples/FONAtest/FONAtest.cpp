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
#include "../../Adafruit_FONA.h"

#define FONA_RST 48

// this is a large buffer for replies
char replybuffer[255];



mraa::Uart *fonaSerial;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

uint8_t readconsole(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
void printMenu(void);
void flushSerial(void);
uint16_t readnumber();
long map(long x, long in_min, long in_max, long out_min, long out_max);

uint8_t type;

void setup() {

  try {
      fonaSerial = new mraa::Uart("/dev/ttyMFD1");
  } catch (std::exception& e) {
      std::cout << "Error while setting up raw UART, do you have a uart?" << std::endl;
      std::terminate();
  }

  int32_t baudrate = 921600;


  fonaSerial->setBaudRate(baudrate);
  fonaSerial->setMode(8, mraa::UART_PARITY_NONE, 1);
  fonaSerial->setFlowcontrol(false, false);

  std::cout << "First trying " << baudrate <<  " baud" << '\n';
  std::cout << "FONA basic test" << std::endl;
  std::cout << "Initializing....(May take 3 seconds)" << std::endl;

  if (! fona.begin(*fonaSerial)) {
    std::cout << "Couldn't find FONA" << std::endl;
    while (1);
  }
  type = fona.type();
  std::cout << "FONA is OK" << std::endl;
  std::cout << "Found ";
  switch (type) {
    case FONA800L:
      std::cout << "FONA 800L" << std::endl; break;
    case FONA800H:
      std::cout << "FONA 800H" << std::endl; break;
    case FONA808_V1:
      std::cout << "FONA 808 (v1)" << std::endl; break;
    case FONA808_V2:
      std::cout << "FONA 808 (v2)" << std::endl; break;
    case FONA3G_A:
      std::cout << "FONA 3G (American)" << std::endl; break;
    case FONA3G_E:
      std::cout << "FONA 3G (European)" << std::endl; break;
    default:
      std::cout << "???" << std::endl; break;
  }

  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    std::cout << "Module IMEI: "; std::cout << imei << std::endl;
  }

  // Optionally configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  //fona.setGPRSNetworkSettings(F("your APN"), F("your username"), F("your password"));

  // Optionally configure HTTP gets to follow redirects over SSL.
  // Default is not to follow SSL redirects, however if you uncomment
  // the following line then redirects over SSL will be followed.
  //fona.setHTTPSRedirect(true);

  printMenu();
}

void printMenu(void) {
  std::cout << "-------------------------------------" << std::endl;
  std::cout << "[?] Print this menu" << std::endl;
  std::cout << "[a] read the ADC 2.8V max (FONA800 & 808)" << std::endl;
  std::cout << "[b] read the Battery V and % charged" << std::endl;
  std::cout << "[C] read the SIM CCID" << std::endl;
  std::cout << "[U] Unlock SIM with PIN code" << std::endl;
  std::cout << "[i] read RSSI" << std::endl;
  std::cout << "[n] get Network status" << std::endl;
  std::cout << "[v] set audio Volume" << std::endl;
  std::cout << "[V] get Volume" << std::endl;
  std::cout << "[H] set Headphone audio (FONA800 & 808)" << std::endl;
  std::cout << "[e] set External audio (FONA800 & 808)" << std::endl;
  std::cout << "[T] play audio Tone" << std::endl;
  std::cout << "[P] PWM/Buzzer out (FONA800 & 808)" << std::endl;

  // FM (SIM800 only!)
  std::cout << "[f] tune FM radio (FONA800)" << std::endl;
  std::cout << "[F] turn off FM (FONA800)" << std::endl;
  std::cout << "[m] set FM volume (FONA800)" << std::endl;
  std::cout << "[M] get FM volume (FONA800)" << std::endl;
  std::cout << "[q] get FM station signal level (FONA800)" << std::endl;

  // Phone
  std::cout << "[c] make phone Call" << std::endl;
  std::cout << "[A] get call status" << std::endl;
  std::cout << "[h] Hang up phone" << std::endl;
  std::cout << "[p] Pick up phone" << std::endl;

  // SMS
  std::cout << "[N] Number of SMSs" << std::endl;
  std::cout << "[r] Read SMS #" << std::endl;
  std::cout << "[R] Read All SMS" << std::endl;
  std::cout << "[d] Delete SMS #" << std::endl;
  std::cout << "[s] Send SMS" << std::endl;
  std::cout << "[u] Send USSD" << std::endl;

  // Time
  std::cout << "[y] Enable network time sync (FONA 800 & 808)" << std::endl;
  std::cout << "[Y] Enable NTP time sync (GPRS FONA 800 & 808)" << std::endl;
  std::cout << "[t] Get network time" << std::endl;

  // GPRS
  std::cout << "[G] Enable GPRS" << std::endl;
  std::cout << "[g] Disable GPRS" << std::endl;
  std::cout << "[l] Query GSMLOC (GPRS)" << std::endl;
  std::cout << "[w] Read webpage (GPRS)" << std::endl;
  std::cout << "[W] Post to website (GPRS)" << std::endl;

  // GPS
  if ((type == FONA3G_A) || (type == FONA3G_E) || (type == FONA808_V1) || (type == FONA808_V2)) {
    std::cout << "[O] Turn GPS on (FONA 808 & 3G)" << std::endl;
    std::cout << "[o] Turn GPS off (FONA 808 & 3G)" << std::endl;
    std::cout << "[L] Query GPS location (FONA 808 & 3G)" << std::endl;
    if (type == FONA808_V1) {
      std::cout << "[x] GPS fix status (FONA808 v1 only)" << std::endl;
    }
    std::cout << "[E] Raw NMEA out (FONA808)" << std::endl;
  }

  std::cout << "[S] create Serial passthru tunnel" << std::endl;
  std::cout << "[z] Send AT commmand to Fona" << '\n';
  std::cout << "-------------------------------------" << std::endl;
  std::cout << "" << std::endl;

}
void loop() {
  std::cout << "FONA> ";
  std::string cinbuffer;

  // while (! Serial.available() ) {
  while ( cinbuffer.empty() ) {
    std::getline(std::cin, cinbuffer);
    if (fona.available()) {
      // Serial.write(fona.read());
      std::cout << static_cast<char>(fona.read());
    }
  }

  // char command = Serial.read();
  char command = cinbuffer[0];
  std::cout << command << std::endl;


  switch (command) {
    case '?': {
        printMenu();
        break;
      }

    case 'a': {
        // read the ADC
        uint16_t adc;
        if (! fona.getADCVoltage(&adc)) {
          std::cout << "Failed to read ADC" << std::endl;
        } else {
          std::cout << "ADC = "; std::cout << adc; std::cout << " mV" << std::endl;
        }
        break;
      }

    case 'b': {
        // read the battery voltage and percentage
        uint16_t vbat;
        if (! fona.getBattVoltage(&vbat)) {
          std::cout << "Failed to read Batt" << std::endl;
        } else {
          std::cout << "VBat = "; std::cout << vbat; std::cout << " mV" << std::endl;
        }


        if (! fona.getBattPercent(&vbat)) {
          std::cout << "Failed to read Batt" << std::endl;
        } else {
          std::cout << "VPct = "; std::cout << vbat; std::cout << "%" << std::endl;
        }

        break;
      }

    case 'U': {
        // Unlock the SIM with a PIN code
        char PIN[5];
        flushSerial();
        std::cout << "Enter 4-digit PIN" << std::endl;
        readconsole(PIN, 3);
        std::cout << PIN << std::endl;
        std::cout << "Unlocking SIM card: ";
        if (! fona.unlockSIM(PIN)) {
          std::cout << "Failed" << std::endl;
        } else {
          std::cout << "OK!" << std::endl;
        }
        break;
      }

    case 'C': {
        // read the CCID
        fona.getSIMCCID(replybuffer);  // make sure replybuffer is at least 21 bytes!
        std::cout << "SIM CCID = "; std::cout << replybuffer << std::endl;
        break;
      }

    case 'i': {
        // read the RSSI
        uint8_t n = fona.getRSSI();
        int8_t r;

        std::cout << "RSSI = "<< static_cast<uint16_t>(n) << ": ";
        if (n == 0) r = -115;
        if (n == 1) r = -111;
        if (n == 31) r = -52;
        if ((n >= 2) && (n <= 30)) {
          r = map(n, 2, 30, -110, -54);
        }
        std::cout << static_cast<int16_t>(r) << " dBm" << std::endl;

        break;
      }

    case 'n': {
        // read the network/cellular status
        uint8_t n = fona.getNetworkStatus();
        std::cout << "Network status ";
        std::cout << static_cast<int16_t>(n);
        std::cout << ": ";
        if (n == 0) std::cout << "Not registered" << std::endl;
        if (n == 1) std::cout << "Registered (home)" << std::endl;
        if (n == 2) std::cout << "Not registered (searching)" << std::endl;
        if (n == 3) std::cout << "Denied" << std::endl;
        if (n == 4) std::cout << "Unknown" << std::endl;
        if (n == 5) std::cout << "Registered roaming" << std::endl;
        break;
      }

    /*** Audio ***/
    // case 'v': {
    //     // set volume
    //     flushSerial();
    //     if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
    //       std::cout << "Set Vol [0-8] ";
    //     } else {
    //       std::cout << "Set Vol % [0-100] ";
    //     }
    //     uint8_t vol = readnumber();
    //     std::cout << std::endl;
    //     if (! fona.setVolume(vol)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "OK!" << std::endl;
    //     }
    //     break;
    //   }

    // case 'V': {
    //     uint8_t v = fona.getVolume();
    //     std::cout << v;
    //     if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
    //       std::cout << " / 8" << std::endl;
    //     } else {
    //       std::cout << "%" << std::endl;
    //     }
    //     break;
    //   }

    // case 'H': {
    //     // Set Headphone output
    //     if (! fona.setAudio(FONA_HEADSETAUDIO)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "OK!" << std::endl;
    //     }
    //     fona.setMicVolume(FONA_HEADSETAUDIO, 15);
    //     break;
    //   }
    // case 'e': {
    //     // Set External output
    //     if (! fona.setAudio(FONA_EXTAUDIO)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "OK!" << std::endl;
    //     }
    //
    //     fona.setMicVolume(FONA_EXTAUDIO, 10);
    //     break;
    //   }

    // case 'T': {
    //     // play tone
    //     flushSerial();
    //     std::cout << "Play tone #";
    //     uint8_t kittone = readnumber();
    //     std::cout << std::endl;
    //     // play for 1 second (1000 ms)
    //     if (! fona.playToolkitTone(kittone, 1000)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "OK!" << std::endl;
    //     }
    //     break;
    //   }

    /*** FM Radio ***/

    // case 'f': {
    //     // get freq
    //     flushSerial();
    //     std::cout << "FM Freq (eg 1011 == 101.1 MHz): ";
    //     uint16_t station = readnumber();
    //     std::cout << std::endl;
    //     // FM radio ON using headset
    //     if (fona.FMradio(true, FONA_HEADSETAUDIO)) {
    //       std::cout << "Opened" << std::endl;
    //     }
    //     if (! fona.tuneFMradio(station)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "Tuned" << std::endl;
    //     }
    //     break;
    //   }
    // case 'F': {
    //     // FM radio off
    //     if (! fona.FMradio(false)) {
    //       std::cout << "Failed" << std::endl;
    //     } else {
    //       std::cout << "OK!" << std::endl;
    //     }
    //     break;
    //   }
//     case 'm': {
//         // Set FM volume.
//         flushSerial();
//         std::cout << "Set FM Vol [0-6]:";
//         uint8_t vol = readnumber();
//         std::cout << std::endl;
//         if (!fona.setFMVolume(vol)) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "OK!" << std::endl;
//         }
//         break;
//       }
//     case 'M': {
//         // Get FM volume.
//         uint8_t fmvol = fona.getFMVolume();
//         if (fmvol < 0) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "FM volume: ";
//           std::cout << fmvol << std::cout;
//         }
//         break;
//       }
//     case 'q': {
//         // Get FM station signal level (in decibels).
//         flushSerial();
//         std::cout << "FM Freq (eg 1011 == 101.1 MHz): ";
//         uint16_t station = readnumber();
//         std::cout << std::endl;
//         int8_t level = fona.getFMSignalLevel(station);
//         if (level < 0) {
//           std::cout << "Failed! Make sure FM radio is on (tuned to station)." << std::endl;
//         } else {
//           std::cout << "Signal level (dB): ";
//           std::cout << level << std::endl;
//         }
//         break;
//       }
//
//     /*** PWM ***/
//
//     case 'P': {
//         // PWM Buzzer output @ 2KHz max
//         flushSerial();
//         std::cout << "PWM Freq, 0 = Off, (1-2000): ";
//         uint16_t freq = readnumber();
//         std::cout << std::endl;
//         if (! fona.setPWM(freq)) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "OK!" << std::endl;
//         }
//         break;
//       }
//
//     /*** Call ***/
//     case 'c': {
//         // call a phone!
//         char number[30];
//         flushSerial();
//         std::cout << "Call #";
//         readconsole(number, 30);
//         std::cout << std::endl;
//         std::cout << "Calling "; std::cout << number << std::endl;
//         if (!fona.callPhone(number)) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "Sent!" << std::endl;
//         }
//
//         break;
//       }
//     case 'A': {
//         // get call status
//         int8_t callstat = fona.getCallStatus();
//         switch (callstat) {
//           case 0: std::cout << "Ready" << std::endl; break;
//           case 1: std::cout << "Could not get status" << std::endl; break;
//           case 3: std::cout << "Ringing (incoming)" << std::endl; break;
//           case 4: std::cout << "Ringing/in progress (outgoing)" << std::endl; break;
//           default: std::cout << "Unknown" << std::endl; break;
//         }
//         break;
//       }
//
//     case 'h': {
//         // hang up!
//         if (! fona.hangUp()) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "OK!" << std::endl;
//         }
//         break;
//       }
//
//     case 'p': {
//         // pick up!
//         if (! fona.pickUp()) {
//           std::cout << "Failed" << std::endl;
//         } else {
//           std::cout << "OK!" << std::endl;
//         }
//         break;
//       }
//
    /*** SMS ***/

    case 'N': {
        // read the number of SMS's!
        int8_t smsnum = fona.getNumSMS();
        if (smsnum < 0) {
          std::cout << "Could not read # SMS" << std::endl;
        } else {
          std::cout << static_cast<int>(smsnum);
          std::cout << " SMS's on SIM card!" << std::endl;
        }
        break;
      }
    case 'r': {
        // read an SMS
        flushSerial();
        std::cout << "Read #";
        uint8_t smsn = readnumber();
        std::cout << "\n\rReading SMS #" << static_cast<uint16_t>(smsn) << std::endl;

        // Retrieve SMS sender address/phone number.
        if (! fona.getSMSSender(smsn, replybuffer, 250)) {
          std::cout << "Failed!" << std::endl;
          break;
        }
        std::cout << "FROM: "; std::cout << replybuffer << std::endl;

        // Retrieve SMS value.
        uint16_t smslen;
        if (! fona.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
          std::cout << "Failed!" << std::endl;
          break;
        }
        std::cout << "***** SMS #"; std::cout << smsn;
        std::cout << " ("; std::cout << smslen; std::cout << ") bytes *****" << std::endl;
        std::cout << replybuffer << std::endl;
        std::cout << "*****" << std::endl;

        break;
      }
    case 'R': {
        // read all SMS
        int8_t smsnum = fona.getNumSMS();
        uint16_t smslen;
        int8_t smsn;

        if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
          smsn = 0; // zero indexed
          smsnum--;
        } else {
          smsn = 1;  // 1 indexed
        }

        for ( ; smsn <= smsnum; smsn++) {
          std::cout << "\n\rReading SMS #" << static_cast<int16_t>(smsn) << std::endl;
          if (!fona.readSMS(smsn, replybuffer, 250, &smslen)) {  // pass in buffer and max len!
            std::cout << "Failed!" << std::endl;
            break;
          }
          // if the length is zero, its a special case where the index number is higher
          // so increase the max we'll look at!
          if (smslen == 0) {
            std::cout << "[empty slot]" << std::endl;
            smsnum++;
            continue;
          }

          std::cout << "***** SMS #" << static_cast<int16_t>(smsn);
          std::cout << " (" << smslen << ") bytes *****" << std::endl;
          std::cout << replybuffer << std::endl;
          std::cout << "*****" << std::endl;
        }
        break;
      }

    case 'd': {
        // delete an SMS
        flushSerial();
        std::cout << "Delete #";
        uint8_t smsn = readnumber();

        std::cout << "\n\rDeleting SMS #" << static_cast<uint16_t>(smsn) << std::endl;
        if (fona.deleteSMS(smsn)) {
          std::cout << "OK!" << std::endl;
        } else {
          std::cout << "Couldn't delete" << std::endl;
        }
        break;
      }

    case 's': {
        // send an SMS!
        char sendto[21], message[141];
        flushSerial();
        std::cout << "Send to #";
        readconsole(sendto, 20);
        std::cout << sendto << std::endl;
        std::cout << "Type out one-line message (140 char): ";
        readconsole(message, 140);
        std::cout << message << std::endl;
        if (!fona.sendSMS(sendto, message)) {
          std::cout << "Failed" << std::endl;
        } else {
          std::cout << "Sent!" << std::endl;
        }

        break;
      }

    // case 'u': {
    //   // send a USSD!
    //   char message[141];
    //   flushSerial();
    //   std::cout << "Type out one-line message (140 char): ";
    //   readconsole(message, 140);
    //   std::cout << message << std::endl;
    //
    //   uint16_t ussdlen;
    //   if (!fona.sendUSSD(message, replybuffer, 250, &ussdlen)) { // pass in buffer and max len!
    //     std::cout << "Failed" << std::endl;
    //   } else {
    //     std::cout << "Sent!" << std::endl;
    //     std::cout << "***** USSD Reply";
    //     std::cout << " ("; std::cout << ussdlen; std::cout << ") bytes *****" << std::endl;
    //     std::cout << replybuffer << std::endl;
    //     std::cout << "*****" << std::endl;
    //   }
    // }

//     /*** Time ***/
//
//     case 'y': {
//         // enable network time sync
//         if (!fona.enableNetworkTimeSync(true))
//           std::cout << "Failed to enable" << std::endl;
//         break;
//       }
//
//     case 'Y': {
//         // enable NTP time sync
//         if (!fona.enableNTPTimeSync(true, F("pool.ntp.org")))
//           std::cout << "Failed to enable" << std::endl;
//         break;
//       }
//
//     case 't': {
//         // read the time
//         char buffer[23];
//
//         fona.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
//         std::cout << "Time = "; std::cout << buffer << std::endl;
//         break;
//       }
//
//
//     /*********************************** GPS (SIM808 only) */
//
//     case 'o': {
//         // turn GPS off
//         if (!fona.enableGPS(false))
//           std::cout << "Failed to turn off" << std::endl;
//         break;
//       }
//     case 'O': {
//         // turn GPS on
//         if (!fona.enableGPS(true))
//           std::cout << "Failed to turn on" << std::endl;
//         break;
//       }
//     case 'x': {
//         int8_t stat;
//         // check GPS fix
//         stat = fona.GPSstatus();
//         if (stat < 0)
//           std::cout << "Failed to query" << std::endl;
//         if (stat == 0) std::cout << "GPS off" << std::endl;
//         if (stat == 1) std::cout << "No fix" << std::endl;
//         if (stat == 2) std::cout << "2D fix" << std::endl;
//         if (stat == 3) std::cout << "3D fix" << std::endl;
//         break;
//       }
//
//     case 'L': {
//         // check for GPS location
//         char gpsdata[120];
//         fona.getGPS(0, gpsdata, 120);
//         if (type == FONA808_V1)
//           std::cout << "Reply in format: mode,longitude,latitude,altitude,utctime(yyyymmddHHMMSS),ttff,satellites,speed,course" << std::endl;
//         else
//           std::cout << "Reply in format: mode,fixstatus,utctime(yyyymmddHHMMSS),latitude,longitude,altitude,speed,course,fixmode,reserved1,HDOP,PDOP,VDOP,reserved2,view_satellites,used_satellites,reserved3,C/N0max,HPA,VPA" << std::endl;
//         std::cout << gpsdata << std::endl;
//
//         break;
//       }
//
//     case 'E': {
//         flushSerial();
//         if (type == FONA808_V1) {
//           std::cout << "GPS NMEA output sentences (0 = off, 34 = RMC+GGA, 255 = all)";
//         } else {
//           std::cout << "On (1) or Off (0)? ";
//         }
//         uint8_t nmeaout = readnumber();
//
//         // turn on NMEA output
//         fona.enableGPSNMEA(nmeaout);
//
//         break;
//       }
//
    /*********************************** GPRS */

    case 'g': {
        // turn GPRS off
        if (!fona.enableGPRS(false))
          std::cout << "Failed to turn off" << std::endl;
        break;
      }
    case 'G': {
        // turn GPRS on
        if (!fona.enableGPRS(true))
          std::cout << "Failed to turn on" << std::endl;
        break;
      }
//     case 'l': {
//         // check for GSMLOC (requires GPRS)
//         uint16_t returncode;
//
//         if (!fona.getGSMLoc(&returncode, replybuffer, 250))
//           std::cout << "Failed!" << std::endl;
//         if (returncode == 0) {
//           std::cout << replybuffer << std::endl;
//         } else {
//           std::cout << "Fail code #"; std::cout << returncode << std::endl;
//         }
//
//         break;
//       }

    case 'w': {
      // read website URL via get request
      std::string website = "www.httpbin.org";
      std::string get_path = "/get";
      // std::string website = "www.adafruit.com";

      //TODO: implement new methods
      //TODO: ErrorHandling
      fona.HTTPS_start(website);
      fona.HTTPS_GET(website, get_path);
      fona.HTTPS_end();

      break;
    }

    case 'W': {
      // send post request to website
      std::string website = "www.httpbin.org";
      std::string post_path = "/post";
      std::string post_string = "I want this string to be posted";
      fona.HTTPS_start(website);
      fona.HTTPS_POST(website, post_path, post_string);
      fona.HTTPS_end();

      break;
    }

//     case 'w': {
//         // read website URL
//         uint16_t statuscode;
//         int16_t length;
//         char url[80];
//
//         flushSerial();
//         std::cout << "NOTE: in beta! Use small webpages to read!" << std::endl;
//         std::cout << "URL to read (e.g. www.adafruit.com/testwifi/index.html):" << std::endl;
//         std::cout << "http://"; readconsole(url, 79);
//         std::cout << url << std::endl;
//
//         std::cout << "****" << std::endl;
//         if (!fona.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
//           std::cout << "Failed!" << std::endl;
//           break;
//         }
//         while (length > 0) {
//           while (fona.available()) {
//             char c = fona.read();
//
//             // Serial.write is too slow, we'll write directly to Serial register!
// #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//             loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
//             UDR0 = c;
// #else
//             // Serial.write(c);
//             std::cout << static_cast<char>(c);
// #endif
//             length--;
//             if (! length) break;
//           }
//         }
//         std::cout << "\n****" << std::endl;
//         fona.HTTP_GET_end();
//         break;
//       }

//     case 'W': {
//         // Post data to website
//         uint16_t statuscode;
//         int16_t length;
//         char url[80];
//         char data[80];
//
//         flushSerial();
//         std::cout << "NOTE: in beta! Use simple websites to post!" << std::endl;
//         std::cout << "URL to post (e.g. httpbin.org/post):" << std::endl;
//         std::cout << "http://"; readconsole(url, 79);
//         std::cout << url << std::endl;
//         std::cout << "Data to post (e.g. \"foo\" or \"{\"simple\":\"json\"}\"):" << std::endl;
//         readconsole(data, 79);
//         std::cout << data << std::endl;
//
//         std::cout << "****" << std::endl;
//         if (!fona.HTTP_POST_start(url, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
//           std::cout << "Failed!" << std::endl;
//           break;
//         }
//         while (length > 0) {
//           while (fona.available()) {
//             char c = fona.read();
//
// #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//             loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
//             UDR0 = c;
// #else
//             // Serial.write(c);
//             std::cout << static_cast<char>(c);
// #endif
//
//             length--;
//             if (! length) break;
//           }
//         }
//         std::cout << "\n****" << std::endl;
//         fona.HTTP_POST_end();
//         break;
//       }
//     /*****************************************/
//
//     case 'S': {
//         std::cout << "Creating SERIAL TUBE" << std::endl;
//         cinbuffer = "";
//         while (1) {
//           // while (Serial.available()) {
//           //   delay(1);
//           //   fona.write(Serial.read());
//           // }
//           std::getline(std::cin, cinbuffer);
//           if(!cinbuffer.empty()){
//             delay(1);
//             for ( auto it = cinbuffer.begin(); it != cinbuffer.end(); ++it){
//               fona.write(*it);
//             }
//           }
//           if (fona.available()) {
//             // Serial.write(fona.read());
//             std::cout << static_cast<char>(fona.read());
//           }
//         }
//         break;
//       }

    case 'z': {
      // Enter raw commands and send to Fona
      std::string rawCommand;

      std::cout << "Enter AT command to be sent to Fona: ";
      std::getline(std::cin, rawCommand);

      fona.execCommand(rawCommand);

      break;
    }

    default: {
        std::cout << "Unknown command" << std::endl;
        printMenu();
        break;
      }
  }
  // flush input
  flushSerial();
  while (fona.available()) {
    // Serial.write(fona.read());
    std::cout << static_cast<char>(fona.read());
  }

}

// flush whatever might be in the stdin buffer
void flushSerial() {
  // while (Serial.available())
  //   Serial.read();
  // std::string cinbuffer;
  // std::getline(std::cin, cinbuffer);

}

// char readBlocking() {
//   while (!Serial.available());
//   return Serial.read();
// }
uint16_t readnumber() {
  uint16_t x = 0;
  std::string cinbuffer;
  // char c;
  // while (! isdigit(c = readBlocking())) {
  //   //std::cout << c;
  // }
  // std::cout << c;
  // x = c - '0';
  // while (isdigit(c = readBlocking())) {
  //   std::cout << c;
  //   x *= 10;
  //   x += c - '0';
  // }

  while (1){
    std::getline(std::cin, cinbuffer);
    if (! cinbuffer.empty() ){
      for ( auto it = cinbuffer.begin(); it != cinbuffer.end(); ++it){
        if(isdigit(*it)){
          std::cout << *it;
          x *= 10;
          x += *it - '0';
        }
      }
      break;
    }
    delay(1);
  }
  return x;
}

uint8_t readconsole(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  std::string cinbuffer;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    // if (buffidx > maxbuff) {
    //   //std::cout << "SPACE" << std::endl;
    //   break;
    // }

    // while (Serial.available())
    std::getline(std::cin, cinbuffer);
    if(! cinbuffer.empty() ){
      // for ( auto it = cinbuffer.begin(); it != cinbuffer.end(); ++it){
      //   char c =  *it;
      //   std::cout << "c: " << c << std::endl;
      //   //std::cout << c, HEX; std::cout << "#"; std::cout << c << std::endl;
      //
      //   if (c == '\r') {
      //     std::cout << "match \'\\r\'" << std::endl;
      //     continue;}
      //   if (c == 0xA) {
      //     if (buffidx == 0)   // the first 0x0A is ignored
      //       continue;
      //
      //     timeout = 0;         // the second 0x0A is the end of the line
      //     timeoutvalid = true;
      //     break;
      //   }
      //   buff[buffidx] = c;
      //   buffidx++;
      // }
      if(static_cast<uint16_t>(cinbuffer.size()) > (maxbuff+1)) break;
      strcpy(buff, cinbuffer.c_str());
      timeout = 0;
      timeoutvalid = true;

    }

    if (timeoutvalid && timeout == 0) {
      //std::cout << "TIMEOUT" << std::endl;
      break;
    }
    delay(1);
  }
  // buff[buffidx] = 0;  // null term
  buffidx = strlen(buff);
  return buffidx;
}

long map(long x, long in_min, long in_max, long out_min, long out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


int main(int argc, char const *argv[]) {
  setup();
  while(1) loop();
  return 0;
}
