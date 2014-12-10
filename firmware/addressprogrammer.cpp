#include "WProgram.h"
#include "pins_arduino.h"
#include "addressprogrammer.h"
#include "blinkytile.h"


#define BIT_LENGTH      4       // Length of a bit, in microseconds
#define BREAK_LENGTH    5000    // Length of the break, in microseconds
#define MAB_LENGTH      88      // Length of the mark after break, in microseconds


uint8_t flipEndianness(uint8_t input);
void sendAddressData(uint8_t* dataArray, int length);

uint8_t flipEndianness(uint8_t input) {
  uint8_t output = 0;
  for(uint8_t bit = 0; bit < 8; bit++) {
    if(input & (1 << bit)) {
      output |= (1 << (7 - bit));
    }
  }
  
  return output;
}


// Send a DMX frame with new data
void sendAddressData(uint8_t* dataArray, int length) {
  
  digitalWriteFast(ADDRESS_PIN, LOW);    // Break - 88us
  delayMicroseconds(BREAK_LENGTH);

  digitalWriteFast(ADDRESS_PIN, HIGH);   // MAB - 8 us
  delayMicroseconds(MAB_LENGTH);

  
  // For each address
  for(int frame = 0; frame < length; frame++) {    

    noInterrupts();
    
    digitalWriteFast(ADDRESS_PIN, LOW);    // Start bit
    delayMicroseconds(BIT_LENGTH);
    
    for(int bit = 0; bit < 8; bit++) {  // data bits
      digitalWriteFast(ADDRESS_PIN, (dataArray[frame] >> bit) & 0x01);
      delayMicroseconds(BIT_LENGTH);
    }
    
    digitalWriteFast(ADDRESS_PIN, HIGH);    // Stop bit
    delayMicroseconds(2*BIT_LENGTH);
    
    interrupts();
  }

//  digitalWriteFast(ADDRESS_PIN, LOW);    // Stop bit
//  delayMicroseconds(20);
//  digitalWriteFast(ADDRESS_PIN, HIGH);    // Stop bit  
  // We're done - MTBP is high, same as the stop bit.
  
//  interrupts();
}


void programAddress(int address) {
  // Build the output pattern to program this address
  #define patternLength 4
  uint8_t pattern[patternLength];
  int controllerType = 1;
  
  if (controllerType ==  1) {
    // WS2822S (from datasheet)
    int channel = (address-1)*3+1;
    pattern[0] = 0; // start code
    pattern[1] = flipEndianness(channel%256);
    pattern[2] = flipEndianness(240 - (channel/256)*15);
    pattern[3] = flipEndianness(0xD2);
  }
  else {
    // WS2821 (determined experimentally)
    int channel = (address)*3;
    pattern[0] = 0; // start code
    pattern[1] = flipEndianness(channel%256);
    pattern[2] = flipEndianness(240 - (channel/256)*15);
    pattern[3] = flipEndianness(0xAE);    
  }

  // Pull address high and data low to signal address programming start
  digitalWriteFast(ADDRESS_PIN, HIGH);
  digitalWriteFast(DATA_PIN, LOW);
  delay(50);
  watchdog_refresh();

  // send program data out
  sendAddressData(pattern, patternLength);
  

  // pull address high again in the end
  // TODO: set low instead?
  digitalWriteFast(ADDRESS_PIN, HIGH);

  // Show white for a short time
  for(int i = 0; i < 10; i++) {
    delay(100);
    watchdog_refresh();
  }

  // toggle power
  disableOutputPower();
  delay(200);
  enableOutputPower();
}
