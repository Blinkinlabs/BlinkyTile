#include "WProgram.h"
#include "pins_arduino.h"
#include "dmaLed.h"
#include "apa102.h"
#include "blinkytile.h"


// Send 3 zeros at the end of the data stream
#define OUTPUT_BYTES        (4+LED_COUNT*4 + 4)*8


// Check that the output buffer size is sufficient
#if DMA_BUFFER_SIZE < (OUTPUT_BYTES*2)
#error DMA Buffer too small, cannot use apa102 output.
#endif

#define DATA_PIN_OFFSET              4       // Offset of our data pin in port C
#define CLOCK_PIN_OFFSET             3      // Offset of our clock pin in port C

APA102Controller apa102;

namespace APA102 {

uint8_t* frontBuffer;
uint8_t* backBuffer;
bool swapBuffers;

uint8_t ONE = _BV(CLOCK_PIN_OFFSET);

#define TIMER_PERIOD        0x0040
#define TIMER_DATA_PWM      0x0001
#define TIMER_CLOCK_PWM     0x0020

// FTM0 drives our whole operation!
void setupFTM0(){
  FTM0_MODE = FTM_MODE_WPDIS;    // Disable Write Protect

  FTM0_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?
  FTM0_MOD = TIMER_PERIOD;       // Period register
  FTM0_SC |= FTM_SC_CLKS(1) | FTM_SC_PS(0);

  FTM0_MODE |= FTM_MODE_INIT;         // Enable FTM0

  FTM0_C0SC = 0x40          // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM0_C0V = TIMER_CLOCK_PWM;  // Duty cycle of PWM signal

  FTM0_C3SC = 0x40          // Enable interrupt
  | 0x20                    // Mode select: Edge-aligned PWM 
  | 0x04                    // Low-true pulses (inverted)
  | 0x01;                   // Enable DMA out
  FTM0_C3V = TIMER_DATA_PWM;  // Duty cycle of PWM signal

  FTM0_SYNC |= 0x80;        // set PWM value update
}


// TCD0 sets the clock output
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

// TCD3 sets the data and clears the clock clock output
void setupTCD3(uint8_t* source, int minorLoopSize, int majorLoops) {
  DMA_TCD3_SADDR = source;                                        // Address to read from
  DMA_TCD3_SOFF = 1;                                              // Bytes to increment source register between writes 
  DMA_TCD3_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
  DMA_TCD3_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
  DMA_TCD3_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
  DMA_TCD3_DADDR = &GPIOC_PDOR;                                   // Address to write to
  DMA_TCD3_DOFF = 0;                                              // Bytes to increment destination register between write
  DMA_TCD3_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
  DMA_TCD3_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
  DMA_TCD3_DLASTSGA = 0;                                          // Address of next TCD (N/A)
}


void setupTCDs() {
    setupTCD0(&ONE, 1, OUTPUT_BYTES);
    setupTCD3(frontBuffer, 1, OUTPUT_BYTES);
}

// Send a DMX frame with new data
void apa102Transmit() {
    setupTCDs();

    FTM0_SC |= FTM_SC_CLKS(1);
}

} // namespace APA102

// At the end of the DMX frame, start it again
// TODO: This is on channel 3, because channel 0, 1, and 2 are used. Can we stack these somehow?
void dma_ch3_isr(void) {
    DMA_CINT = DMA_CINT_CINT(3);

    // Wait until DMA2 is triggered, then stop the counter
    while(FTM0_CNT < TIMER_CLOCK_PWM) {}

    // TODO: Turning off the timer this late into the cycle causes a small glitch :-/
    FTM0_SC = 0;                   // Turn off the clock so we can update CNTIN and MODULO?

    if(APA102::swapBuffers) {
        uint8_t* lastBuffer = APA102::frontBuffer;
        APA102::frontBuffer = APA102::backBuffer;
        APA102::backBuffer = lastBuffer;
        APA102::swapBuffers = false;
    }

    // TODO: Re-start this chain here  
//    apa102Transmit();
}

