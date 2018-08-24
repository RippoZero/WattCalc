#include <Arduino.h>
#ifndef WattCalc_h
#define WattCalc_h
class WattCalc {
	/*There is one set of functions intended to measure voltage and another set to measure current. 
	This means all the amp functions are the same as the voltage functions so i will only be commenting the voltage functions*/
public:
	/*vRef - reffrence voltage for the adc expressed in V. If you are using a 3.3v micro this will most likley be 3.3. 
	vRes - The resolution of the ADC expressed in decimal. On the uno it's 10 bit wich equates to 1023. On the esp32 it's 12 bit that equate to 4096
	vZeroPoint - The reading you get from the ADC when reading 0v expressed in raw reading from adc. Take a reading from your circuit with no power and set that as vZeroPoint. 
	vGain - Most likley you are using a circuit to step down a higher ac voltage in order to be able to measure it. If you read the voltage of a differential amplifier with a gain of 0.007 you take the gain and elevate it to 0.007^(-1). This will be the gain.*/
	WattCalc(float _vRef, int _vRes, int _vZeroPoint, float _vGain, float _aRef, int _aRes, int _aZeroPoint, float _aGain);
	WattCalc(float _vRef, int _vRes, int _vZeroPoint, float _vGain, float _aRef, int _aRes, int _aZeroPoint, float _aGain, byte _triacPin); // Extra variable to designate pin to control triac for phase angle control if you want that.


	/* 1 -|   ,-'''-.
    |      ,-'       `-.           *Baddabing*
    |    ,'             `.
    |  ,'                 `.
    | /                     \
    |/                       \
----+-------------------------\--------------------------
    |          __           __ \          __           /  __
    |          ||/2         ||  \        3||/2        /  2||
    |                            `.                 ,'
    |                              `.             ,'
    |                                `-.       ,-'
-1 -|                                   `-,,,-'
    10000000000000000000000000-10000000000000000000000001*/
	void getVoltCrossing(int voltReading); //These two functions finds the zero crossing. leave these in the main loop.
	void getAmpCrossing(int ampReading);   //their result is stored in the voltCrossing and ampCrossing variables. Returns 1 if reading passes from below zeroPoint to above zeroPoint and -1 if the opposite happens.
										  
	/*The first function keeps a running average of the phase delay. Put that in the main loop
	and only run the second function when you want to convert the average into power factor. all the
	slow math is contained in calculatePowerFactor so use with caution. */
	void getPhaseDelay();
	float calculatePowerFactor();

	
	void getVoltMidOrdinates(int voltReading); //Tracks the mid-ordinates readings of the waveform. Goes into main loop.
	float calculateVoltageRMS(); //Converts the mid-ordinates tracked by the function above into true RMS voltage in it's input amplitude. it will only be the right amplitude if you put in the right gain. 

	void getAmpMidOrdinates(int ampReading);
	float calculateAmpRMS();

	/*Allternative to the four functions above. does not give true RMS but require way less resources to run.
	leave the getPeak functions in main loop and only call the approximate functions when you want to convert the
	result.*/
	void getVoltPeak(int voltReading);
	int approximateVoltRMS();
	void getAmpPeak(int ampReading);
	int approximateAmpRMS();

	/*If you want to do phase angle control you can use this. Set the pin that controls the triac in the constructor first. You pass this function
	a number between 1 and 100 to tell it how many percent of the phase angle you want the triac to remain off for*/
	void cutPhaseAngle(byte percentOfAngleToCut);

private:
	float vRef;
	int vRes;
	int vZeroPoint;
	float vGain;
	float aRef;
	int aRes;
	int aZeroPoint;
	float aGain;

	int vZeroPointLow; //vZeroPoint plus and minus one to give some leeway when comparing for to find zeroCrossing and maybe trigger the zero crossing a bit early to account for prosessing delay. 
	int vZeroPointHigh;

	int aZeroPointLow; 
	int aZeroPointHigh;

	float vToMains(float vValue); //Converts the readings back into the input amplitude if the external cicuitry. 
	float aToMains(float aValue);

	byte frequency = 50;
	int oneCycleInUs = 1000000 / frequency;
	int quarterCycleUs = oneCycleInUs / 4;

	int voltCrossing; //Where the result from the getVoltCrossing functions above is stored
	int lastVoltReading; //stores the reading from the last cycle

	int ampCrossing;
	int lastAmpReading;

	unsigned long aTimer;
	byte asampleNumber = 0;
	long amidOrdinate[20];
	byte agetSample = 0;
	float aProduct = 0;
	unsigned long asquaredMidOrdinate;

	unsigned long t1 = 0; //Time of first zero crossing
	unsigned long zCrossDelay[5]; //5 cycles of t2 - t1. All of these values are added together then divided by 5 to get smoothing
	unsigned long zCross; //Stores the avredge of the zero crossings
	float zCrossRadians = 0; //zCross converted to radians
	bool counting = 0; //Keeps track of weather we are taking t1 or t2
	byte n = 0; //Keeps track of wich part of the array the reading put into

	unsigned long vTimer; //tracks the time since last zero crossing so we know when to take the next reading. 
	byte sampleNumber = 0; //Number of samples taken. when this reaches 20 it cycles back to 0 and starts filling over the midPrdinate[] array. this happens five times a second.
	long midOrdinate[20]; // stores the mid ordinates of the waveform
	byte getSample = 0; //Tracks weather the getVoltMidOrdinates should be taking a sample or not
	int sampleInterval = 950;
	float vProduct; //stores the product of our calculations until it can be sent as a return value
	/*Lookup used to find when after zero crossing to collect sample.
	Doing these calculations on the fly took to long even with integer math.
	The readings below place 525us between sample. They are also snipped by 2 us to account for 
	a slight offset to one side of the sinewave due to time of calculations*/
	int sampleIntervalLookup[20] = { 
		23, 523, 1048, 1573, 2098,
		2623, 3148, 3673, 4198, 4723,
		5248, 5773, 6298, 6823, 7348,
		7873, 8398, 8923, 9448, 9973
	};
	unsigned long squaredMidOrdinate; //midOrdinate[n] ^ 2

	byte triacPin; //Pin the triac trigger is attached to
	bool triacIsClosed; //keeps track of when the triac is cutting the phase angle. TRUE = triac closed FALSE = triac open.
	unsigned long triacOffTimer; //time triac will remain off each cycle
	int onePercentOfHalfCycle = oneCycleInUs / 200; // One percent of 180 degrees. 
	int preCalculatedWaitTimes[100]; // the wait times for each percent is stored here and fetched so that we do not have to do the calculations on the fly
};
#endif