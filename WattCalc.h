#include <Arduino.h>
#include <stdint.h>
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
	WattCalc(float _vRef, int16_t _vRes, int8_t _vZeroPoint, float _vGain, float _aRef, int16_t _aRes, int8_t _aZeroPoint, float _aGain);
	WattCalc(float _vRef, int16_t _vRes, int8_t _vZeroPoint, float _vGain, float _aRef, int16_t _aRes, int8_t _aZeroPoint, float _aGain, int8_t _triacPin); // Extra variable to designate pin to control triac for phase angle control if you want that.

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
	void getVoltCrossing(int16_t voltReading); //These two functions finds the zero crossing. leave these in the main loop.
	void getAmpCrossing(int16_t ampReading);   //their result is stored in the voltCrossing and ampCrossing variables. Returns 1 if reading passes from below zeroPoint to above zeroPoint and -1 if the opposite happens.
										  
	/*The first function keeps a running average of the phase delay. Put that in the main loop
	and only run the second function when you want to convert the average into power factor. all the
	slow math is contained in calculatePowerFactor so use with caution. */
	void getPhaseDelay();
	float calculatePowerFactor();

	
	void getVoltMidOrdinates(int16_t voltReading); //Tracks the mid-ordinates readings of the waveform. Goes into main loop.
	float calculateVoltageRMS(); //Converts the mid-ordinates tracked by the function above into true RMS voltage in it's input amplitude. it will only be the right amplitude if you put in the right gain. 

	void getAmpMidOrdinates(int16_t ampReading);
	float calculateAmpRMS();

	/*Allternative to the four functions above. does not give true RMS but require way less resources to run.
	leave the getPeak functions in main loop and only call the approximate functions when you want to convert the
	result.*/
	void getVoltPeak(int16_t voltReading);
	int16_t approximateVoltRMS();
	void getAmpPeak(int16_t ampReading);
	int16_t approximateAmpRMS();

	/*If you want to do phase angle control you can use this. Set the pin that controls the triac in the constructor first. You pass this function
	a number between 1 and 100 to tell it how many percent of the phase angle you want the triac to remain off for*/
	void cutPhaseAngle(int8_t percentOfAngleToCut);

private:
	float vRef;
	int16_t vRes;
	int8_t vZeroPoint;
	float vGain;
	float aRef;
	int16_t aRes;
	int8_t aZeroPoint;
	float aGain;

	int8_t vZeroPointLow; //vZeroPoint plus and minus one to give some leeway when comparing for to find zeroCrossing and maybe trigger the zero crossing a bit early to account for prosessing delay. 
	int8_t vZeroPointHigh;

	int8_t aZeroPointLow; 
	int8_t aZeroPointHigh;

	float vToMains(float vValue); //Converts the readings back into the input amplitude if the external cicuitry. 
	float aToMains(float aValue);

	int8_t frequency = 50;
	uint16_t oneCycleInUs = 1000000 / frequency;
	int16_t quarterCycleUs = oneCycleInUs / 4;

	int8_t voltCrossing; //Where the result from the getVoltCrossing functions above is stored
	int16_t lastVoltReading; //stores the reading from the last cycle

	int8_t ampCrossing;
	int16_t lastAmpReading;

	uint32_t aTimer;
	int8_t asampleNumber = 0;
	int32_t amidOrdinate[20];
	int8_t agetSample = 0;
	float aProduct = 0;
	uint32_t asquaredMidOrdinate;

	uint32_t t1 = 0; //Time of first zero crossing
	uint32_t zCrossDelay[5]; //5 cycles of t2 - t1. All of these values are added together then divided by 5 to get smoothing
	uint32_t zCross; //Stores the avredge of the zero crossings
	float zCrossRadians = 0; //zCross converted to radians
	int8_t counting = 0; //Keeps track of weather we are taking t1 or looking for end time. 
	int8_t n = 0; //Keeps track of wich part of the array the reading put into

	uint32_t vTimer; //tracks the time since last zero crossing so we know when to take the next reading. 
	int8_t sampleNumber = 0; //Number of samples taken. when this reaches 20 it cycles back to 0 and starts filling over the midPrdinate[] array. this happens five times a second.
	int16_t midOrdinate[20]; // stores the mid ordinates of the waveform
	int8_t getSample = 0; //Tracks weather the getVoltMidOrdinates should be taking a sample or not

	float vProduct; //stores the product of our calculations until it can be sent as a return value

	/*Lookup used to find when after zero crossing to collect sample.
	Doing these calculations on the fly took to long even with integer math.
	The readings below place 525us between sample. They are also snipped by 2 us to account for 
	a slight offset to one side of the sinewave due to time of calculations*/
	int16_t sampleIntervalLookup[20] = { 
		23, 523, 1048, 1573, 2098,
		2623, 3148, 3673, 4198, 4723,
		5248, 5773, 6298, 6823, 7348,
		7873, 8398, 8923, 9448, 9973
	};
	uint16_t squaredMidOrdinate; //Stores midOrdinate[n] ^ 2

	int8_t triacPin; //Pin the triac trigger is attached to
	int8_t triacIsClosed; //keeps track of when the triac is cutting the phase angle. TRUE = triac closed FALSE = triac open.
	uint32_t triacOffTimer; //time triac will remain off each cycle
	int16_t onePercentOfHalfCycle = oneCycleInUs / 200; // One percent of 180 degrees of the sinewave expressed in microseconds. 
	uint16_t preCalculatedWaitTimes[100]; // the wait times for each percent is stored here and fetched so that we do not have to do the calculations on the fly
};
#endif