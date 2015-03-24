#include "WProgram.h"
#include "pins_arduino.h"
#include "dmaLed.h"
#include "dmx.h"
#include "blinkytile.h"

#ifdef FAST_DMX

#define BAUD_RATE                250000	// TODO

#define FRAME_SPACING            4000         // uS between frames
#define BREAK_LENGTH             88
#define MAB_LENGTH               8

#else

#define BAUD_RATE                250000

#define FRAME_SPACING            4000         // uS between frames
#define BREAK_LENGTH             100
#define MAB_LENGTH               8

#endif

#define BAUD2DIV(baud)  (((F_CPU * 2) + ((baud) >> 1)) / (baud))


#define OUTPUT_BYTES             1 + LED_COUNT*BYTES_PER_PIXEL

// Check that the output buffer size is sufficient
#if DMA_BUFFER_SIZE < OUTPUT_BYTES*2
#error DMA Buffer too small, cannot use DMX output.
#endif

namespace DMX {

uint8_t* frontBuffer;
uint8_t* backBuffer;
bool swapBuffers;


// TCD0 writes DMX channel data to the UART 
void setupDMA(uint8_t* source, int minorLoopSize, int majorLoops) {
    // Set up the DMA peripheral
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX;  // Enable clocks
    SIM_SCGC7 |= SIM_SCGC7_DMA;
 
    DMA_TCD0_SADDR = source;                                        // Address to read from
    DMA_TCD0_SOFF = 1;                                              // Bytes to increment source register between writes 
    DMA_TCD0_SLAST = 0;                                             // Bytes to add after a major iteration count (N/A)
 
    DMA_TCD0_DADDR = &UART1_D;                                      // Address to write to
    DMA_TCD0_DOFF = 0;                                              // Bytes to increment destination register between write
    DMA_TCD0_DLASTSGA = 0;                                          // Address of next TCD (N/A)
 
    DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(0) | DMA_TCD_ATTR_DSIZE(0);  // 8-bit input and output
    DMA_TCD0_NBYTES_MLNO = minorLoopSize;                           // Number of bytes to transfer in the minor loop
    DMA_TCD0_CITER_ELINKNO = majorLoops;                            // Number of major loops to complete
    DMA_TCD0_BITER_ELINKNO = majorLoops;                            // Reset value for CITER (must be equal to CITER)
 
    // Timer DMA channel:
    // Configure DMAMUX to trigger DMA0 from FTM1_CH0
    DMAMUX0_CHCFG0 = DMAMUX_DISABLE;
    DMAMUX0_CHCFG0 = DMAMUX_SOURCE_UART1_TX | DMAMUX_ENABLE;
    DMA_TCD0_CSR = DMA_TCD_CSR_DREQ | DMA_TCD_CSR_START;       // Start the transaction

    // Enable interrupt on major completion for DMX data
    DMA_TCD0_CSR = DMA_TCD_CSR_INTMAJOR;  // Enable interrupt on major complete
    NVIC_ENABLE_IRQ(IRQ_DMA_CH0);         // Enable interrupt request
}

void scheduleDMA(uint8_t* source, int length) {
    DMA_TCD0_SADDR = source;                                    // Address to read from
    DMA_TCD0_NBYTES_MLNO = 1;                                   // Number of bytes to transfer in the minor loop
    DMA_TCD0_CITER_ELINKNO = length;                            // Number of major loops to complete
    DMA_TCD0_BITER_ELINKNO = length;                            // Reset value for CITER (must be equal to CITER)
    DMA_SERQ = DMA_SERQ_SERQ(0);

    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);	// Configure TX Pin for UART
}

void setupUart() {
    uint32_t divisor = BAUD2DIV(BAUD_RATE);

    SIM_SCGC4 |= SIM_SCGC4_UART1;	// turn on clock
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);	// Configure TX Pin

    UART1_C2 = 0;	// disable transmitter

    UART1_BDH = (divisor >> 13) & 0x1F;  // baud rate
    UART1_BDL = (divisor >> 5) & 0xFF;
    UART1_C4 = divisor & 0x1F;

    UART1_C1 = UART_C1_M;
    UART1_S2 = 0;
    UART1_C3 = (uint8_t)(0x40);  // 9th data bit as faux stop bit
    UART1_C5 = (uint8_t)(0x80); // TDMAS

    UART1_PFIFO = UART_PFIFO_TXFE;
    UART1_TWFIFO = 0;	// Set the watermark to 0 byte

    //UART1_C2 = UART_C2_TIE | UART_C2_TE;
    UART1_C2 = UART_C2_TE;
}

// Send a DMX frame with new data
void dmxTransmit() {
    // TODO: Use the break hardware here?
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(1);	// Configure TX Pin for digital

    // TODO: Do this with a timer instead of hard delay here?
    digitalWriteFast(DATA_PIN, HIGH);    // Break - 88us
    delayMicroseconds(FRAME_SPACING);

    digitalWriteFast(DATA_PIN, LOW);    // Break - 88us
    delayMicroseconds(BREAK_LENGTH);

    digitalWriteFast(DATA_PIN, HIGH);   // MAB - 8 us
    delayMicroseconds(MAB_LENGTH);

    // Set up a TCD and kick off the DMA engine
    scheduleDMA(frontBuffer, OUTPUT_BYTES);

    UART1_C2 |= UART_C2_TIE;
}


}; // namespace DMX


// At the end of the DMX frame, start it again
void dma_ch0_isr(void) {
    DMA_CINT = DMA_CINT_CINT(0);
    UART1_C2 &= ~UART_C2_TIE;

    if(DMX::swapBuffers) {
        uint8_t* lastBuffer = DMX::frontBuffer;
        DMX::frontBuffer = DMX::backBuffer;
        DMX::backBuffer = lastBuffer;
        DMX::swapBuffers = false;
    }
  
    DMX::dmxTransmit();
}

void dmxSetup() {

    DMX::frontBuffer = dmaBuffer;
    DMX::backBuffer =  dmaBuffer + OUTPUT_BYTES;
    DMX::swapBuffers = false;

    // Set up the UART
    DMX::setupUart();
    // Configure TX DMA
    DMX::setupDMA(DMX::frontBuffer, OUTPUT_BYTES, 1);

    // Clear the display
    memset(DMX::frontBuffer, 0, OUTPUT_BYTES);

    DMX::backBuffer[0] = 0x0;
    DMX::frontBuffer[0] = 0x0;

    DMX::dmxTransmit();
}


void dmxStop() {
    // TODO: Stop!
}

bool dmxWaiting() {
    return DMX::swapBuffers;
}

void dmxShow() {
    // If a draw is already pending, skip the new frame
    if(DMX::swapBuffers == true) {
        return;
    }

    // Copy the data, but scale it based on the system brightness
    for(int i = 0; i < DRAW_BUFFER_SIZE; i++) {
        DMX::backBuffer[i + 1] = (drawBuffer[i]*brightness)/255;
    }
    DMX::swapBuffers = true;
}

