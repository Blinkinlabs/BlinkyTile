#include "WProgram.h"
#include "pins_arduino.h"
#include "dmx.h"
#include "blinkytile.h"


#define INPUT_BYTES             LED_COUNT*BYTES_PER_PIXEL
#define OUTPUT_BYTES            LED_COUNT*BYTES_PER_PIXEL

#define OUTPUT_BITCOUNT         LED_COUNT*BYTES_PER_PIXEL*8


#define OUT_OFFSET              4       // Offset of our output pin in port C

uint8_t drawBuffer[INPUT_BYTES];        // Buffer for the user to draw into
uint8_t dmaBuffer[2][OUTPUT_BITCOUNT];  // Double-buffered output for the DMA engine
uint8_t* frontBuffer;
uint8_t* backBuffer;
bool swapBuffers;

uint8_t brightness;



uint8_t ONE = _BV(OUT_OFFSET);

// FTM0 drives our whole operation!
void setupFTM0(){
  FTM0_MODE = FTM_MODE_WPDIS;    // Disable Write Protect

  FTM0_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?
  FTM0_MOD = 0x003C;             // Period register
  FTM0_SC |= FTM_SC_CLKS(1) | FTM_SC_PS(0);

  FTM0_MODE |= FTM_MODE_INIT;         // Enable FTM0

  FTM0_C0SC = 0x40          // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM0_C0V = 0x0001;        // Duty cycle of PWM signal

  FTM0_C1SC = 0x40          // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM0_C1V = 0x0014;        // Duty cycle of PWM signal     // TODO: Why is this backwards?

  FTM0_C2SC = 0x40          // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM0_C2V = 0x0028;        // Duty cycle of PWM signal

  FTM0_SYNC |= 0x80;        // set PWM value update
}


// TCD0 sets the outputs to 1 at the beginning of the cycle
void setupTCD0(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD0_SADDR = source;                                        // Address to read from
  DMA_TCD0_SOFF = 0;                                              // Bytes to increment source register between writes 
  DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD0_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD0_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  DMA_TCD0_DADDR = &GPIOC_PSOR;                                   // Address to write to
  DMA_TCD0_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD0_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  DMA_TCD0_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD0_DLASTSGA = 0;                                          // Address of next TCD (N/A)
}

// TCD2 sets the outputs to 0 further along the cycle
void setupTCD1(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD1_SADDR = source;                                        // Address to read from
  DMA_TCD1_SOFF = 1;                                              // Bytes to increment source register between writes 
  DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD1_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD1_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  DMA_TCD1_DADDR = &GPIOC_PCOR;                                   // Address to write to
  DMA_TCD1_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD1_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  DMA_TCD1_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD1_DLASTSGA = 0;                                          // Address of next TCD (N/A)
}

// TCD2 sets the outputs to 0 further along the cycle
void setupTCD2(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD2_SADDR = source;                                        // Address to read from
  DMA_TCD2_SOFF = 0;                                              // Bytes to increment source register between writes 
  DMA_TCD2_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD2_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD2_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  DMA_TCD2_DADDR = &GPIOC_PCOR;                                   // Address to write to
  DMA_TCD2_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD2_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  DMA_TCD2_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD2_DLASTSGA = 0;                                          // Address of next TCD (N/A)
}


void setupTCDs() {
    setupTCD0(&ONE, 1, OUTPUT_BITCOUNT);
    setupTCD1(frontBuffer, 1, OUTPUT_BITCOUNT);
    setupTCD2(&ONE, 1, OUTPUT_BITCOUNT);
}

// Send a DMX frame with new data
void dmxTransmit() {
    setupTCDs();

    DMA_SSRT = DMA_SSRT_SAST;

    FTM0_SC |= FTM_SC_CLKS(1);
}

