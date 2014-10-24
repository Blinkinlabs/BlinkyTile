/*
 * Peripherals for the BlinkyTile controller
 *
 * Copyright (c) 2014 Matt Mets
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

#ifndef BLINKYTILE_H
#define BLINKYTILE_H

#define LED_COUNT 170        // Number of LEDs we are controlling
#define BYTES_PER_PIXEL    3

#define REV_A

#ifdef REV_A

#define BUTTON_A_PIN      5 // Button A: Port D7
#define BUTTON_B_PIN     21 // Button B: Port D6
#define STATUS_LED_PIN   20 // Status LED: Port D5
#define POWER_ENABLE_PIN  2 // Output power enable resistor: Port D0
#define FLASH_CS_PIN     17 // Flash chip select: Port B1
#define ADDRESS_P_PIN     9 // Address program pin: Port C3
#define DATA_OUT_PIN     10 // Data output pin: Port C4

#elif defined REV_B

// TODO: RevB pinout

#endif

// Initialize the board hardware (buttons, status led, LED control pins)
extern void initBoard();


// Set the brightness of the status LED
// 0 = off, 255 = on
extern void setStatusLed(uint8_t value);

// Turn on the output power line
extern void enableOutputPower();

// Turn off the output power line
extern void disableOutputPower();

#endif

