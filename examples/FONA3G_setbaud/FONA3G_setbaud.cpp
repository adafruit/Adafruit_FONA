/*
 * Author: Brendan Le Foll <brendan.le.foll@intel.com>
 * Copyright (c) 2015 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <unistd.h>
#include <iostream>
#include <exception>

#include "../../Adafruit_FONA.h"

#define FONA_RST 48


mraa::Uart *fonaSerial;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);


int main()
{

  //! [Interesting]
  // If you have a valid platform configuration use numbers to represent uart
  // device. If not use raw mode where std::string is taken as a constructor
  // parameter

  //TODO: put declaration fonaSerial in object

  // try {
  //     fonaSerial = new mraa::Uart(1);
  // } catch (std::exception& e) {
  //     std::cout << e.what() << ", likely invalid platform config" << std::endl;
  // }

  try {
      fonaSerial = new mraa::Uart("/dev/ttyMFD1");
  } catch (std::exception& e) {
      std::cout << "Error while setting up raw UART, do you have a uart?" << std::endl;
      std::terminate();
  }

  int32_t baudrate = 115200;

  std::cout << "First trying " << baudrate << " baud" << '\n';

  if (fonaSerial->setBaudRate(baudrate) != mraa::SUCCESS) {
      std::cout << "Error setting baudrate on UART" << std::endl;
  }

  if (fonaSerial->setMode(8, mraa::UART_PARITY_NONE, 1) != mraa::SUCCESS) {
      std::cout << "Error setting parity on UART" << std::endl;
  }

  if (fonaSerial->setFlowcontrol(false, false) != mraa::SUCCESS) {
      std::cout << "Error setting flow control UART" << std::endl;
  }

  // fonaSerial->writeStr("Hello monkeys");
  if (! fona.begin(*fonaSerial) ){
    std::cout << "Connecting with " << baudrate << "baud did not work." << std::endl;
  }
  else {
    std::cout << "FONA is OK" << std::endl;
    std::cout << "Connecting fona with " << baudrate << " baud did work"<< '\n';
  }
  if(true){
    baudrate = 921600;
    std::cout << "Now trying " << baudrate << " baud" << '\n';

    // send the command to reset the baud rate
    if(! fona.setBaudrate(baudrate)){
      std::cout << "Error setting baudrate on FONA" << '\n';
    }
    // fona.readOut();
    // restart with 4800 baud
    if (fonaSerial->setBaudRate(baudrate) != mraa::SUCCESS) {
        std::cout << "Error setting baudrate on UART" << std::endl;
    }
    std::cout << "Initializing  @" << baudrate << "baud..." << std::endl;

    if (! fona.begin(*fonaSerial)) {
      std::cout << "Couldn't find FONA" << '\n';
    }
    else {
      std::cout << "FONA is OK" << std::endl;
    }
  }

  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    std::cout << "Module IMEI: " << imei << '\n';
  }

  delete fonaSerial;

  return mraa::SUCCESS;
}
