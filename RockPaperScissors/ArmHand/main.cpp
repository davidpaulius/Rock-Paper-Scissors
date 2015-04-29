#include "Include.h"

int max_strain = 130;

BHand  bh;           // Handles all hand communication
int    value;        // Hand parameter obtained with Get
int    result;       // Return value (error) of all BHand calls

int    szbuffer;     // Size of data buffers
int*   pdata[4][8];  // Pointers to data buffers
int	   N;			 //Save data count

vector<int> counterMoves;	//Global variable for main() to use

vector<int> InputFile();	//Reads in moves and returns countermoves

int main(int argc, char* argv[])
{
	srand(time(NULL));
	int pose_num = POSE_NUM;
	int pid = 0;
	//int num_corners = (CHECKER_COLS-1)*(CHECKER_ROWS-1);
	printf( "Initialization..." );
	Initialize(8);
	printf( " Done\n" );
	
	PrepareRealTime();

	float ** pose_mem;
	pose_mem = new float* [pose_num];

	MyRobot mRobot;

	mRobot.StartPipeServer();
	mRobot.ListenPipeConnect();

	float * current_pose = new float[DOF];

	//Read off values from file
	counterMoves = InputFile();

	while(!counterMoves.empty())
	{
		// Arm Motion		
		for(int i=0; i<3; i++)
		{
			// ROCK Formation
			if (result = bh.Open( "S" ))
				Error();
			if (result = bh.GoToPosition( "123" , 14000 ))
				Error();
			
			//-----------------move robot------------------------

			cout << "Moving arm up..." << endl;

			// Make the arm go up (J1: 10)
			//XyzWpr ps = { 562.3476, 151, -114.453, 158.4313, 5.408640, 81, 0};
			//cout << "Setting Pose..." << endl;
			//mRobot.SetPose(ps);
			//cout << "Going to Pose..." << endl;
			//mRobot.GoPose(pid, current_pose);
			//Sleep(1000);

			//cout << "Moving arm down..." << endl;

			//// Make the arm go down (J1:-15)
			//XyzWpr ps1 = { 566.3476, -151, -116.453, 160.4313, 6.408640, 82, 0};
			//cout << "Setting Pose..." << endl;
			//mRobot.SetPose(ps1);
			//cout << "Going to Pose..." << endl;
			//mRobot.GoPose(pid, current_pose);
			//Sleep(1000);

			//XyzWpr ps5 = { 562.3476, 151, -114.453, 158.4313, 5.408640, 81, 0};
			//mRobot.SetPose(ps5);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);

			//// Make the arm go down (J1:-15)
			//XyzWpr ps6 = { 566.3476, -151, -116.453, 160.4313, 6.408640, 82, 0};
			//mRobot.SetPose(ps6);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);
			//
			///*
			////make the robot go up once more before making move
			//XyzWpr ps7 = { 562.3476, 151, -114.453, 158.4313, 5.408640, 81, 0};
			//mRobot.SetPose(ps7);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);

			////go back down and then make move
			//XyzWpr ps10 = { 566.3476, -151, -116.453, 160.4313, 6.408640, 82, 0};
			//mRobot.SetPose(ps10);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);
			//*/ 

			//// Make the arm go down (J1:-15)
			//XyzWpr ps8 = { 566.3476, 0, -116.453, 160.4313, 6.408640, 82, 0};
			//mRobot.SetPose(ps8);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);

			//XyzWpr ps3 = { 570.3476, 0, -118.453, 168.4313, 7.408640, 84, 0};
			//mRobot.SetPose(ps3);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);

			//XyzWpr ps4 = { 570.3476, 0, -118.453, 168.4313, 7.408640, 84, 0};
			//mRobot.SetPose(ps4);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(50);

			//XyzWpr ps9 = { 570.3476, 0, -118.453, 168.4313, 7.408640, 84, 0};
			//mRobot.SetPose(ps9);
			//mRobot.GoPose(pid, current_pose);
			//Sleep(3000);
			//getchar();
		}

		//----------------------move robot----------------------------
		//XyzWpr ps3 = { 570.3476, 0, -118.453, 168.4313, 7.408640, 84, 0};
		//mRobot.SetPose(ps3);
		//mRobot.GoPose(pid, current_pose);
		Execute();		//Hand Motion
		Sleep(3000);
	}

	// ROCK Formation
	/*if (result = bh.GoToPosition( "123" , 14000 ))
		Error();*/

	mRobot.DisconnectPipe();

	cout<<"======== End of Program! Press Enter after disconnecting pipe. ================"<<endl;

	getchar();

	delete [] pose_mem;
	delete [] current_pose;

	return 0;
}

// Read from file
vector<int> InputFile()
{
	int play;
	ifstream input("C:/Users/david/Documents/Visual Studio 2008/Projects/RockPaperScissors/rsp_in.txt");
	string line;

	while (getline(input, line))
	{
		istringstream s(line);

		while(s.good())
		{
			s >> play;

			counterMoves.push_back(play);
			cout << "LALALA:" << play << endl;
			/*
			if(play == 3)					//enemy plays scissor
				counterMoves.push_back(1);	//we play rock
			else if(play == 2)				//enemy plays paper
				counterMoves.push_back(3);	//we play scissors
			else							//enemy plays rock
				counterMoves.push_back(2);	//we play paper
				*/
		}
	}

	input.close();

	return counterMoves;
}


///////////////////////////////////////////////////////////
// ROCK PAPER SCISSORS
int Execute()
{
	//Sleep(300);
	if (result = bh.GoToHome( ))
		Error();
	
	// ADDITION: AI Integration from file
	int random = counterMoves.back();
	counterMoves.pop_back();

	//int random = rand() % 3 + 1;
	if (random == 3){
		//if (result = bh.Open( "S" ))
			//Error();
		// Make Rock formation
		if (result = bh.GoToPosition( "123" , 14000 ))
			Error();
	}
	else if (random == 2){
		// Make scissors formation
		if (result = bh.GoToPosition( "S" , 300 ))
			Error();

		if (result = bh.Close( "3" ))
			Error();

		if (result = bh.GoToPosition( "12" , 5000 ))
			Error();
	}
	else{
		// Make Paper Formation
		if (result = bh.Close( "S" ))
			Error();

		if (result = bh.GoToPosition( "123" , 9000 ))
			Error();
	}
	
	return 0;
}

//****************************************Start of Hand Code********************************************************

///////////////////////////////////////////////////////////
// Error Handler - called whenever result!=0
// Feel free to customize this function
void Error(void)
{
	printf( "ERROR: %d\n%s\n", result, bh.ErrorMessage(result) );
	exit(0);
}

///////////////////////////////////////////////////////////
//  Initialize hand, set timeouts and baud rate
void Initialize(int port)
{
	if( result=bh.InitSoftware(port,THREAD_PRIORITY_TIME_CRITICAL) )
		Error();

	if( result=bh.ComSetTimeouts(0,100,15000,100,5000) )
		Error();

	if( result=bh.Baud(9600) )
		Error();

	if( result=bh.InitHand("") )
		Error();
}

///////////////////////////////////////////////////////////
//  Set parameters, allocate data buffers, load files
void PrepareRealTime(void)
{
	szbuffer = 1000;

	for( int m=0; m<4; m++ )
		for( int v=0; v<8; v++ )
		{
			pdata[m][v] = new int[szbuffer];
			memset((void*)pdata[m][v], 0, szbuffer*sizeof(int));
		}

		if( result=bh.RTSetFlags( "123", 1, 1, 1,  1, 1, 1, 1, 0, 1 ))
			Error();

}

//***************************************************************End of hand code*******************************************