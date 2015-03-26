
#include <dmaLed.h>

#include <dmx.h>
#include <ws2812.h>
#include <lpd8806.h>
#include <apa102.h>

// Big LED buffer that can be used by the different DMA engines
uint8_t dmaBuffer[DMA_BUFFER_SIZE];

// Buffer for input LED data
uint8_t drawBuffer[DRAW_BUFFER_SIZE];

// System brightness scaler
uint8_t brightness;


CDmaLed::CDmaLed() :
    outputType(DMX),
    running(false) {
    brightness = 255;
}

void CDmaLed::setBrightness(uint8_t scale) {
    brightness = scale;
}

uint8_t CDmaLed::getBrightness() {
    return brightness;
}

// Untested
void CDmaLed::clearData() {
    memset(drawBuffer, 0, DRAW_BUFFER_SIZE);

}

void CDmaLed::delay(unsigned long ms) {
    delay(ms);
}

void CDmaLed::show() {
    show(brightness);
}

uint8_t* CDmaLed::getPixels() {
    return drawBuffer;
}

bool CDmaLed::drawWaiting() {
    return false;
}

void CDmaLed::setPixel(int pixel, uint8_t r, uint8_t g, uint8_t b) {
    drawBuffer[pixel*BYTES_PER_PIXEL + 0] = r;
    drawBuffer[pixel*BYTES_PER_PIXEL + 1] = g;
    drawBuffer[pixel*BYTES_PER_PIXEL + 2] = b;
}


void CDmaLed::setOutputType(output_type_t type) {
    if(running) {

        switch(outputType) {
            case DMX:
                dmx.stop();
                break;
            case WS2812:
                ws2812.stop();
                break;
            case LPD8806:
                lpd8806.stop();
                break;
            case APA102:
                apa102.stop();
                break;
        }
    }

    running = true;
    outputType = type;
    switch(outputType) {
        case DMX:
            dmx.start();
            break;
        case WS2812:
            ws2812.start();
            break;
        case LPD8806:
            lpd8806.start();
            break;
        case APA102:
            apa102.start();
            break;
    }
}


void CDmaLed::show(uint8_t scale) {
    switch(outputType) {
        case DMX:
            dmx.show();
            break;
        case WS2812:
            ws2812.show();
            break;
        case LPD8806:
            lpd8806.show();
            break;
        case APA102:
            apa102.show();
            break;
    }
}
