#include "defaultanimation.h"
#include "animation.h"
#include "matrix.h"

// Make a default animation, and write it to flash
void makeDefaultAnimation(winbondFlashSPI& flash) {
    flash.setWriteEnable(true);
    flash.eraseAll();
    while(flash.busy()) { delay(100); }
    flash.setWriteEnable(false);

    #define SAMPLE_ANIMATION_ADDRESS 0x00000100
    #define SAMPLE_ANIMATION_FRAMES  5

    // Program a table!
    uint32_t sampleTable[64];
    sampleTable[0] = ANIMATIONS_MAGIC_NUMBER;       // magic number
    sampleTable[1] = 1;                             // number of animations in table
    sampleTable[2] = SAMPLE_ANIMATION_FRAMES;       // frames in animation 1
    sampleTable[3] = 1000;                           // speed of animation 1
    sampleTable[4] = SAMPLE_ANIMATION_ADDRESS;      // flash address of animation 1
   
    flash.setWriteEnable(true); 
    flash.writePage(ANIMATIONS_TABLE_ADDRESS, (uint8_t*) sampleTable);
    while(flash.busy()) { delay(100); }
    flash.setWriteEnable(false); 

    uint8_t sampleAnimation[256];

    for(uint8_t frame = 0; frame < SAMPLE_ANIMATION_FRAMES; frame++) {
        for(uint8_t row = 0; row < LED_ROWS; row++) {
            for(uint8_t col = 0; col < LED_COLS; col++) {
                uint8_t value = 0;
                switch(frame) {
                    case 0:
                        value = 255;
                        break;
                    case 1:
                        value = 0;
                        break;
                    case 2:
                        value = ((row+col)%2==1?255:0);
                        break;
                    case 3:
                        value = ((row+col)%2==1?0:255);
                        break;
                    case 4:
                        value = 20;
                        break;
                }
                sampleAnimation[frame*LED_ROWS*LED_COLS + row*LED_COLS + col] = value;
            }
        }
    }

    flash.setWriteEnable(true); 
    flash.writePage(SAMPLE_ANIMATION_ADDRESS, sampleAnimation);
    while(flash.busy()) { delay(100); }
    flash.setWriteEnable(false);
}
