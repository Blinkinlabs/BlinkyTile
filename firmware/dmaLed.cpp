
#include <dmaLed.h>

#include <dmx.h>
#include <ws2812.h>

// Big LED buffer that can be used by the different DMA engines
uint8_t dmaBuffer[DMA_BUFFER_SIZE];

// Buffer for input LED data
uint8_t drawBuffer[DRAW_BUFFER_SIZE];

// System brightness scaler
uint8_t brightness;

CDmaLed::CDmaLed() {
    outputType = DMX;
}

void CDmaLed::setBrightness(uint8_t scale) {
    brightness = scale;
}

uint8_t CDmaLed::getBrightness() {
    return brightness;
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

    // TODO: Move these to a setup() function
    brightness = 255;
    memset(drawBuffer, 0, DRAW_BUFFER_SIZE);

    switch(outputType) {
        case DMX:
            dmxStop();
            break;
        case WS2812:
            ws2812Stop();
            break;
    }

    outputType = type;
    switch(outputType) {
        case DMX:
            dmxSetup();
            break;
        case WS2812:
            ws2812Setup();
            break;
    }
}


void CDmaLed::show(uint8_t scale) {
    switch(outputType) {
        case DMX:
            dmxShow();
            break;
        case WS2812:
            ws2812Show();
            break;
    }
}