void dmxSetup() {
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(1);	// Configure TX Pin for digital
    GPIOC_PDDR |= _BV(4);

    // DMA
    // Configure DMA
    SIM_SCGC7 |= SIM_SCGC7_DMA;  // Enable DMA clock
    DMA_CR = 0;  // Use default configuration
 
    // Configure the DMA request input for DMA0
    DMA_SERQ = DMA_SERQ_SERQ(0);        // Configure DMA0 to enable the request signal
    DMA_SERQ = DMA_SERQ_SERQ(1);        // Configure DMA2 to enable the request signal
    DMA_SERQ = DMA_SERQ_SERQ(2);        // Configure DMA2 to enable the request signal
 
    // Enable interrupt on major completion for DMA channel 2
    DMA_TCD2_CSR = DMA_TCD_CSR_INTMAJOR;  // Enable interrupt on major complete
    NVIC_ENABLE_IRQ(IRQ_DMA_CH2);         // Enable interrupt request
 
    // DMAMUX
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX; // Enable DMAMUX clock

    // Timer DMA channel:
    // Configure DMAMUX to trigger DMA0 from FTM0_CH0
    DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG0 = DMAMUX_SOURCE_FTM0_CH0 | DMAMUX_ENABLE;

    // Configure DMAMUX to trigger DMA0 from FTM0_CH0
    DMAMUX0_CHCFG1 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG1 = DMAMUX_SOURCE_FTM0_CH1 | DMAMUX_ENABLE;

    // Configure DMAMUX to trigger DMA2 from FTM0_CH2
    DMAMUX0_CHCFG2 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG2 = DMAMUX_SOURCE_FTM0_CH2 | DMAMUX_ENABLE;
 
    // Load this frame of data into the DMA engine
    setupTCDs();
 
    // FTM
    SIM_SCGC6 |= SIM_SCGC6_FTM0;  // Enable FTM0 clock
    setupFTM0();

    frontBuffer = dmaBuffer[0];
    backBuffer = dmaBuffer[1];
    swapBuffers = false;

    // Clear the display
    memset(drawBuffer, 0, INPUT_BYTES);
    memset(frontBuffer, 0, OUTPUT_BYTES);

    brightness = 255;

    dmxTransmit();
}


// At the end of the DMX frame, start it again
void dma_ch2_isr(void) {
    DMA_CINT = DMA_CINT_CINT(2);


    // TODO: Turning off the timer this late into the cycle causes a small glitch :-/
    FTM0_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?

    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;
    GPIOC_PCOR = 0xFF;

    
    if(swapBuffers) {
        uint8_t* lastBuffer = frontBuffer;
        frontBuffer = backBuffer;
        backBuffer = lastBuffer;
        swapBuffers = false;
    }
  
//    dmxTransmit();
}


bool dmxWaiting() {
    return swapBuffers;
}

void dmxShow() {
    dmxTransmit();

    // If a draw is already pending, skip the new frame
    if(swapBuffers == true) {
        return;
    }

    // Copy the data, but scale it based on the system brightness
    for(int led = 0; led < LED_COUNT; led++) {

        // TODO: Make the pattern order RGB
        uint8_t red =   (drawBuffer[led*3+0]*brightness)/255;
        uint8_t green = (drawBuffer[led*3+1]*brightness)/255;
        uint8_t blue =  (drawBuffer[led*3+2]*brightness)/255;

        for(int bit = 0; bit < 8; bit++) {
            backBuffer[led*3*8 +  0 + bit] = ~((( green >> (7-bit))&0x1) << OUT_OFFSET);
            backBuffer[led*3*8 +  8 + bit] = ~((( red >>   (7-bit))&0x1) << OUT_OFFSET);
            backBuffer[led*3*8 + 16 + bit] = ~((( blue >>  (7-bit))&0x1) << OUT_OFFSET);
        }
    }
    swapBuffers = true;
}

uint8_t* dmxGetPixels() {
    return drawBuffer;
}

void dmxSetBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
}

void dmxSetPixel(int pixel, uint8_t r, uint8_t g, uint8_t b) {
    drawBuffer[pixel*BYTES_PER_PIXEL + 0] = r;
    drawBuffer[pixel*BYTES_PER_PIXEL + 1] = g;
    drawBuffer[pixel*BYTES_PER_PIXEL + 2] = b;
}

