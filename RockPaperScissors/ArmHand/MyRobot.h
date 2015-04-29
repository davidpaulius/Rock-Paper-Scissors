#pragma once
#include <string>
#include <windows.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

#define PROVIDER_WAIT_TIME	3000
#define POSE_NUM  1
#define DOF 6

typedef struct XyzWpr_t 
{
double X;
double Y;
double Z;
double W;
double P;
double R;
double pid;   // start from 0
} XyzWpr;

class MyRobot
{
private:

HANDLE m_PipeHandle;
string m_PipeName;
bool   m_PipeConnected;
XyzWpr m_Pose[POSE_NUM];
string m_PosePath;

public:
	MyRobot(void);
	~MyRobot(void);

    bool StartPipeServer();
	bool ListenPipeConnect();
	void DisconnectPipe();
	void ReadPose();
	void SetPose(const XyzWpr pose);
	int GoPose(int i,float * realpose);
};
