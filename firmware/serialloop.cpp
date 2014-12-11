#include "WProgram.h"
#include "blinkytile.h"
#include "serialloop.h"
#include "usb_serial.h"
#include "dmx.h"

// We start in BlinkyTape mode, but if we get the magic escape sequence, we transition
// to BlinkyTile mode.
// Escape sequence is 10 0xFF characters in a row.

int serialMode;         // Serial protocol we are speaking
int escapeRunCount;     // Count of how many escape characters we've received
uint8_t buffer[3];      // Buffer for one pixel of data
int bufferIndex;        // Current location in the buffer

int pixelIndex;         // Pixel we are currently writing to

void serialReset() {
    serialMode = SERIAL_MODE_BLINKYTAPE;

    escapeRunCount = 0;
    pixelIndex = 0;
    bufferIndex = 0;
}

void serialLoop() {
    while(usb_serial_available() > 0) {
        uint8_t c = usb_serial_getchar();

        // Pixel character
        if(c < 255) {
            escapeRunCount = 0;

            // If we got a pixel color, buffer it.
            buffer[bufferIndex++] = c;

            // If this makes a complete pixel color, update the display and reset for the next color
            if(bufferIndex > 2) {
                bufferIndex = 0;

                // Prevent overflow by ignoring any pixel data beyond LED_COU
                if(pixelIndex == LED_COUNT)
                    break;

                dmxSetPixel(pixelIndex+1, buffer[0], buffer[1], buffer[2]);
                pixelIndex++;
            }
        }

        // Control character
        else {
            // reset the pixel engine
            bufferIndex = 0;
            pixelIndex = 0;
            escapeRunCount++;

            // If this is the first escape character, refresh the output
            if(escapeRunCount == 1) {
                dmxShow();
            }
            
            if(escapeRunCount > 10) {
                serialMode = SERIAL_MODE_BLINKYTILE;
            }
        }
    }
}
