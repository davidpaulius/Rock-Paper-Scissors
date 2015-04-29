#ifndef CNano17H
#define CNano17H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <intrin.h>


class Nano17
{
private:
	double AmpCalMatrix[6][6];	//
	double UnloadedVoltage[6];
	double CurrVoltage[6];
	double CurrForce[6];

public:
	Nano17();
	~Nano17();

	void getCalilibration(double CalMatrix[][6]);
	void readVoltage(double CurrVoltage[]);
	void getOffsetVoltage(void);
	void setVoltage(double Voltage[]);
	void voltage2Force(void);
	void getForce(double * MyForce);

};

#endif