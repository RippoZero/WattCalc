#include <Arduino.h>
#include <WattCalc.h>
WattCalc::WattCalc(float _vRef, int _vRes, int _vZeroPoint, float _vGain, float _aRef, int _aRes, int _aZeroPoint, float _aGain) {
	vRef = _vRef;
	vRes = _vRes;
	vZeroPoint = _vZeroPoint;
	vGain = _vGain;
	aRef = _aRef;
	aRes = _aRes;
	aZeroPoint = _aZeroPoint;
	aGain = _aGain;
	vZeroPointLow = vZeroPoint - 1;
	vZeroPointHigh = vZeroPoint + 1;
	aZeroPointLow = aZeroPoint - 1; 
	aZeroPointHigh = aZeroPoint + 1;
}

WattCalc::WattCalc(float _vRef, int _vRes, int _vZeroPoint, float _vGain, float _aRef, int _aRes, int _aZeroPoint, float _aGain, byte _triacPin) {
	vRef = _vRef;
	vRes = _vRes;
	vZeroPoint = _vZeroPoint;
	vGain = _vGain;
	aRef = _aRef;
	aRes = _aRes;
	aZeroPoint = _aZeroPoint;
	aGain = _aGain;
	triacPin = _triacPin;
	vZeroPointLow = vZeroPoint - 1;
	vZeroPointHigh = vZeroPoint + 1;
	aZeroPointLow = aZeroPoint - 1;
	aZeroPointHigh = aZeroPoint + 1;
	pinMode(triacPin, OUTPUT);
	for (uint8_t n = 0; n < 100; n++) {
		preCalculatedWaitTimes[n+1] = onePercentOfHalfCycle * (n+1);
	}
}

float WattCalc::vToMains(float vValue) {
	vValue = vValue / vRes * vRef;
	vValue = vValue * vGain;
	return vValue;
}

float WattCalc::aToMains(float aValue) {
	aValue = aValue / aRes * aRef;
	aValue = aValue * aGain;
	return aValue;
}

void WattCalc::getVoltCrossing(int voltReading) {
	if (triacIsClosed == HIGH) { //if the triac is closed this reports a zero instead of getting false possitives from having readings floating around 0.
		voltCrossing = 0;
		return;
	}
	if (voltReading > vZeroPoint && lastVoltReading < vZeroPointHigh) {
		lastVoltReading = voltReading;
		voltCrossing = 1;
		return;
	}
	if (voltReading < vZeroPoint && lastVoltReading > vZeroPointLow) {
		lastVoltReading = voltReading;
		voltCrossing = -1;
		return;
	}
	lastVoltReading = voltReading;
	voltCrossing = 0;
}

void WattCalc::getAmpCrossing(int ampReading) {
	if (triacIsClosed == HIGH) {
		ampCrossing = 0;
		return;
	}
	if (ampReading > aZeroPoint && lastAmpReading < aZeroPointHigh) {
		lastAmpReading = ampReading;
		ampCrossing = 1;
		return;
	}
	if (ampReading < aZeroPoint && lastAmpReading > aZeroPointLow) {
		lastAmpReading = ampReading;
		ampCrossing = -1;
		return;
	}
	lastAmpReading = ampReading;
	ampCrossing = 0;
}

void WattCalc::getPhaseDelay() {
	if (voltCrossing == 1 && counting == 0) {
		t1 = micros();
		counting = 1;
		return;
	}
	if (ampCrossing == 1 && counting == 1) {
		zCrossDelay[n] = micros() - t1;
		counting = 0;
	}
}

float WattCalc::calculatePowerFactor() { //converts the running avredge above to power factor with strenuous floating point path. only run once a second or so.
	zCross = zCrossDelay[0] + zCrossDelay[1] + zCrossDelay[2] + zCrossDelay[3] + zCrossDelay[4];
	zCross = zCross / 5;
	zCrossRadians = (float)zCross / oneCycleInUs * 6.283185;
	n++;
	if (n == 5) {
		n = 0;
	}
	return cos(zCrossRadians);
}