void APA102Controller::start() {
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(1);	// Configure TX Pin for digital
    PORTC_PCR3 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(1);	// Configure RX Pin for digital
    GPIOC_PDDR |= _BV(DATA_PIN_OFFSET) | _BV(CLOCK_PIN_OFFSET);

    // DMA
    // Configure DMA
    SIM_SCGC7 |= SIM_SCGC7_DMA;  // Enable DMA clock
    DMA_CR = 0;  // Use default configuration
 
    // Configure the DMA request input for DMA0
    DMA_SERQ = DMA_SERQ_SERQ(0);        // Configure DMA0 to enable the request signal
    DMA_SERQ = DMA_SERQ_SERQ(3);        // Configure DMA2 to enable the request signal
 
    // Enable interrupt on major completion for DMA channel 1
    DMA_TCD3_CSR = DMA_TCD_CSR_INTMAJOR;  // Enable interrupt on major complete
    NVIC_ENABLE_IRQ(IRQ_DMA_CH3);         // Enable interrupt request
 
    // DMAMUX
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX; // Enable DMAMUX clock

    // Timer DMA channel:
    // Configure DMAMUX to trigger DMA0 from FTM0_CH0
    DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG0 = DMAMUX_SOURCE_FTM0_CH0 | DMAMUX_ENABLE;

    // Configure DMAMUX to trigger DMA2 from FTM0_CH3
    DMAMUX0_CHCFG3 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG3 = DMAMUX_SOURCE_FTM0_CH3 | DMAMUX_ENABLE;
 
    // Load this frame of data into the DMA engine
    APA102::setupTCDs();
 
    // FTM
    SIM_SCGC6 |= SIM_SCGC6_FTM0;  // Enable FTM0 clock
    APA102::setupFTM0();

    APA102::frontBuffer = dmaBuffer;
    APA102::backBuffer =  dmaBuffer + OUTPUT_BYTES;
    APA102::swapBuffers = false;

    // Clear the display
    memset(APA102::frontBuffer, 0x00, OUTPUT_BYTES);

    APA102::apa102Transmit();
}


void APA102Controller::stop() {
    FTM0_SC = 0;                        // Disable FTM0 clock

    DMA_TCD3_CSR = 0;                   // Disable interrupt on major complete
    NVIC_DISABLE_IRQ(IRQ_DMA_CH3);      // Disable interrupt request

    DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG3 = DMAMUX_DISABLE;

    SIM_SCGC6 &= ~SIM_SCGC6_FTM0;       // Disable FTM0 clock
    SIM_SCGC6 &= ~SIM_SCGC6_DMAMUX;     // turn off DMA MUX clock
    SIM_SCGC7 &= ~SIM_SCGC7_DMA;        // turn off DMA clock
}


bool APA102Controller::waiting() {
    return APA102::swapBuffers;
}

void APA102Controller::show() {
    APA102::apa102Transmit();

    // If a draw is already pending, skip the new frame
    if(APA102::swapBuffers == true) {
        return;
    }

    // Copy the data, but scale it based on the system brightness
    for(int led = 0; led < LED_COUNT; led++) {

        // TODO: Make the pattern order RGB
        uint8_t red =   (drawBuffer[led*3+0]*brightness)/255;
        uint8_t green = (drawBuffer[led*3+1]*brightness)/255;
        uint8_t blue =  (drawBuffer[led*3+2]*brightness)/255;

        // Shift the remaining bits by 1, since this is a 7 bit LED controller
        for(int bit = 0; bit < 8; bit++) {
            APA102::backBuffer[(4 + led*4)*8 +  0 + bit] = (1 << DATA_PIN_OFFSET);
            APA102::backBuffer[(4 + led*4)*8 +  8 + bit] = ((( green >> (7-bit))&0x1) << DATA_PIN_OFFSET);
            APA102::backBuffer[(4 + led*4)*8 + 16 + bit] = ((( red >>   (7-bit))&0x1) << DATA_PIN_OFFSET);
            APA102::backBuffer[(4 + led*4)*8 + 24 + bit] = ((( blue >>  (7-bit))&0x1) << DATA_PIN_OFFSET);
        }
    }
    APA102::swapBuffers = true;
}


