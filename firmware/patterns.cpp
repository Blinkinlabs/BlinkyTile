#include "WProgram.h"
#include "pins_arduino.h"
#include "blinkytile.h"
#include "patterns.h"
#include "usb_serial.h"
#include "dmx.h"

void color_loop() {  
    static int j = 0;
    static int f = 0;
    static int k = 0;
    
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxSetPixel(i+1,
            128*(1+sin(i/30.0 + j/1.3       )),
            128*(1+sin(i/10.0 + f/2.9)),
            128*(1+sin(i/25.0 + k/5.6))
        );
    }
    
    j = j + 1;
    f = f + 1;
    k = k + 2;
}

void white_loop() {
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxSetPixel(i+1, 255, 255, 255);
    }
}

void green_loop() {
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxSetPixel(i+1, 0, 255, 0);
    }
}

void count_up_loop() {
    #define MAX_COUNT 2
  
    static int counts = 0;
    static int pixel = 0;
    
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        if(pixel == i) {
            dmxSetPixel(i+1, 255, 255, 255);
        }
        else {
            dmxSetPixel(i+1, 0,0,0);
        }
    }
    
    counts++;
    if(counts>MAX_COUNT) {
        counts = 0;
        pixel = (pixel+1)%LED_COUNT;
    }
}


void serial_loop() {
    static int pixelIndex;

        if(usb_serial_available() > 2) {

            uint8_t buffer[3]; // Buffer to store three incoming bytes used to compile a single LED color

            for (uint8_t x=0; x<3; x++) { // Read three incoming bytes
                uint8_t c = usb_serial_getchar();
                
                if (c < 255) {
                   buffer[x] = c; // Using 255 as a latch semaphore
                }
                else {
                    dmxShow();
                    pixelIndex = 0;
                    
                    break;
            }

            if (x == 2) {   // If we received three serial bytes
                if(pixelIndex == LED_COUNT) break; // Prevent overflow by ignoring the pixel data beyond LED_COUNT
                dmxSetPixel(pixelIndex+1, buffer[0], buffer[1], buffer[2]);
                pixelIndex++;
            }
        }
    }
}
