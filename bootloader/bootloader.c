/*
 * Fadecandy DFU Bootloader
 * 
 * Copyright (c) 2013 Micah Elizabeth Scott
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdbool.h>
#include "usb_dev.h"
#include "serial.h"
#include "mk20dx128.h"

extern uint32_t boot_token;
static __attribute__ ((section(".appvectors"))) uint32_t appVectors[64];

#define REV_A

#ifdef REV_A

const uint32_t led_bit = 1 << 5;    // LED is on Port D5

const uint32_t button_1_bit = 1 << 7;  // Button 1 on D7
const uint32_t button_2_bit = 1 << 6;  // Button 2 on D6

#else

#warning Board revision not recognized- LED disabled

#endif

static void led_init()
{

#ifdef REV_A
    // Set the status LED on PC5, as an indication that we're in bootloading mode.
    // TODO: Test me!
    PORTD_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_DSE | PORT_PCR_SRE;
    GPIOD_PDDR = led_bit;
    GPIOD_PDOR = led_bit;
#endif

}

static void led_toggle()
{
#ifdef REV_A
    GPIOD_PTOR = led_bit;
#endif
}

static bool test_boot_token()
{
    /*
     * If we find a valid boot token in RAM, the application is asking us explicitly
     * to enter DFU mode. This is used to implement the DFU_DETACH command when the app
     * is running.
     */

    return boot_token == 0x74624346;
}

static bool test_app_missing()
{
    /*
     * If there doesn't seem to be a valid application installed, we always go to
     * bootloader mode.
     */

    uint32_t entry = appVectors[1];
    return entry < 0x00001000 || entry >= 128 * 1024;
}

static bool test_banner_echo()
{
    /*
     * At startup we print this banner out to the serial port.
     * If we see it echo back to us, we enter bootloader mode no matter what.
     * This is intended to be a foolproof way to enter recovery mode, even if other
     * circuitry has been connected to the serial port.
     */

    static char banner[] = "FC-Boot";
    const unsigned bannerLength = sizeof banner - 1;
    unsigned matched = 0;

    // Write banner
    serial_begin(BAUD2DIV(9600));
    serial_write(banner, sizeof banner - 1);

    // Newline is not technically part of the banner, so we can do the RX check
    // at a time when we're sure the other characters have arrived in the RX fifo.
    serial_putchar('\n');
    serial_flush();

    while (matched < bannerLength) {
        if (serial_available() && serial_getchar() == banner[matched]) {
            matched++;
        } else {
            break;
        }
    }

    serial_end();
    return matched == bannerLength;
}

static bool test_user_buttons() {
    /*
     * At startup we test to see if both user buttons are pressed down. If they are,
     * we enter DFU mode no matter what. This is intended to be a recovery mechanism
     * for a misbehaving application that prevents DFU mode.
     */

#ifdef REV_A
    // Read the two status pins.
    // TODO: Test me!
    PORTD_PCR6 = PORT_PCR_MUX(1) | PORT_PCR_PS | PORT_PCR_PE | PORT_PCR_SRE;
    PORTD_PCR7 = PORT_PCR_MUX(1) | PORT_PCR_PS | PORT_PCR_PE | PORT_PCR_SRE;

    GPIOD_PDDR = GPIOD_PDDR & (~button_1_bit) & (~button_2_bit);
    uint32_t status = GPIOD_PDIR & (button_1_bit | button_2_bit);

    return status == 0;

#else
    return false;
#endif
}

static void app_launch()
{
    // Relocate IVT to application flash
    __disable_irq();
    SCB_VTOR = (uint32_t) &appVectors[0];

    // Refresh watchdog right before launching app
    watchdog_refresh();

    // Clear the boot token, so we don't repeatedly enter DFU mode.
    boot_token = 0;

    asm volatile (
        "mov lr, %0 \n\t"
        "mov sp, %1 \n\t"
        "bx %2 \n\t"
        : : "r" (0xFFFFFFFF),
            "r" (appVectors[0]),
            "r" (appVectors[1]) );
}

int main()
{
//    if (test_banner_echo() || test_app_missing() || test_boot_token()) {
    if (test_user_buttons() || test_app_missing() || test_boot_token()) {
        unsigned i, j;

        // We're doing DFU mode!
        led_init();
        dfu_init();
        usb_init();

        // Wait for firmware download
        while (dfu_getstate() != dfuMANIFEST) {
            watchdog_refresh();
        }

        // Clear boot token, to enter the new application
        boot_token = 0;

        // Wait a little bit longer, while flashing the LED.
        for (i = 11; i; --i) {
            led_toggle();
            for (j = 100000; j; --j) {
                watchdog_refresh();
            }
        }

        // USB disconnect and reboot, using watchdog to time 10ms.
        watchdog_refresh();
        __disable_irq();
        USB0_CONTROL = 0;
        while (1);
    }

    app_launch();
    return 0;
}
