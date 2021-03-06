/*
 * Remote control for the Fadecandy firmware, using the ARM debug interface.
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

#include <Arduino.h>
#include "fc_remote.h"
#include "testjig.h"
#include "firmware_data.h"

// TODO: DELETE ME!
#define fw_pFlags 0x00FFFFFF
#define fw_pFbPrev 0x00FFFFFF
#define fw_pFbNext 0x00FFFFFF
#define fw_pLUT 0x00FFFFFF
#define fw_usbPacketBufOffset 0x00FFFFFF


bool FcRemote::installFirmware()
{
    // Install firmware, blinking both target and local LEDs in unison.

    bool blink = false;
    static int i = 0;
    ARMKinetisDebug::FlashProgrammer programmer(target, fw_data, fw_sectorCount);

    if (!programmer.begin())
        return false;

    while (!programmer.isComplete()) {
        if (!programmer.next()) return false;
        
        if((i++ % 128) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }

    return true;
}

bool FcRemote::boot()
{ 
    // Run the new firmware, and let it boot
    if (!target.reset())
        return false;
    delay(50);
    return true;
}

bool FcRemote::setLED(bool on)
{
    const unsigned pin = target.PTD6;
    return
        target.pinMode(pin, OUTPUT) &&
        target.digitalWrite(pin, !on);
}

bool FcRemote::setFlags(uint8_t cflag)
{
    // Set control flags    
    return target.memStoreByte(fw_pFlags, cflag);
}

bool FcRemote::testUserButtons()
{
    // Button pins to test
    const unsigned button2Pin = target.PTD5;
    const unsigned button1Pin = target.PTD7;
    
    if(!target.pinMode(button1Pin,INPUT_PULLUP))
        return false;
    if(!target.pinMode(button2Pin,INPUT_PULLUP))
        return false;
        
    // Button should start high
    if(!target.digitalRead(button1Pin)) {
        target.log(target.LOG_ERROR, "BUTTON: Button 1 stuck low");
        return false;
    }
    if(!target.digitalRead(button2Pin)) {
        target.log(target.LOG_ERROR, "BUTTON: Button 2 stuck low");
        return false;
    }
        
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to press button 1");

    const int FLASH_SPEED = 200;

    while(target.digitalRead(button1Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }

    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to release button 1");

    while(!target.digitalRead(button1Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to press button 2");

    while(target.digitalRead(button2Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Waiting for user to release button 2");
    
    while(!target.digitalRead(button2Pin)) {
        static bool blink = false;
        static int i = 0;
        
        if((i++ % FLASH_SPEED) == 0) {
            blink = !blink;
            if (!setLED(blink)) return false;
            digitalWrite(ledPin, blink);
        }
    }
    
    target.log(target.LOG_NORMAL, "BUTTON: Buttons ok.");
    return true;
}

bool FcRemote::testExternalFlash(bool erase)
{
    target.log(target.LOG_NORMAL, "External flash test: Beginning external flash test");
    
    ////// Initialize the SPI hardware
    if (!target.initializeSpi0())
        return false;
    
    ////// Identify the flash
    
    if(!target.sendSpi0(0x9F, false)) {
        target.log(target.LOG_ERROR, "External flash test: Error communicating with device.");
        return false;
    }

    const int JEDEC_ID_LENGTH = 3;
    uint8_t receivedId[JEDEC_ID_LENGTH];

    for(int i = 0; i < JEDEC_ID_LENGTH; i++) {
        if(!target.receiveSpi0(receivedId[i], (JEDEC_ID_LENGTH - 1) == i)) {
            target.log(target.LOG_ERROR, "External flash test: Error communicating with device.");
            return false;
        }
    }
    
    if((receivedId[0] != 0x01) | (receivedId[1] != 0x40) | (receivedId[2] != 0x15)) {    // Spansion 16m
        target.log(target.LOG_ERROR, "External flash test: Invalid device type, got %02X%02X%02X", receivedId[0], receivedId[1], receivedId[2]);
        return false;
    }

    if(erase) {
        ////// Erase the flash
        
        if(!target.sendSpi0(0x06, true)) {
            target.log(target.LOG_ERROR, "External flash test: Error enabling flash write");
            return false;
        }
    
        if(!target.sendSpi0(0xC7, true)) {
            target.log(target.LOG_ERROR, "External flash test: Error sending erase command.");
            return false;
        }
        
        target.log(target.LOG_NORMAL, "External flash test: Erasing external flash");
        
        uint8_t sr;
        do {
            static bool blink = false;
            static int i = 0;
            const int FLASH_SPEED = 40;
            
            if((i++ % FLASH_SPEED) == 0) {
                blink = !blink;
                if (!setLED(blink)) return false;
                digitalWrite(ledPin, blink);
            }
          
          if(!target.sendSpi0(0x05, false)) {
              target.log(target.LOG_ERROR, "External flash test: Error getting status report");
              return false;
          }
          if(!target.receiveSpi0(sr, true)) {
              target.log(target.LOG_ERROR, "External flash test: Error getting status report");
              return false;
          }
        }
        while(sr & 0x01);
        
        if(!target.sendSpi0(0x03, true)) {
            target.log(target.LOG_ERROR, "External flash test: Error disabling flash write");
            return false;
        }
    }
    
    
    target.log(target.LOG_NORMAL, "External flash test: Successfully completed external flash test!");
    return true;
}
