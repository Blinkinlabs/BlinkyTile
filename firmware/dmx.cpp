#include "WProgram.h"
#include "pins_arduino.h"
#include "dmx.h"
#include "blinkytile.h"

#ifdef FAST_DMX

#define BIT_LENGTH               (2)            // This isn't exact, but is much faster!
#define BREAK_LENGTH             (88)
#define MAB_LENGTH               (8)

#else

#define BIT_LENGTH               (4)
#define BREAK_LENGTH             (100)
#define MAB_LENGTH               (8)

#endif

uint8_t dataArray[1 + LED_COUNT*BYTES_PER_PIXEL];    // Storage for DMX output (0 is the start frame)
uint8_t brightness;

static volatile uint8_t *dmxPort;
static uint8_t dmxBit = 0;

void dmxSetup() {
    dmxPort = portOutputRegister(digitalPinToPort(DATA_PIN));
    dmxBit = digitalPinToBitMask(DATA_PIN);

    dataArray[0] = 0;    // Start bit!

    digitalWrite(DATA_PIN, HIGH);

    brightness = 255;
}

void dmxSetBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
}

void dmxSetPixel(int pixel, uint8_t r, uint8_t g, uint8_t b) {
    dataArray[pixel*BYTES_PER_PIXEL + 1] = r;
    dataArray[pixel*BYTES_PER_PIXEL + 2] = g;
    dataArray[pixel*BYTES_PER_PIXEL + 3] = b;
}

void dmxSendByte(uint8_t value)
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
void dmxShow() {
  
    digitalWriteFast(DATA_PIN, LOW);    // Break - 88us
    delayMicroseconds(BREAK_LENGTH);

    digitalWriteFast(DATA_PIN, HIGH);   // MAB - 8 us
    delayMicroseconds(MAB_LENGTH);

  
    // For each address
    for(int frame = 0; frame < 1 + LED_COUNT*BYTES_PER_PIXEL; frame++) {    
        dmxSendByte(dataArray[frame]*(brightness/255.0));
    }
}

uint8_t* dmxGetPixels() {
    return &dataArray[1]; // Return first pixel in the array
}
