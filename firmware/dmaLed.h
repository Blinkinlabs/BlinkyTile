#ifndef DMALED_H
#define DMALED_H

#include "WProgram.h"
#include "BlinkyTile.h"

class CDmaLed {
public:
    enum output_type_t {DMX, WS2812, LPD8806, APA102};

    CDmaLed();

    // Not supported by FastLED, but wtf?
    // Update a single pixel in the array
    // @param pixel int Pixel address
    // @param r uint8_t New red value for the pixel (0 - 255)
    // @param g uint8_t New red value for the pixel (0 - 255)
    // @param b uint8_t New red value for the pixel (0 - 255)
    void setPixel(int pixel, uint8_t r, uint8_t g, uint8_t b);

    void setOutputType(output_type_t type);
    output_type_t getOutputType() {return outputType;}

    uint8_t* getPixels();

    bool drawWaiting();


// FastLed-ish API

    void setBrightness(uint8_t scale);
    uint8_t getBrightness();


    /// Update all our controllers with the current led colors, using the passed in brightness
    void show(uint8_t scale);

    /// Update all our controllers with the current led colors
    void show();

/*
    void clear(boolean writeData = false);
*/
    void clearData();
/*

    void showColor(const struct CRGB & color, uint8_t scale);
    void showColor(const struct CRGB & color) { showColor(color, m_Scale); }
*/

    void delay(unsigned long ms);

/*
    void setTemperature(const struct CRGB & temp);
    void setCorrection(const struct CRGB & correction);
    void setDither(uint8_t ditherMode = BINARY_DITHER);

    // for debugging, will keep track of time between calls to countFPS, and every
    // nFrames calls, it will update an internal counter for the current FPS.
    void countFPS(int nFrames=25);

    // Get the number of frames/second being written out
    uint16_t getFPS() { return m_nFPS; }

    // returns the number of controllers (strips) that have been added with addLeds
    int count();

    // returns the Nth controller
    CLEDController & operator[](int x);

    // Convenience functions for single-strip setups:
    // returns the number of LEDs in the first strip
    int size() { return (*this)[0].size(); }

    // returns pointer to the CRGB buffer for the first strip
    CRGB *leds() { return (*this)[0].leds(); }
*/

private:
    output_type_t outputType;

    bool running;   // True if the LED engine is running
};

class DmaLedController {
public:
    virtual void start();       // Initialize, set up the controller
    virtual void stop();        // Stop the controller
    virtual void show();        // Tell the controller to flip the display
    virtual bool waiting();     // True if the controller has a display flip pending
};

// Size of the input buffer
// TODO: Convert this to a FastLED style array?
#define DRAW_BUFFER_SIZE LED_COUNT*BYTES_PER_PIXEL

// Size of the output buffer, implementation dependent.
// Needs to be as large as the largest implementation (set by hand)

#define DMA_BUFFER_FRAME_SIZE (4+LED_COUNT*4)*8 + (LED_COUNT + 1)/2

#if defined(DOUBLE_BUFFER)
#define DMA_BUFFER_SIZE DMA_BUFFER_FRAME_SIZE*2
#else
#define DMA_BUFFER_SIZE DMA_BUFFER_FRAME_SIZE
#endif

extern uint8_t dmaBuffer[DMA_BUFFER_SIZE];
extern uint8_t drawBuffer[DRAW_BUFFER_SIZE];        // Buffer for the user to draw into
extern uint8_t brightness;


#endif
