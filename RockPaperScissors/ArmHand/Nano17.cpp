#include "Nano17.h"
#include <stdio.h>
//********************User changed>>>>>>>>>>>>>>>>>>>>>>>>>
#include <iostream>
using namespace std;

Nano17::Nano17()
{
	;
}

Nano17::~Nano17()
{
	;
}

void  Nano17::getCalilibration(double CalMatrix[][6])
{
	for(int i=0;i<6;i++)
	{
		for(int j=0;j<6;j++)
		{
			AmpCalMatrix[i][j] = CalMatrix[i][j];
		}
	}
}

void Nano17::getOffsetVoltage(void)
{
	int i, j;
    const int NumAverage=30;	//Number of repeated times.
	//double TmpVoltage[6];
	
	cout<<"Start getting force offset\n";
    for ( i=0; i<NumAverage; i++)
    {			
		//readVoltage(TmpVoltage);

		for ( j=0; j<6; j++ )
		{
		   UnloadedVoltage[j] += CurrVoltage[j];
		}
		Sleep(10);
    }
	for ( j=0; j<6; j++ )
	{
		UnloadedVoltage[j]/=((double) NumAverage);
		//cout<<"force offset "<<j<<": "<<force_offset[j]<<endl;
	}
	cout<<"End of getting offset."<<endl;
}//End of getOffsetVoltage

void Nano17::setVoltage(double Voltage[])
{
	for (int i=0; i<6; i++)
	{
		CurrVoltage[i] = Voltage[i];			//Get voltage from DAQ
	}
}//End of setVoltage

//Convert voltage to force 
void Nano17::voltage2Force(void)
{
	for (int i=0; i<6; i++)
	{
		CurrVoltage[i] -= UnloadedVoltage[i];	//CompensateVoltage;
	}

	for(int i=0; i<6; i++)
	{
		CurrForce[i]=0;
		for(int j=0; j<6; j++)
		{
			CurrForce[i]+=AmpCalMatrix[i][j]*CurrVoltage[j];
		}
	}
}//End of voltage2Force

void Nano17::getForce(double * MyForce)
{
	for (int i=0; i<6; i++)
	{
		MyForce[i] = CurrForce[i];
	}
}//End of voltage2Force
