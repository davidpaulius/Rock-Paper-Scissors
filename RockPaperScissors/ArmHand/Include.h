#ifndef CINCLUDEH
#define CINCLUDEH

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <intrin.h>
#include <cmath>
#include <conio.h>
#include <time.h>

#include "Nano17.h"
#include "Dask.h"
#include "BHand.h"
#include "MyRobot.h"

using namespace std;

//Read Force iterations
#define numIterations 1000	

Nano17 Sensor0;
Nano17 Sensor1;

//DAQ Board variables
#define NUMChannel	12

U16 range=AD_B_10_V;
I16 card, err, card_num=0;


//*********************************Hand Code***************************************
void Error(void);
void Initialize(int port);		//  Initialize hand, set timeouts and baud rate
void PrepareRealTime(void);		//  Set parameters, allocate data buffers, load files
int RunRealTime(void);			//  Run RealRime loop, return 1 if interrupted with a key
int Execute();

#endif