Note:

For debug messages to work correctly, the Teensyduino build file needs to be modified. In:

	/c/Program Files (x86)/Arduino/hardware/teensy/avr/boards.txt


Change the line:

	teensy30.build.flags.ld=-Wl,--gc-sections,--relax,--defsym=__rtc_localtime={extra.time.local} "-T{build.core.path}/mk20dx128.ld"

to:

	teensy30.build.flags.ld=-Wl,-u_printf_float,--gc-sections,--relax,--defsym=__rtc_localtime={extra.time.local} "-T{build.core.path}/mk20dx128.ld"
