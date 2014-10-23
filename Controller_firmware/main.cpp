/*
 * Fadecandy Firmware
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <math.h>
#include <algorithm>
#include <stdlib.h>
#include "fc_usb.h"
#include "fc_defs.h"
#include "HardwareSerial.h"
#include "arm_math.h"

#include "matrix.h"
#include "winbondflash.h"
#include "animation.h"
#include "defaultanimation.h"
#include "protocol.h"

// USB data buffers
static fcBuffers buffers;
fcLinearLUT fcBuffers::lutCurrent;

// Winbond flash stuff
winbondFlashSPI flash;

// Animations class
Animations animations;

// Serial programming receiver
Protocol serialReceiver;

#define FLASH_CS_PIN      17       // Port B, 1

// Reserved RAM area for signalling entry to bootloader
extern uint32_t boot_token;

static void dfu_reboot()
{
    // Reboot to the Fadecandy Bootloader
    boot_token = 0x74624346;

    // Short delay to allow the host to receive the response to DFU_DETACH.
    uint32_t deadline = millis() + 10;
    while (millis() < deadline) {
        watchdog_refresh();
    }

    // Detach from USB, and use the watchdog to time out a 10ms USB disconnect.
    __disable_irq();
    USB0_CONTROL = 0;
    while (1);
}

extern "C" int usb_rx_handler(usb_packet_t *packet)
{
    // USB packet interrupt handler. Invoked by the ISR dispatch code in usb_dev.c
    return buffers.handleUSB(packet);
}


void setupWatchdog() {
    // Change the watchdog timeout because the SPI access is too slow.
    const uint32_t watchdog_timeout = F_BUS / 1;  // 1000ms

    WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
    WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
    asm volatile ("nop");
    asm volatile ("nop");
    WDOG_STCTRLH = WDOG_STCTRLH_ALLOWUPDATE | WDOG_STCTRLH_WDOGEN |
        WDOG_STCTRLH_WAITEN | WDOG_STCTRLH_STOPEN | WDOG_STCTRLH_CLKSRC;
    WDOG_PRESC = 0;
    WDOG_TOVALH = (watchdog_timeout >> 16) & 0xFFFF;
    WDOG_TOVALL = (watchdog_timeout)       & 0xFFFF;
}

// Handle full messages here
void handleData(uint16_t dataSize, uint8_t* data) {
    switch(data[0]) {
        case 0x00:  // Display some data on the LEDs
        {
    
            if(dataSize != 1 + LED_ROWS*LED_COLS) {
                return;
            }
            memcpy(getPixels(), &data[1], LED_ROWS*LED_COLS);
            show();
        }
        case 0x01:  // Clear the flash
        {
            if(dataSize != 1) {
                return;
            }
            flash.setWriteEnable(true);
            flash.eraseAll();
            while(flash.busy()) {
                watchdog_refresh();
                delay(100);
            }
            flash.setWriteEnable(false);
        }
            break;

        case 0x02:  // Program a page of memory
        {
            if(dataSize != (1 + 4 + 256)) {
                return;
            }

            uint32_t address =
                  (data[1] << 24)
                + (data[2] << 16)
                + (data[3] <<  8)
                + (data[4]      );

            flash.setWriteEnable(true); 
            flash.writePage(address, (uint8_t*) &data[5]);
            while(flash.busy()) {
                delay(10);
                watchdog_refresh();
            }
            flash.setWriteEnable(false); 
        }
            break;

        default:
            break;
    }
}

extern "C" int main()
{
    setupWatchdog();

    srand(22);
 
    flash.begin(winbondFlashClass::autoDetect, FLASH_CS_PIN);

    animations.begin(flash);

    // If the flash was not set up with an animation, burn one to it.
    if(!animations.isInitialized()) {
        makeDefaultAnimation(flash);
        animations.begin(flash);
    }

    matrixSetup();


    setBrightness(1);

    serial_begin(BAUD2DIV(115200));
    serialReceiver.reset();

    uint32_t animation = 0;    

    uint32_t frame = 0;                             // current frame to display
    uint32_t nextTime = millis() + animations.getAnimation(animation)->speed; // time to display next frame
    bool playback = true;                           // if true, play back from flash

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();
        
        if(playback && millis() > nextTime) {
            animations.getAnimation(animation)->getFrame(frame, getPixels());
            show();

            frame++;
            if(frame >= animations.getAnimation(animation)->frameCount) {
                frame = 0;
#if 1
                // increment through
                animation++;
                if(animation >= animations.getAnimationCount()) {
                    animation = 0;
                }
#else
                animation = floor(animations.getAnimationCount()*rand());
#endif
            }

            nextTime += animations.getAnimation(animation)->speed;
        }

        if(serial_available() > 0) {
            playback = false;

            if(serialReceiver.parseByte(serial_getchar())) {


              uint16_t dataSize = serialReceiver.getPacketSize();
              uint8_t* data = serialReceiver.getPacket();
              handleData(dataSize, data);
            }
        }
    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
