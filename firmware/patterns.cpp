#include "WProgram.h"
#include "blinkytile.h"
#include "patterns.h"
#include "dmx.h"

void color_loop() {  
    static int j = 0;
    static int f = 0;
    static int k = 0;
    
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxSetPixel(i,
            128*(1+sin(i/30.0 + j/1.3)),
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
        dmxSetPixel(i, 255, 255, 255);
    }
}

void green_loop() {
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxSetPixel(i, 0, 255, 0);
    }
}

void count_up_loop() {
    #define MAX_COUNT 2
  
    static int counts = 0;
    static int pixel = 0;
    
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        if(pixel == i) {
            dmxSetPixel(i, 255, 255, 255);
        }
        else {
            dmxSetPixel(i, 0,0,0);
        }
    }
    
    counts++;
    if(counts>MAX_COUNT) {
        counts = 0;
        pixel = (pixel+1)%LED_COUNT;
    }
}

