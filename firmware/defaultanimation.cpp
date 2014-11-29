#include "defaultanimation.h"
#include "animation.h"
#include "jedecflash.h"
#include "blinkytile.h"

// Make a default animation, and write it to flash
void makeDefaultAnimation(FlashSPI& flash) {
    flash.setWriteEnable(true);
    flash.eraseAll();
    while(flash.busy()) {
        watchdog_refresh();
        delay(100);
    }
    flash.setWriteEnable(false);

    #define SAMPLE_ANIMATION_ADDRESS 0x00000100
    #define SAMPLE_ANIMATION_FRAMES  5

    // Program a table!
    uint32_t sampleTable[64];
    sampleTable[0] = ANIMATIONS_MAGIC_NUMBER;       // magic number
    sampleTable[1] = 1;                             // number of animations in table
    sampleTable[2] = SAMPLE_ANIMATION_FRAMES;       // frames in animation 1
    sampleTable[3] = 300;                           // speed of animation 1
    sampleTable[4] = SAMPLE_ANIMATION_ADDRESS;      // flash address of animation 1
   
    flash.setWriteEnable(true); 
    flash.writePage(ANIMATIONS_TABLE_ADDRESS, (uint8_t*) sampleTable);
    while(flash.busy()) {
        watchdog_refresh();
        delay(100);
    }
    flash.setWriteEnable(false); 

    uint8_t sampleAnimation[256];

    for(int frame = 0; frame < SAMPLE_ANIMATION_FRAMES; frame++) {
        for(int pixel = 0; pixel < LED_COUNT; pixel++) {
            uint8_t value = 0;

            switch(frame) {
                case 0:
                    value = 255;
                    break;
                case 1:
                    value = 128;
                    break;
                case 2:
                    value = 64;
                    break;
                case 3:
                    value = 32;
                    break;
                case 4:
                    value = 16;
                    break;
                case 5:
                    value = 8;
                    break;
            }
            sampleAnimation[frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 0] = 0;     // b
            sampleAnimation[frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 1] = 0;     // g
            sampleAnimation[frame*LED_COUNT*BYTES_PER_PIXEL + pixel*BYTES_PER_PIXEL + 2] = value; // r
        }
    }

    flash.setWriteEnable(true); 
    flash.writePage(SAMPLE_ANIMATION_ADDRESS, sampleAnimation);
    while(flash.busy()) {
        watchdog_refresh();
        delay(100);
    }
    flash.setWriteEnable(false);
}
