Test D21 at 48MHz w/ Internal OSC32
===================================

Working example of Atmel ASF conf_clocks.h for an ATSAMD21E18 running at 48MHz using the DFLL fed by the 
internal OSC32K oscillator.  Includes basic systick delay routines and blinks a pin.

Within main.c, a version of code that does not use ASF but rather tweaks the registers directly is provided as 
a control example.

Note that Atmel datasheet documentation states that only 1 flash wait state is necessary to run 48MHz when
powered between 2.7V-3.3V.  However, in practice, even at 2.9V measured, 2 wait states are necessary.