void WattCalc::getVoltMidOrdinates(int voltReading) { //keeps a running sample of all the mid ordinates required to calculate true rms.
	if (voltCrossing != 0 && getSample == 0) {
		vTimer = micros();
		getSample = 1;
		return;
	}
	if (getSample == 1 && micros() - vTimer >= sampleIntervalLookup[sampleNumber]) {
		midOrdinate[sampleNumber] = voltReading;
		sampleNumber++;
		getSample = 0;
		if (sampleNumber == 20) {
			sampleNumber = 0;
		}
	}
}

float WattCalc::calculateVoltageRMS() {	//calculates the rms voltage from the running avredge above.
	for (byte i = 0; i < 20; i++) {
		if (midOrdinate[i] < vZeroPoint) {
			midOrdinate[i] = vZeroPoint - midOrdinate[i];
			midOrdinate[i] = midOrdinate[i] + vZeroPoint;
		}
		midOrdinate[i] = midOrdinate[i] - vZeroPoint;
		squaredMidOrdinate = midOrdinate[i] * midOrdinate[i];
		vProduct += squaredMidOrdinate;
	}
	vProduct = vProduct / 20;
	vProduct = sqrt(vProduct);
	vProduct = vToMains(vProduct);
	return vProduct;
}

void WattCalc::getAmpMidOrdinates(int ampReading) { //keeps a running sample of all the mid ordinates required to calculate true rms.
	if (ampCrossing != 0 && agetSample == 0) {
		aTimer = micros();
		agetSample = 1;
		return;
	}
	if (agetSample == 1 && micros() - aTimer >= sampleIntervalLookup[sampleNumber]) {
		amidOrdinate[asampleNumber] = ampReading;
		asampleNumber++;
		agetSample = 0;
		if (asampleNumber == 20) {
			asampleNumber = 0;
		}
	}
}

float WattCalc::calculateAmpRMS() {//calculates the rms amprage from the running avredge above.
	for (byte i = 0; i < 20; i++) {
		if (amidOrdinate[i] < aZeroPoint) {
			amidOrdinate[i] = aZeroPoint - amidOrdinate[i];
			amidOrdinate[i] = amidOrdinate[i] + aZeroPoint;
		}
		amidOrdinate[i] = amidOrdinate[i] - aZeroPoint;
		asquaredMidOrdinate = amidOrdinate[i] * amidOrdinate[i];
		aProduct = aProduct + asquaredMidOrdinate;
	}
	aProduct = aProduct / 20;
	aProduct = sqrt(aProduct);
	aProduct = aToMains(aProduct);
	return aProduct;
}

void WattCalc::getVoltPeak(int voltReading) {
	if (voltCrossing == 1 && getSample == 0) {
		vTimer = micros();
		getSample = 1;
		return;
	}
	if (getSample == 1 && micros() - vTimer >= quarterCycleUs) { //Takes a sample of the peak value.
		vProduct = voltReading;
		getSample = 0;
	}
}

int WattCalc::approximateVoltRMS() {
	vProduct = vProduct - vZeroPoint;
	vProduct = vProduct / 1.414;
	vProduct = vToMains(vProduct);
	return vProduct;
}

void WattCalc::getAmpPeak(int ampReading) {
	if (ampCrossing == 1 && agetSample == 0) {
		aTimer = micros();
		agetSample = 1;
		return;
	}
	if (agetSample == 1 && micros() - aTimer > quarterCycleUs) { //Takes a sample of the peak value.
		aProduct = ampReading;
		agetSample = 0;
	}
}

int WattCalc::approximateAmpRMS() {
	aProduct = aProduct - aZeroPoint;
	aProduct = aProduct / 1.414;
	aProduct = aToMains(aProduct);
	return aProduct;
}

void WattCalc::cutPhaseAngle(byte percentOfAngleToCut) {
	if (percentOfAngleToCut == 100) {
		digitalWrite(triacPin, LOW);
		triacIsClosed = HIGH;
		return;
}	
		if (voltCrossing != 0) {
			digitalWrite(triacPin, LOW);
			triacIsClosed = HIGH;
			triacOffTimer = micros();
			return;
		}
		if (micros() - triacOffTimer > preCalculatedWaitTimes[percentOfAngleToCut]) {
			digitalWrite(triacPin, HIGH);
			triacIsClosed = LOW;
		}
}


