#include "WProgram.h"
#include "pins_arduino.h"
#include "blinkytile.h"
#include "patterns.h"
#include "HardwareSerial.h"
#include "dmx.h"

void color_loop() {  
  static int j = 0;
  static int f = 0;
  static int k = 0;
  
  for (uint16_t i = 0; i < LED_COUNT; i+=1) {
    dmxWritePixel(i+1,
      128*(1+sin(i/30.0 + j/1.3       )),
      128*(1+sin(i/10.0 + f/2.9)),
      128*(1+sin(i/25.0 + k/5.6))
    );
  }
  
  j = j + 1;
  f = f + 1;
  k = k + 2;
}

void white_loop() {
    for (uint16_t i = 0; i < LED_COUNT; i+=1) {
        dmxWritePixel(i+1, 255, 255, 255);
    }
}


void serial_loop() {
  static int pixelIndex;

  // while(true) {
  //   watchdog_refresh();

    if(serial_available() > 2) {

      uint8_t buffer[3]; // Buffer to store three incoming bytes used to compile a single LED color

      for (uint8_t x=0; x<3; x++) { // Read three incoming bytes
        uint8_t c = serial_getchar();
        
        if (c < 255) {
          buffer[x] = c; // Using 255 as a latch semaphore
        }
        else {
          dmxShow();
          pixelIndex = 0;
          
          // BUTTON_IN (D10):   07 - 0111
          // EXTRA_PIN_A(D7):          11 - 1011
          // EXTRA_PIN_B (D11):        13 - 1101
          // ANALOG_INPUT (A9): 14 - 1110

          // char c = (digitalRead(BUTTON_IN)    << 3)
          //        | (digitalRead(EXTRA_PIN_A)  << 2)
          //        | (digitalRead(EXTRA_PIN_B)  << 1)
          //        | (digitalRead(ANALOG_INPUT)     );
          // Serial.write(c);
          
          break;
        }

        if (x == 2) {   // If we received three serial bytes
          if(pixelIndex == LED_COUNT) break; // Prevent overflow by ignoring the pixel data beyond LED_COUNT
//          leds[pixelIndex] = CRGB(buffer[0], buffer[1], buffer[2]);
          dmxWritePixel(pixelIndex+1, buffer[0], buffer[1], buffer[2]);
          pixelIndex++;
        }
      }
    }
//  }
}