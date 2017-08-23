#ifndef ADAFRUIT_FONA_HELPER_H
#define ADAFRUIT_FONA_HELPER_H


#define FONA_DEFAULT_TIMEOUT_MS 500
#define ADAFRUIT_FONA_DEBUG

#ifdef ADAFRUIT_FONA_DEBUG
  #define DEBUG_PRINT(X) std::cout << X
  #define DEBUG_PRINTLN(X) std::cout << X << std::endl
#else
  #define DEBUG_PRINT(X)
  #define DEBUG_PRINTLN(X)
#endif

// To be compatible with the Arduino library I added an F macro that doesn't do anything
#define F(X) X

#endif /* end of include guard: ADAFRUIT_FONA_HELPER_H */
