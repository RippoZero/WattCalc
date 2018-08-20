# WattCalc
General purpose library for converting AC waveforms into RMS values. Also allows you to get the power factor and do phase angle control.The library does not block the execution of any other code if used right. Also comes with four functions that takes the peak of the sinewave and divides it with the square root of two. These require way less resources from the micro to run but only give the correct reading if the wavefunction is a pure sinusoid.

My goal with the library was to create a library that can be used in most wattmeter projects. Don't hesitate to tell me if you find any shortcommings or struggle to apply it to your own project. 
