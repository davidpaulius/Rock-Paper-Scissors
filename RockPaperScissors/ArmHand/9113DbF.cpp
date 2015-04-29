#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <mmsystem.h>
#include "dask.h"
#include "include.h"
//#include "Nano17.h"
using namespace std;

#define NUMChannel	12

U16 range=AD_B_10_V;
I16 card, err, card_num=0;


int main()
{
	double Force0[6], Force1[6];

	SensorInitialize();
	//Start reading.
	readForce();
	Sensor0.getForce(Force0);
	Sensor1.getForce(Force1);
	
	for(int i=0; i<6; i++)
	{
		cout<<Force0[i]<<", ";
	}
	cout<<endl;

	for(int i=0; i<6; i++)
	{
		cout<<Force1[i]<<", ";
	}
	cout<<endl;

	return 0;
}
void SensorInitialize()
{
	double Voltage0[6], Voltage1[6];

	DAQInitialize();
	setMatrix();	//Send calibration matrix to sensor object.
	readVoltage(Voltage0, Voltage1);
	//get unloaded voltage for sensor 0;
	Sensor0.setVoltage(Voltage0);
	Sensor0.getOffsetVoltage();

		Sensor1.setVoltage(Voltage1);
	Sensor1.getOffsetVoltage();
}//End of SensorInitialize();

void readForce(void)
{
	double Voltage0[6], Voltage1[6];

	readVoltage(Voltage0, Voltage1);
	//Get forces from sensor 0
	Sensor0.setVoltage(Voltage0);
	Sensor0.voltage2Force();
//Get forces from sensor 1
	Sensor1.setVoltage(Voltage1);
	Sensor1.voltage2Force();
}//End of readForce

void DAQInitialize()
{
	I16 card, card_num=0;	
	if ((card=Register_Card (PCI_9113, card_num)) <0 ) {
		printf("Register_Card error=%d", card);
		exit(1);
	}

	if (err!=0)
	{
		printf("AI_ContReadChannel error=%d", err);
		exit(1);
	}
}//End of DAQInitialize

void readVoltage(double * voltage0, double * voltage1)
{
	U16 j=0;
	double voltage[NUMChannel];

	for(j = 0; j<NUMChannel; j++)
	{
		//	I16 AI_VReadChannel (U16 CardNumber, U16 Channel,U16 AdRange, F64 *voltage)
		err = AI_VReadChannel (card, j, range, &voltage[j]);
		//cout<<j<<"="<<voltage[i][j]<<endl;

				if (err!=0)
		{
			printf("AI_ContReadChannel error=%d", err);
			exit(1);
		}
	}

for(int i=0; i<6; i++)
{
	voltage0[i] = voltage[i];
	voltage1[i] = voltage[i+6];
}
}//End of ReadVoltage


void setMatrix()
{
	double AmpCalMatrix0[6][6], AmpCalMatrix1[6][6];
	//9637  Sensor RawCalMatrix
	double RawCalMatrix0[6][6]={
		{-3.486208493,	-6.903041322,	66.60394089,	-1344.960447,	-68.13284139,	1351.288548},
		{-32.10924069,	1577.909451,	24.11977675,	-778.0418049,	41.0139806,	-792.2703216},
		{1464.497848,	65.43700081,	1470.334179,	53.87110492,	1477.512428,	49.34682443},
		{-249.6565943,	9511.70975,	8331.229364,	-4403.087269,	-7965.37715,	-5059.008845},
		{-9287.433687,	-394.6154674,	4443.969903,	8298.080292,	5277.503892,	-7966.893172},
		{-186.3974957,	5571.762842,	-271.5403687,	5875.155556,	-226.1747232,	5780.014724}
	};

	//9633 Sensor RawCalMatrix
	double RawCalMatrix1[6][6]={
		{-3.794840232,	-32.1044324,	39.94592133,	-1306.133223,	-70.19715221,	1321.098895},
		{-120.2108566,	1510.727545,	22.80053749,	-788.4841105,	23.3344147,	-720.190785},
		{1453.424961,	50.83435978,	1451.081347,	57.9763995,	1451.083869,	-38.40427872},
		{-700.3162788,	9053.763321,	8004.033968,	-4397.889068,	-8342.572299,	-4096.112947},
		{-9137.205675,	-152.4137472,	4760.026035,	8084.07326,	4902.16003,	-8065.122189},
		{-495.0328877,	5526.264106,	-84.83427486,	5467.851542,	-285.4175244,	5777.535077}};


		double gain[6] ={36.752592,	36.752592,	36.765379,	36.752592,	36.739814,	36.739814};
		double excitation = 10.094;

		for(int i=0; i<6; i++)
		{
			for(int j=0; j<6; j++)
			{
				AmpCalMatrix0[i][j]= RawCalMatrix0[i][j]/gain[j]/excitation;	
				AmpCalMatrix1[i][j]= RawCalMatrix1[i][j]/gain[j]/excitation;	
			}
		}

		Sensor0.getCalilibration(AmpCalMatrix0);	
		Sensor1.getCalilibration(AmpCalMatrix1);

}

//int main()
//{
//	U16 j=0;
//	F64 voltage[1000][18];
//	if ((card=Register_Card (PCI_9113, card_num)) <0 ) {
//		printf("Register_Card error=%d", card);
//		exit(1);
//	}
//
//	for(int i=0; i<11; i++)
//	{
//		for(j = 0; j<11; j++)
//		{
//
//			//	I16 AI_VReadChannel (U16 CardNumber, U16 Channel,U16 AdRange, F64 *voltage)
//			err = AI_VReadChannel (card, j, range, &voltage[i][j]);
//			//cout<<j<<"="<<voltage[i][j]<<endl;
//		}
//
//		Sleep(1000);
//		if (err!=0)
//		{
//			printf("AI_ContReadChannel error=%d", err);
//			exit(1);
//		}
//		// cout<<voltage[0]<<endl;
//	}
//
//
//	for(int i=0; i<10; i++)
//	{
//		for(int j=0;j<2;j++)
//		{
//			cout<<voltage[i][j]<<", ";
//		}
//		cout<<endl;
//	}
//	cin.ignore();
//	return (0);
//}
