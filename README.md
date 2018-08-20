# WattCalc
General purpose library for converting AC waveforms into RMS values. Also allows you to get the power factor and do phase angle control.The library does not block the execution of any other code if used right. Comes with four functions that takes the peak of the sinewave and divides it with the square root of two. These require way less resources from the micro to run but only give the correct reading if the wavefunction is a pure sinusoid.

The goal of the library is to do most of the calculations associated with wattmeter projects. The library has as of yet not been tested on acuall hardware. 
