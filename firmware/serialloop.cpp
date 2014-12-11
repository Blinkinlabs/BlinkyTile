#include "WProgram.h"
#include "blinkytile.h"
#include "serialloop.h"
#include "usb_serial.h"
#include "dmx.h"
#include "addressprogrammer.h"

// We start in BlinkyTape mode, but if we get the magic escape sequence, we transition
// to BlinkyTile mode.
// Escape sequence is 10 0xFF characters in a row.

int serialMode;         // Serial protocol we are speaking

///// Defines for the data mode
void dataLoop();

int escapeRunCount;     // Count of how many escape characters we've received
uint8_t buffer[3];      // Buffer for one pixel of data
int bufferIndex;        // Current location in the buffer
int pixelIndex;         // Pixel we are currently writing to

///// Defines for the control mode
void commandLoop();

#define CONTROL_BUFFER_SIZE 300
uint8_t controlBuffer[CONTROL_BUFFER_SIZE];     // Buffer for receiving command data
uint8_t controlBufferIndex;     // Current location in the buffer


void serialReset() {
    serialMode = SERIAL_MODE_DATA;

    bufferIndex = 0;
    pixelIndex = 0;

    escapeRunCount = 0;

    controlBufferIndex = 0;
}

void serialLoop() {
    switch(serialMode) {
        case SERIAL_MODE_DATA:
            dataLoop();
            break;
        case SERIAL_MODE_COMMAND:
            commandLoop();
            break;
        default:
            serialReset();
    }
}

void dataLoop() {
    uint8_t c = usb_serial_getchar();

    // Pixel character
    if(c != 0xFF) {
        // Reset the control character state variables
        escapeRunCount = 0;

        // Buffer the color
        buffer[bufferIndex++] = c;

        // If this makes a complete pixel color, update the display and reset for the next color
        if(bufferIndex > 2) {
            bufferIndex = 0;

            // Prevent overflow by ignoring any pixel data beyond LED_COUNT
            if(pixelIndex < LED_COUNT) {
                dmxSetPixel(pixelIndex, buffer[0], buffer[1], buffer[2]);
                pixelIndex++;
            }
        }
    }

    // Control character
    else {
        // reset the pixel character state vairables
        bufferIndex = 0;
        pixelIndex = 0;

        escapeRunCount++;

        // If this is the first escape character, refresh the output
        if(escapeRunCount == 1) {
            dmxShow();
            setStatusLed(0);
        }
        
        if(escapeRunCount > 8) {
            serialMode = SERIAL_MODE_COMMAND;
            controlBufferIndex = 0;
        }
    }
}

#define COMMAND_PROGRAM_ADDRESS         'p'
#define COMMAND_PROGRAM_ADDRESS_LENGTH  3

void commandLoop() {
    uint8_t c = usb_serial_getchar();

    // If we get extra 0xFF, ignore them
    if(c == 0xFF)
        return;

    controlBuffer[controlBufferIndex++] = c;

    switch(controlBuffer[0]) {
        case COMMAND_PROGRAM_ADDRESS:
            if(controlBufferIndex == COMMAND_PROGRAM_ADDRESS_LENGTH) {
                programAddress((controlBuffer[1] << 8) + controlBuffer[2]);
                serialReset();
            }
            break;
        default:
            serialReset();
    }
}
