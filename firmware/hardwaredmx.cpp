#include "WProgram.h"
#include "pins_arduino.h"
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


#define INPUT_BYTES              LED_COUNT*BYTES_PER_PIXEL
#define OUTPUT_BYTES             1 + LED_COUNT*BYTES_PER_PIXEL

uint8_t drawBuffer[INPUT_BYTES];        // Buffer for the user to draw into
uint8_t dmaBuffer[2][OUTPUT_BYTES];     // Double-buffered output for the DMA engine
uint8_t* frontBuffer;
uint8_t* backBuffer;
bool swapBuffers;

uint8_t brightness;

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

void dmxSetup() {
    // Set up the UART
    setupUart();

    // Configure TX DMA
    setupDMA(frontBuffer, OUTPUT_BYTES, 1);

    frontBuffer = dmaBuffer[0];
    backBuffer = dmaBuffer[1];
    swapBuffers = false;

    // Clear the display
    memset(drawBuffer, 0, INPUT_BYTES);
    memset(frontBuffer, 0, OUTPUT_BYTES);

    backBuffer[0] = 0x0;
    frontBuffer[0] = 0x0;

    brightness = 255;

    dmxTransmit();
}


// At the end of the DMX frame, start it again
void dma_ch0_isr(void) {
    DMA_CINT = DMA_CINT_CINT(0);
    UART1_C2 &= ~UART_C2_TIE;

    if(swapBuffers) {
        uint8_t* lastBuffer = frontBuffer;
        frontBuffer = backBuffer;
        backBuffer = lastBuffer;
        swapBuffers = false;
    }
  
    dmxTransmit();
}

bool dmxWaiting() {
    return swapBuffers;
}

void dmxShow() {
    // If a draw is already pending, skip the new frame
    if(swapBuffers == true) {
        return;
    }

    // Copy the data, but scale it based on the system brightness
    for(int i = 0; i < INPUT_BYTES; i++) {
        backBuffer[i + 1] = (drawBuffer[i]*brightness)/255;
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

