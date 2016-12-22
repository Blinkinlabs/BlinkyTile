Fadecandy Test Jig
==================

This is a simple one-off test fixture that's designed to facilitate in-circuit debugging of the Fadecandy firmware, and doing reliable low-volume production.

My prototype consists of:

* An ugly hand-wired perfboard
* 0.1"-spaced pogo pins which connect to the Fadecandy board's "hacker port" as well as its output header
* A standard ARM JTAG header for firmware development using an external JTAG adapter
* A 2-LED string of WS2811 LEDs, for verifying the timing of the Fadecandy outputs
* A Teensy 3.0 board which coordinates production and testing tasks

The board is intended for two separate use cases:

* During firmware development, it's a simple breakout board for the JTAG port
* During low-volume production, the Teensy installs firmware and performs electrical testing

This board is a work-in-progress. This directory contains Teensyduino sketches which run on the onboard Teensy. More information about the hardware will be included later.

Hardware tidbits
----------------

* JTAG port uses 10K pull-up resistors to target's' 3.3v rail on all signals.
* Pogo pins are 2x Digi-Key part ED8178-16-ND
* Big green button next to pogo-pins to initiate programming/test

Teensy 3.0 pin assignment
-------------------------

Pin      | Description
-------- | ----------------------------------
Gnd      | Shared ground
Vin      | +5V power for testjig itself
0        | Teensy RX, target TX
1        | Teensy TX, target RX
2        | To ground via green button
3        | target TCLK (SWCLK)
4        | target TMS (SWDIO)
5        | target USB D- (white wire)
6        | target USB D+ (green wire)
7        | target USB shield ground
8        | target USB signal ground
9        | (No connection)
10       | PWM out for power supply control
11       | PASS LED (to ground)
12       | FAIL LED (to ground)
13       | (Teensy built-in LED)
14 (A0)  | target output 0 (via resistor divider) PA4
15 (A1)  | target output 1 (via resistor divider) PB0
16 (A2)  | target output 2 (via resistor divider) PC1
17 (A3)  | target output 3 (via resistor divider)
18 (A4)  | target output 4 (via resistor divider)
19 (A5)  | target output 5 (via resistor divider)
20 (A6)  | target output 6 (via resistor divider) PB1
21 (A7)  | target output 7 (via resistor divider)
22 (A8)  | target 3.3v     (via resistor divider)
23 (A9)  | target VUSB     (via resistor divider)

All analog inputs connect via the same resistor divider:

* 1K between analog input and ground
* 6.8K between analog input and Fadecandy signal
* Use 5% tolerance resistors or better

Target power supply (micro-USB cable) is driven at a variable voltage. This is achieved by buffering a PWM output with an RC filter and op-amp.

* Op-amp should be able to drive ~20mA. I used half an OPA2350, since it was handy.
* Op-amp supply voltage driven by Teensy VUSB (~5V), bypassed with 0.1uF cap
* Configured as a non-inverting amplifier with gain=2
  * Feedback resistor, - to out, 68K
  * Feedback resistor, - to ground, 68K
* Low-pass filter on + input
  * 68K between + and PWM input
  * 0.1uF capacitor between + and ground
* Unused op-amps configured as followers
  * 33K from - to out
  * 33K from + to ground

The Teensy isn't capable of communicating over USB with the Fadecandy board when wired this way, but the USB pins are electrically tested in a minimal way. Note that the USB signal ground and shield ground are set up this way only for electrical testing. During testing and programming, the actual power and signal ground is achieved through the test clip's connection to the output port. Except for VUSB, all signals on the USB connector are used only to electrically test the USB connector itself.

Testjig Firmwares
-----------------

* `production`
	* Programs bootloader and initial firmware image
	* Runs electrical tests
	* Communicates with the DUT processor using Serial Wire Debug
* `serial_passthrough`
	* Teensyduino sketch
    * Appears as a USB serial port device
	* Passes through access to the DUT serial port
	* When the green button is held, acts as a loopback for the DUT serial port, as one way to enter FC-Boot.

Contact
-------

Micah Elizabeth Scott <<micah@scanlime.org>>