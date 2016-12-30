#include "WProgram.h"
#include "pins_arduino.h"
#include "addressprogrammer.h"
#include "blinkytile.h"

static volatile uint8_t *dmxPort;
static uint8_t dmxBit = 0;


#define BIT_LENGTH      4       // Length of a bit, in microseconds
#define BREAK_LENGTH    10000    // Length of the break, in microseconds (min: 5000)
#define MAB_LENGTH      12      // Length of the mark after break, in microseconds (typical: 12)

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

void sendByteDelay(uint8_t data) {
    noInterrupts();
    
    digitalWriteFast(ADDRESS_PIN, LOW);    // Start bit
    delayMicroseconds(BIT_LENGTH);
    
    for(int bit = 0; bit < 8; bit++) {  // data bits
        digitalWriteFast(ADDRESS_PIN, (data >> bit) & 0x01);
        delayMicroseconds(BIT_LENGTH);
    }
    
    digitalWriteFast(ADDRESS_PIN, HIGH);        // Stop bit
    delayMicroseconds(2*BIT_LENGTH);
    
    interrupts();
}

void sendByte(uint8_t value)
{
    uint32_t begin, target;
    uint8_t mask;

    noInterrupts();
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    begin = ARM_DWT_CYCCNT;
    *dmxPort = 0;
    target = F_CPU / 250000;
    while (ARM_DWT_CYCCNT - begin < target) ; // wait, start bit
    for (mask=1; mask; mask <<= 1) {
        *dmxPort = (value & mask) ? 1 : 0;
        target += (F_CPU / 250000);
        while (ARM_DWT_CYCCNT - begin < target) ; // wait, data bits
    }
    *dmxPort = 1;
    target += (F_CPU / 125000);
    while (ARM_DWT_CYCCNT - begin < target) ; // wait, 2 stops bits
    interrupts();
}


// Send a DMX frame with new data
void sendAddressData(uint8_t* dataArray, int length) {
    
    digitalWriteFast(ADDRESS_PIN, LOW);
    delayMicroseconds(BREAK_LENGTH);

    digitalWriteFast(ADDRESS_PIN, HIGH);
    delayMicroseconds(MAB_LENGTH);

  
    // For each address
    for(int frame = 0; frame < length; frame++) {    

        sendByte(dataArray[frame]);

    }
}


void programAddress(int address) {
    // Put pins in GPIO mode
    digitalWrite(DATA_PIN, HIGH);
    digitalWrite(ADDRESS_PIN, HIGH);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(ADDRESS_PIN, OUTPUT);

    // Set up port pointers for interrupt routine
    dmxPort = portOutputRegister(digitalPinToPort(ADDRESS_PIN));
    dmxBit = digitalPinToBitMask(ADDRESS_PIN);

    // Build the output pattern to program this address
    #define patternLength 4
    uint8_t pattern[patternLength];

    enum LedType {
        WS2821      = 0,
        WS2822S     = 1,
    };

    LedType ledType = WS2821;
    
    if (ledType == WS2821) {
        // WS2821 (determined experimentally)
        int channel = (address)*3 + 1;
        pattern[0] = 0; // start code
        pattern[1] = flipEndianness(channel%256);
        pattern[2] = flipEndianness(240 - (channel/256)*15);
        pattern[3] = flipEndianness(0xD2);
    }
    else if (ledType == WS2822S) {
        // WS2822S (from datasheet)
        int channel = (address)*3 + 1;
        pattern[0] = 0; // start code
        pattern[1] = flipEndianness(channel%256);
        pattern[2] = flipEndianness(240 - (channel/256)*15);
        pattern[3] = flipEndianness(0xD2);
    }
    else {
        // Error
        return;
    }

    // Pull address high and data low to signal address programming start
    digitalWriteFast(ADDRESS_PIN, HIGH);
    digitalWriteFast(DATA_PIN, LOW);
    delay(50);
    watchdog_refresh();

    // send program data out
    sendAddressData(pattern, patternLength);
    
    // pull address and data pins low
    digitalWriteFast(ADDRESS_PIN, HIGH);
    digitalWriteFast(DATA_PIN, HIGH);

    // Delay for a while to show the pixel color; white means programming was successful
    for(int i = 0; i < 3; i++) {
        delay(100);
        watchdog_refresh();
    }

    // toggle power to reset the LEDs
    // Set outputs to low first to avoid powering the LEDs through the digital pins
    digitalWriteFast(ADDRESS_PIN, LOW);
    digitalWriteFast(DATA_PIN, LOW);
    disableOutputPower();

    delay(100);

    enableOutputPower();
    digitalWriteFast(ADDRESS_PIN, HIGH);
    digitalWriteFast(DATA_PIN, HIGH);
}
