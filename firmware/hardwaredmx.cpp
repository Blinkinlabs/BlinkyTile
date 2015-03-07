#include "WProgram.h"
#include "pins_arduino.h"
#include "dmx.h"
#include "blinkytile.h"

#ifdef FAST_DMX

#define BAUD_RATE                250000	// TODO

#define BIT_LENGTH               2            // This isn't exact, but is much faster!
#define BREAK_LENGTH             88
#define MAB_LENGTH               8

#else

#define BAUD_RATE                250000

#define BIT_LENGTH               4
#define BREAK_LENGTH             100
#define MAB_LENGTH               8

#endif

#define BAUD2DIV(baud)  (((F_CPU * 2) + ((baud) >> 1)) / (baud))


#define OUTPUT_BYTES             1 + LED_COUNT*BYTES_PER_PIXEL

uint8_t dataArray[OUTPUT_BYTES];    // Storage for DMX output (0 is the start frame)
uint8_t brightness;

//static volatile uint8_t *dmxPort;
//static uint8_t dmxBit = 0;

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
    DMA_SERQ = DMA_SERQ_SERQ(0);
    // TODO: Interrupt on complete
}

void scheduleDMA(uint8_t* source, int length) {
    DMA_TCD0_SADDR = source;                                    // Address to read from
    DMA_TCD0_NBYTES_MLNO = 1;                                   // Number of bytes to transfer in the minor loop
    DMA_TCD0_CITER_ELINKNO = length;                            // Number of major loops to complete
    DMA_TCD0_BITER_ELINKNO = length;                            // Reset value for CITER (must be equal to CITER)
    DMA_TCD0_CSR = DMA_TCD_CSR_DREQ | DMA_TCD_CSR_START;       // Start the transaction
    DMA_SERQ = DMA_SERQ_SERQ(0);
}

void setupUart() {
    uint32_t divisor = BAUD2DIV(BAUD_RATE);

    SIM_SCGC4 |= SIM_SCGC4_UART1;	// turn on clock
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);	// Configure TX Pin

    UART1_C2 = 0;	// disable transmitter

    UART1_BDH = (divisor >> 13) & 0x1F;  // baud rate
    UART1_BDL = (divisor >> 5) & 0xFF;
    UART1_C4 = divisor & 0x1F;

    UART1_C1 = 0;
    UART1_S2 = 0;
    UART1_C3 = 0;
    UART1_C5 = (uint8_t)(0x80); // TDMAS

    UART1_PFIFO = UART_PFIFO_TXFE;
    UART1_TWFIFO = 0;	// Set the watermark to 0 byte

    UART1_C2 = UART_C2_TIE | UART_C2_TE;
}

void dmxSetup() {
//    dmxPort = portOutputRegister(digitalPinToPort(DATA_PIN));
//    dmxBit = digitalPinToBitMask(DATA_PIN);
//    digitalWrite(DATA_PIN, HIGH);

    
    // Set up the UART
    setupUart();

    // Configure TX DMA
    setupDMA(dataArray, OUTPUT_BYTES, 1);

    dataArray[0] = 0;    // DMX start frame!
    brightness = 255;
}

void dmxSetBrightness(uint8_t newBrightness) {
    brightness = newBrightness;
}

void dmxSetPixel(int pixel, uint8_t r, uint8_t g, uint8_t b) {
    dataArray[pixel*BYTES_PER_PIXEL + 1] = r;
    dataArray[pixel*BYTES_PER_PIXEL + 2] = g;
    dataArray[pixel*BYTES_PER_PIXEL + 3] = b;
}

void dmxSendByte(uint8_t value)
{
    UART1_D = value;
    delayMicroseconds(100); // Should wait for a flag, etc.

/*
    uint32_t begin, target;
    uint8_t mask;

    noInterrupts();
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
    begin = ARM_DWT_CYCCNT;
    *dmxPort = 0;
    target = F_CPU / 250000;
    while (ARM_DWT_CYCCNT - begin < target) ; // wait, start bit
    for (mask=1; mask; mask <<= 1) {
        *dmxPort = (value & mask) ? 1 : 0;
        target += (F_CPU / 250000);
        while (ARM_DWT_CYCCNT - begin < target) ; // wait, data bits
    }
    *dmxPort = 1;
    target += (F_CPU / 125000);
    while (ARM_DWT_CYCCNT - begin < target) ; // wait, 2 stops bits
    interrupts();
*/
}

// Send a DMX frame with new data
void dmxShow() {
  
    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(1);	// Configure TX Pin for digital

    digitalWriteFast(DATA_PIN, LOW);    // Break - 88us
    delayMicroseconds(BREAK_LENGTH);

    digitalWriteFast(DATA_PIN, HIGH);   // MAB - 8 us
    delayMicroseconds(MAB_LENGTH);

    PORTC_PCR4 = PORT_PCR_DSE | PORT_PCR_SRE | PORT_PCR_MUX(3);	// Configure TX Pin for UART
 
    // Set up a TCD and kick off the DMA engine
    scheduleDMA(dataArray, OUTPUT_BYTES);

    // For each address
//    for(int frame = 0; frame < OUTPUT_BYTES; frame++) {    
//        dmxSendByte(dataArray[frame]*(brightness/255.0));
//    }
}

uint8_t* dmxGetPixels() {
    return &dataArray[1]; // Return first pixel in the array
}
