#include "WProgram.h"
#include "blinkytile.h"
#include "serialloop.h"
#include "usb_serial.h"
#include "dmx.h"
#include "addressprogrammer.h"
#include "nofatstorage.h"
#include "jedecflash.h"
#include "defaultanimation.h"
#include "animation.h"
#include <stdlib.h>
#include <stdio.h>

extern FlashSPI flash;
extern NoFatStorage flashStorage;
extern Animations animations;


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
int controlBufferIndex;     // Current location in the buffer


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
        }
        
        if(escapeRunCount > 8) {
            serialMode = SERIAL_MODE_COMMAND;
            controlBufferIndex = 0;
        }
    }
}



bool commandProgramAddress(uint8_t* buffer);
bool commandEraseFlash(uint8_t* buffer);
bool commandFreeSpace(uint8_t* buffer);
bool commandLargestFile(uint8_t* buffer);
bool commandNewFile(uint8_t* buffer);
bool commandWritePage(uint8_t* buffer);
bool commandReadData(uint8_t* buffer);
bool commandFileCount(uint8_t* buffer);
bool commandFirstFreeSector(uint8_t* buffer);

struct Command {
    uint8_t name;
    int length;
    bool (*function)(uint8_t*);
};

Command commands[] = {
    {0x01,   3, commandProgramAddress},
    {0x10,   1, commandFreeSpace},
    {0x11,   1, commandLargestFile},
    {0x12,   1, commandFileCount},
    {0x18,   6, commandNewFile},
    {0x19, 263, commandWritePage},
    {0x1A,   8, commandReadData},
    {0x20,   3, commandEraseFlash},
    {0x21,   1, commandFirstFreeSector},
    {0xFF,   0, NULL}
};



void commandLoop() {
    uint8_t c = usb_serial_getchar();

    // If we get extra 0xFF bytes before the command byte, ignore them
    if((controlBufferIndex == 0) && (c == 0xFF))
        return;

    controlBuffer[controlBufferIndex++] = c;

    for(Command *command = commands; 1; command++) {
        // If the command isn't found in the list, bail
        if(command->name == 0xFF) {
            serialReset();
            break;
        }

        // If this iteration isn't the correct one, keep looking
        if(command->name != controlBuffer[0])
            continue;

        // Now we're on to something- have we gotten enough data though?
        if(controlBufferIndex >= command->length) {
            if(command->function(controlBuffer + 1)) {
                usb_serial_putchar('P');
                usb_serial_putchar((char)controlBuffer[1]);

                usb_serial_write(controlBuffer + 2, controlBuffer[1] + 1);
            }
            else {
                usb_serial_putchar('F');
                usb_serial_putchar(char(0x00));
                usb_serial_putchar(char(0x00));
            }

            serialReset();
        }
        break;
    }
}


bool commandProgramAddress(uint8_t* buffer) {
    programAddress((buffer[0] << 8) + buffer[1]);

    buffer[0] = 0;
    return true;
}

bool commandEraseFlash(uint8_t* buffer) {
    if((buffer[0] != 'E') || (buffer[1] != 'e')) {
        return false;
    }

    flash.setWriteEnable(true);
    flash.eraseAll();
    while(flash.busy()) {
        watchdog_refresh();
        delay(100);
    }
    flash.setWriteEnable(false);

    flashStorage.begin(flash);

    buffer[0] = 0;
    return true;
}

bool commandFirstFreeSector(uint8_t* buffer) {
    int files = flashStorage.findFreeSector(0);

    buffer[0] = 4 - 1;
    buffer[1] = (files >> 24) & 0xFF;
    buffer[2] = (files >> 16) & 0xFF;
    buffer[3] = (files >>  8) & 0xFF;
    buffer[4] = (files >>  0) & 0xFF;

    return true;
}

bool commandFreeSpace(uint8_t* buffer) {
    int freeSpace = flashStorage.freeSpace();

    buffer[0] = 4 - 1;
    buffer[1] = (freeSpace >> 24) & 0xFF;
    buffer[2] = (freeSpace >> 16) & 0xFF;
    buffer[3] = (freeSpace >>  8) & 0xFF;
    buffer[4] = (freeSpace >>  0) & 0xFF;

    return true;
}

bool commandFileCount(uint8_t* buffer) {
    int files = flashStorage.files();

    buffer[0] = 4 - 1;
    buffer[1] = (files >> 24) & 0xFF;
    buffer[2] = (files >> 16) & 0xFF;
    buffer[3] = (files >>  8) & 0xFF;
    buffer[4] = (files >>  0) & 0xFF;

    return true;
}

bool commandLargestFile(uint8_t* buffer) {
    int largestFile = flashStorage.largestNewFile();

    buffer[0] = 4 - 1;
    buffer[1] = (largestFile >> 24) & 0xFF;
    buffer[2] = (largestFile >> 16) & 0xFF;
    buffer[3] = (largestFile >>  8) & 0xFF;
    buffer[4] = (largestFile >>  0) & 0xFF;

    return true;
}

bool commandNewFile(uint8_t* buffer) {
    uint8_t fileType = buffer[0];
    int fileSize = 
        (buffer[1] << 24) + (buffer[2] << 16) + (buffer[3] << 8)+ buffer[4];

    int sector = flashStorage.createNewFile(fileType, fileSize);

    if(sector < 0)
        return false;

    buffer[0] = 2 - 1;
    buffer[1] = (sector >> 8) & 0xFF;
    buffer[2] = (sector     ) & 0xFF;
    return true;
}

bool commandWritePage(uint8_t* buffer) {
    uint8_t sector = (buffer[0] << 8) + buffer[1];
    uint8_t offset =
        (buffer[2] << 24) + (buffer[3] << 16) + (buffer[4] << 8)+ buffer[5];

    int written = flashStorage.writePageToFile(sector, offset, buffer + 6);

    if(written != 256)
        return false;
    
    buffer[0] = 0;
    return true;
}

bool commandReadData(uint8_t* buffer) {
    int sector = (buffer[0] << 8) + buffer[1];
    int offset =
        (buffer[2] << 24) + (buffer[3] << 16) + (buffer[4] << 8)+ buffer[5];
    int length = buffer[6] + 1;

    int read = flashStorage.readFromFile(sector, offset, buffer + 1, length);

    if(read != length)
        return false;
    
    buffer[0] = length - 1;
    return true;
}
