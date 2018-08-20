/*
 * This sketch is written for the ESP32 and won't run on anything else. 
 */
#include <WattCalc.h>

TaskHandle_t Task1;
WattCalc Tidemann (5, 255, 127, 142.85, 5, 255, 127, 142.85);


//The block of code below starts a task for the the first core of the ESP32 and simulate the sinewaves. 

//Normally the wavefunctions would be read from external circuitry but this is a good way to test the library without having to  use anything but the ESP32.

/********************************************************************************************************************************************************************************************/
byte sineLookup[256] = {
  128, 131, 134, 137, 140, 143, 146, 149,
  152, 156, 159, 162, 165, 168, 171, 174,
  176, 179, 182, 185, 188, 191, 193, 196,
  199, 201, 204, 206, 209, 211, 213, 216,
  218, 220, 222, 224, 226, 228, 230, 232,
  234, 235, 237, 239, 240, 242, 243, 244,
  246, 247, 248, 249, 250, 251, 251, 252,
  253, 253, 254, 254, 254, 255, 255, 255,
  255, 255, 255, 255, 254, 254, 253, 253,
  252, 252, 251, 250, 249, 248, 247, 246,
  245, 244, 242, 241, 239, 238, 236, 235,
  233, 231, 229, 227, 225, 223, 221, 219,
  217, 215, 212, 210, 207, 205, 202, 200,
  197, 195, 192, 189, 186, 184, 181, 178,
  175, 172, 169, 166, 163, 160, 157, 154,
  151, 148, 145, 142, 138, 135, 132, 129,
  126, 123, 120, 117, 113, 110, 107, 104,
  101, 98, 95, 92, 89, 86, 83, 80,
  77, 74, 71, 69, 66, 63, 60, 58,
  55, 53, 50, 48, 45, 43, 40, 38,
  36, 34, 32, 30, 28, 26, 24, 22,
  20, 19, 17, 16, 14, 13, 11, 10,
  9, 8, 7, 6, 5, 4, 3, 3,
  2, 2, 1, 1, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 2, 2,
  3, 4, 4, 5, 6, 7, 8, 9,
  11, 12, 13, 15, 16, 18, 20, 21,
  23, 25, 27, 29, 31, 33, 35, 37,
  39, 42, 44, 46, 49, 51, 54, 56,
  59, 62, 64, 67, 70, 73, 76, 79,
  81, 84, 87, 90, 93, 96, 99, 103,
  106, 109, 112, 115, 118, 121, 124, 128
};
int i;
int i2;
byte frequency = 50;
int oneCycleInUs = 1000000 / frequency;
int intervalWait = oneCycleInUs / 260; //Should be divided by 255 but had to up that number to make the frequency 50hz
unsigned long timer;
int dY, dY2;

void core0( void * parameter ) {
  for (;;) {
    dY = sineLookup[i];
    dY2 = sineLookup[i2];
    if (micros() - timer > intervalWait) {
      i++;
      timer = micros();
    }
    i2 = i - 20;
    if (i2 < 0) {
      i2 = i2 + 255;
    }
    if (i == 255) {
      i = 0;
    }
  }
}

void setup() {
  Serial.begin(19200);
  xTaskCreatePinnedToCore(
    core0,
    "Task_1",
    1000,
    NULL,
    1,
    &Task1,
    0);
}
/************************************************************************************************************************************************************************************************/

  unsigned long timer2 = micros();
  float meanVoltage;
  float meanAmp;
  float powerFactor;

void loop() {
  /*The five functions below need to be in the mian loop. You pick wich you need. they run quick since they only contain integer math*/
  Tidemann.getVoltCrossing(dY);
  Tidemann.getAmpCrossing(dY2);
  Tidemann.getPhaseDelay();
  Tidemann.getVoltMidOrdinates(dY);
  Tidemann.getAmpMidOrdinates(dY2);

  /*The functions inside this if statement contain all the slow floating point math. These should only be run when you want to update the reading. I recomend no more than once a second.*/
  if (micros() - timer2 > 1000000) {
      powerFactor = Tidemann.calculatePowerFactor();
      meanVoltage = Tidemann.calculateVoltageRMS();
      meanAmp = Tidemann.calculateAmpRMS();
      Serial.print(powerFactor);
      Serial.print(" ");
      Serial.print(meanVoltage);
      Serial.print(" ");
      Serial.println(meanAmp);
      timer2 = micros();
    }
}
