#ifndef CUSTOM_CHARS_H
#define CUSTOM_CHARS_H

#include <avr/pgmspace.h>  // Required to store bitmaps in flash memory (PROGMEM)

// Define a 8x8 custom character bitmap
const uint8_t alarmOnIcon[] PROGMEM = {
  B00011000,
  B00111100,
  B01111110,
  B01111110,
  B01111110,
  B01111110,
  B11111111,
  B00011000
};

const uint8_t alarmOffIcon[] PROGMEM = {
  B00011000,
  B00111100,
  B01111010,
  B01110110,
  B01101110,
  B01011110,
  B10111111,
  B00011000
};

const uint8_t playIcon[] PROGMEM = {
  B00000000,
  B10000000,
  B11000000,
  B11100000,
  B11110000,
  B11100000,
  B11000000,
  B10000000
};

const uint8_t backIcon[] PROGMEM = {
  B00100000,
  B01100000,
  B11111110,
  B01100011,
  B00100001,
  B00000001,
  B00000011,
  B00111110
};


// Archived arrow icons that were originally used for displaying arrows above the time when setting the numbers with + and -

// const uint8_t upIcon[] PROGMEM = {
//   B00010000,
//   B00101000,
//   B01000100,
//   B10000010,
//   B00000000,
//   B00000000,
//   B00000000,
//   B00000000
// };

// const uint8_t downIcon[] PROGMEM = {
//   B10000010,
//   B01000100,
//   B00101000,
//   B00010000,
//   B00000000,
//   B00000000,
//   B00000000,
//   B00000000
// };

#endif  // CUSTOM_CHARS_H
