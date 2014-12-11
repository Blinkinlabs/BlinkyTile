/*
 * Blinky Controller
 *
* Copyright (c) 2014 Matt Mets
 *
 * based on Fadecandy Firmware, Copyright (c) 2013 Micah Elizabeth Scott
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
#include <stdio.h>
#include "fc_usb.h"
#include "arm_math.h"
#include "HardwareSerial.h"
#include "usb_serial.h"

#include "blinkytile.h"
#include "animation.h"
#include "defaultanimation.h"
#include "jedecflash.h"
#include "nofatstorage.h"
#include "protocol.h"
#include "dmx.h"
#include "addressprogrammer.h"
#include "patterns.h"
#include "buttons.h"

// TODO: delete me
extern void singleCharacterHack(char in);

// USB data buffers
static fcBuffers buffers;
fcLinearLUT fcBuffers::lutCurrent;

// External flash chip
FlashSPI flash;

// Flash storage class, works on top of the base flash chip
NoFatStorage flashStorage;

// Animations class
Animations animations;

// Serial programming receiver
Protocol serialReceiver;

// Button inputs
Buttons userButtons;

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

extern "C" int usb_fc_rx_handler()
{
    // USB packet interrupt handler. Invoked by the ISR dispatch code in usb_dev.c
    return buffers.handleUSB();
}


void setupWatchdog() {
    // Change the watchdog timeout because the SPI access is too slow.
    const uint32_t watchdog_timeout = F_BUS / 2;  // 500ms

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



#define MESSAGE_SHOW_LEDS	0x00
#define MESSAGE_ERASE_FLASH	0x01
#define MESSAGE_PROGRAM_PAGE	0x02

// Handle full messages here
void handleData(uint16_t dataSize, uint8_t* data) {
    switch(data[0]) {
        case MESSAGE_SHOW_LEDS:  // Display some data on the LEDs
        {
    
            if(dataSize != 1 + LED_COUNT) {
                return;
            }
            memcpy(dmxGetPixels(), &data[1], LED_COUNT);
            dmxShow();
        }
	    break;

        case MESSAGE_ERASE_FLASH:  // Clear the flash
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

        case MESSAGE_PROGRAM_PAGE:  // Program a page of memory
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

    initBoard();

    userButtons.setup();

    dmxSetup();
    enableOutputPower();

    // If the flash initializes successfully, then load the animations table.
    if(flash.begin(FlashClass::autoDetect)) {

	// Connect up the flash storage class
	flashStorage.begin(flash);

        animations.begin(flashStorage);
    }

//    serial_begin(BAUD2DIV(115200));
//    serialReceiver.reset();

    uint32_t animation = 1;

    uint32_t frame = 0;                             // current frame to display
    uint32_t nextTime = millis() + animations.getAnimation(animation)->speed; // time to display next frame

    // Application main loop
    while (usb_dfu_state == DFU_appIDLE) {
        watchdog_refresh();
       
        // TODO: put this in an ISR? Make the buttons do pin change interrupts?
        userButtons.buttonTask();

        static int state = 0;
        #define PATTERN_COUNT 4
        static int pattern = 0;

        #define BRIGHTNESS_COUNT 5
        static int brightnessLevels[BRIGHTNESS_COUNT] = {5,20,60,120,255};
        static int brightnessStep = 5;

        static bool serial_mode = false;

        // Play a pattern
        if((state%2000 == 1) & (!serial_mode)) {
            switch(pattern) {
                case 0:
                    // If the flash wasn't initialized, then skip to the next built-in pattern.
                    if(!animations.isInitialized()) {
//                        pattern++;
                        break;
                    }

                    // Flash-based
                    if(millis() > nextTime) {
                        animations.getAnimation(animation)->getFrame(frame, dmxGetPixels());
                        dmxShow();

                        frame++;
                        if(frame >= animations.getAnimation(animation)->frameCount) {
                            frame = 0;

                            // increment through
                            animation++;
                            if(animation >= animations.getAnimationCount()) {
                                animation = 0;
                            }
                        }

                        nextTime += animations.getAnimation(animation)->speed;

                        // If we've gotten too far ahead of ourselves, reset the animation count
                        if(millis() > nextTime) {
                            nextTime = millis() + animations.getAnimation(animation)->speed;
                        }
                    }
                    break;

                case 1:
                    color_loop();
                    break;
                case 2:
                    count_up_loop();
                    break;
                case 3:
                    white_loop();
                    break;
                case 4:
                    green_loop();
                    break;
            }


            dmxShow();
        }
        state++;

        if(userButtons.isPressed()) {
            uint8_t button = userButtons.getPressed();
    
            if(button == BUTTON_A) {
                pattern = (pattern + 1) % PATTERN_COUNT;
            }
            else if(button == BUTTON_B) {
                brightnessStep = (brightnessStep + 1) % BRIGHTNESS_COUNT;
                dmxSetBrightness(brightnessLevels[brightnessStep]);
            }
        }

        buffers.finalizeFrame();

        if(usb_serial_available() > 0) {
//            singleCharacterHack(usb_serial_getchar());

            serial_mode = true;
            serial_loop();

//            if(serialReceiver.parseByte(serial_getchar())) {
//              uint16_t dataSize = serialReceiver.getPacketSize();
//              uint8_t* data = serialReceiver.getPacket();
//              handleData(dataSize, data);
//            }
        }
    }

    // Reboot into DFU bootloader
    dfu_reboot();
}
