#include "MyRobot.h"
using namespace std;

MyRobot::MyRobot(void)
{
	m_PipeHandle =0;
	m_PipeConnected = false;
	m_PipeName="\\\\.\\pipe\\PipeTest";


	/*
	//	= { 2,3,3};
	XyzWpr ps0 = {729.377,-183.458, -110.735,6.329462,-89.98957,173.656677,0};
	XyzWpr ps1 = {729.377,-173.458, -110.735,6.329462,-89.98957,173.656677,1};
	XyzWpr ps2 = {729.377,-186.458, -112.735,6.329462,-89.98957,173.656677,2};
	XyzWpr ps3 = {729.377,-186.458, -110.735,7.329462,-89.98957,171.656677,3};

	/*	for(int i=0;i<POSE_NUM;i++)
	{
	XyzWpr xx  = { 100 + i*10, 200 - i*10 , -300 - i*3, 0 + i*7, 0 + i*10, 0 +  pow(-1.0f,(i+1))*5, i};
	m_Pose[i]	=xx;
	}
	*/

	/*  m_Pose[0]	= ps0;	
	m_Pose[1]	= ps1;	
	m_Pose[2]	= ps2;	
	m_Pose[3]	= ps3;*/
}

MyRobot::~MyRobot(void)
{
	if(m_PipeHandle!=0)
	{
		DisconnectPipe();
	}
}

void MyRobot::DisconnectPipe()
{
	FlushFileBuffers(m_PipeHandle); 
	DisconnectNamedPipe(m_PipeHandle); 
	m_PipeHandle =0;
}

bool MyRobot::StartPipeServer()
{
	//  string m_PipeName;
	//string outfile;
	HANDLE hPipeHandle = CreateNamedPipe(m_PipeName.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE| PIPE_WAIT,1,MAX_PATH,MAX_PATH,PROVIDER_WAIT_TIME,NULL);

	if(INVALID_HANDLE_VALUE == hPipeHandle)
	{
		cout<<"Can't create pipe "<<m_PipeName<<"!\n";
		return false;
	}
	else
	{
		//fConnected = ConnectNamedPipe(pPipeDlg->m_PipeHandle, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
		m_PipeHandle = hPipeHandle;
		cout<<"Pipe "<<m_PipeName<<" has been created!\n";

		return true;
	}

}
bool MyRobot::ListenPipeConnect()
{
	if(m_PipeHandle!=0)
	{
		cout<<"Listening to robot client...\n";
		m_PipeConnected = ConnectNamedPipe(m_PipeHandle, NULL);

		if(m_PipeConnected)
			cout<<"One robot client connected!\n";
	}
	else
	{
		cout<< "Failed!" << endl;
		m_PipeConnected=false;
	}

	return m_PipeConnected;

}

int MyRobot::GoPose(int Pid, float * realpose)
{
	if(0 == m_PipeHandle)
	{
		cout <<"Pipe handle does not exist!";
		return -1;
	}

	if(!m_PipeConnected)
	{
		m_PipeConnected = ConnectNamedPipe(m_PipeHandle, NULL); 
	}

	if(m_PipeConnected) 
	{ 

		cout<<"Pipe is connected\n";
		DWORD dwWrite;

		cout << "Can I write to pipe? ";
		bool bwt= WriteFile(m_PipeHandle,(LPCVOID)&(m_Pose[Pid]), sizeof(XyzWpr), &dwWrite, NULL);

		if (bwt)
		{ 
			cout << "YES!" << endl;
		}
		else
		{
			cout << "NO!" << endl;
		}

		if(bwt)
		{
			cout<<"Write pipe "<<dwWrite<<" bytes!\n";
		}
		else
		{
			cout<<"Can't write pipe!\n";
		}

		bool bRobotFinish = false;
		XyzWpr * xBuffer = new XyzWpr;

		while(!bRobotFinish)
		{
			// unsigned char * ucBuffer = new unsigned char[sizeof(XyzWpr)];

			DWORD dwToRead = sizeof(XyzWpr);
			DWORD dwRead;

			bRobotFinish = ReadFile (m_PipeHandle, // handle to pipe 
				(LPVOID)xBuffer,//chRequest, // buffer to receive data 
				dwToRead, // size of buffer 
				&dwRead, // number of bytes read 
				NULL); // not overlapped I/O 

			if(bRobotFinish)
			{
				if(xBuffer->pid == (double)Pid)
				{
					cout << "Received robot pose response:  pid= "<< xBuffer->pid<<endl;
					realpose[0] = (float)(xBuffer->X);
					realpose[1] = (float)(xBuffer->Y);
					realpose[2] = (float)(xBuffer->Z);
					realpose[3] = (float)(xBuffer->W);
					realpose[4] = (float)(xBuffer->P);
					realpose[5] = (float)(xBuffer->R);
				}
				else
				{
					bRobotFinish= false;
					cout<<"Not receiving the expected reply from robot! Received id:"<<xBuffer->pid<<"\n";
				}
			}//if

		}//end while

		delete xBuffer;       
	}
	else
	{
		cout <<"Pipe can't be connected!";
		return -1;
	}

	return 0;
}



void MyRobot::ReadPose()
{

	m_PosePath = "InputPose\\";

	string fname;
	stringstream ss;
	ifstream file_in;

	cout<<"Reading Pose from file...\n";

	double pose[DOF];

	ss<<"RealPose1.txt";
	ss>>fname;
	// cout<<fname<<endl;
	ss.clear();
	file_in.open(fname.c_str());
	string sline;
	for(int j=0;j<DOF;j++)
	{
		getline(file_in,sline);
		pose[j] = atof(sline.c_str());

	}
	file_in.close();

	XyzWpr ps = { pose[0],pose[1], pose[2],pose[3],pose[4],pose[5],0};
	m_Pose[0] = ps;

	double *ptr = &(m_Pose[0].X);
	for(int k =0;k<DOF+1;k++)
	{
		cout<<*ptr++<<"  ";
	}
	cout<<endl;

}

void MyRobot::SetPose(const XyzWpr pose)
{
	m_Pose[0] = pose;
}