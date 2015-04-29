
//for grasping

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <math.h>
#include <conio.h>
#include "BHand.h"

int max_strain = 130;

BHand  bh;           // Handles all hand communication
int    value;        // Hand parameter obtained with Get
int    result;       // Return value (error) of all BHand calls

int    szbuffer;     // Size of data buffers
int*   pdata[4][8];  // Pointers to data buffers
int    setpos(char finger, int despos,int velocity,int gain);
int	   N;			 //Save data count
//int		stop(void);

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
//  Execute commands, return 1 if interrupted with a key

int Before(void)
{
	printf( "Press Any Key to Abort..." );

	if( result=bh.Command("sc") )
		Error();
	if( _kbhit() )
		{ _getch(); return 1; }

	if( result=bh.Command("go") )
		Error();
	if( _kbhit() )
		{ _getch(); return 1; }

	return 0;
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


int SetPos(int despos1, int despos2, int despos3, int despos4, int v1, int v2, int v3, int v4)
{int flag=1,curpos[4],despos[4],v[4],pos;

	despos[0]=despos1;
	despos[1]=despos2;
	despos[2]=despos3;
	despos[3]=despos4;
	v[0]=v1;
	v[1]=v2;
	v[2]=v3;
	v[3]=v4;

	while(flag&&!_kbhit())
	{ flag=0;

	pos = N%szbuffer;
	printf("N= %d\n", N);
		for( int m=0; m<3; m++ )
		{
			pdata[m][0][pos] = bh.RTGetPosition( m+'1' );
			pdata[m][1][pos] = bh.RTGetVelocity( m+'1' );
			pdata[m][2][pos] = bh.RTGetStrain( m+'1' );

			printf("finger %d position: %d\n", m, pdata[m][0][pos]);
			printf("finger %d velocity: %d\n", m, pdata[m][1][pos]);
			printf("finger %d strain: %d\n\n", m, pdata[m][2][pos]);

			curpos[m] = bh.RTGetPosition('1'+m);
			//printf("position %d: %d\n",m+1, curpos[m]);
			if(abs(despos[m]-curpos[m])>400 && pdata[m][2][pos] < max_strain)	//(abs(v[m])+10)
			//if(abs(despos[m]-curpos[m])>400 )	//(abs(v[m])+10)
			{
				flag=1;
				if(despos[m]<curpos[m]) v[m]=-abs(v[m]);
				else v[m]=abs(v[m]);
				bh.RTSetVelocity(m+'1',v[m]);
				
			}
			else
				bh.RTSetVelocity(m+'1',0);
		}
		pos = (N+1)%szbuffer;
		bh.RTUpdate();
		N++;
	}

	flag=1;

	/*while(flag&&!_kbhit())
	{flag=0;
		for( int m=0; m<4; m++ )
		{
			if(bh.RTGetVelocity( m+'1' ))
			{
						bh.RTSetVelocity(m+'1',0);
						bh.RTUpdate();
						flag=1;
			}
		}
	}*/
	return 0;
}

int PlayPiano(void)
{
	int spread=1000;
	int posup=6000;
	int posdown=10000;
	int speed=50;
	int lowspeed=25;
	int inc=20;
	int shortime=500;
	int longtime=1000;

//***********finger 1,2,3**********
	SetPos(posdown, posdown, posdown,spread, speed, speed, speed, 20);
	//SetPos(posup, posup, posup,spread, speed, speed, speed, 20);
	Sleep(longtime);


	return 0;
}

///////////////////////////////////////////////////////////
//  Run RealRime loop, return 1 if interrupted with a key

int RunRealTime(void)
{
	int spread=1000;
	N=0;
	//DWORD  time, tmstart;
	bool terminate=false;
	bh.GoToPosition("4", spread);

	bh.RTStart( "123" );
	//tmstart = GetTickCount();
	
	int gain=50;
	bh.RTSetGain('1',gain);
	bh.RTSetGain('2',gain);
	bh.RTSetGain('3',gain);
	//bh.RTSetGain('4',gain);
	bh.RTUpdate();

	printf( "Press Any Key to Abort..." );
	
	int posup=8000;
	int posdown=8000;
	int speed=50;
	int lowspeed=25;
	int inc=20;
	int shortime=200;
	int longtime=1000;

	Sleep(300);
	SetPos(posup, posup, posup-1000,spread, lowspeed, 30, lowspeed, 20);
//SetPos(posup, posup, posup,spread, lowspeed, 30, lowspeed, 20);
	posup=10000;
	posdown=12000;
	speed = 25;
	SetPos(posdown, posdown, posdown-1000, spread, speed, speed, speed, 20);
/*
	printf("Please press <ENTER> to continue.");
	while(getchar() != '\n');
	
	PlayPiano();

	spread=1000;
	bh.GoToDifferentPositions(posup, posup, posup,spread);
	bh.RTStart( "123" );

	
	//Sleep(300);
	SetPos(posup, posup, posup,spread, lowspeed, 30, lowspeed, 20);

	PlayPiano();
*/

	bh.RTAbort();
	if( _kbhit() )
		{ _getch(); return 1; }
	else
		return 0;
}

///////////////////////////////////////////////////////////
//  Execute commands, return 1 if interrupted with a key

int After(void)
{
	printf( "Press Any Key to Abort..." );

	while(1){
	if( _kbhit() )
		{ _getch();
	if( result=bh.Command("t") )
		Error();
	return 1; }
	}
	return 0;
}


///////////////////////////////////////////////////////////
//  Save all buffers into a text file

void SaveData(char* name)
{
	FILE* fp = fopen(name, "wt");
	if( !fp )
		return;
	if(N > 1000) N = 1000;
	fprintf(fp, "%d ", N);
	fprintf(fp, "\n");

	for( int r=0; r<szbuffer; r++ )
	{
		for( int v=0; v<8; v++ )
		{
			for( int m=0; m<4; m++ )
				fprintf(fp, "%d ", pdata[m][v][r]);
			fprintf(fp, "  ");
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
}


///////////////////////////////////////////////////////////
//  Main function - initialize, execute

void main(void)
{
	printf( "Initialization..." );

	Initialize(8);
	printf( " Done\n" );

	printf( "Before - " );
	if( Before() )
		return;
	printf( " Done\n" );

	PrepareRealTime();

	printf( "RealTime Loop - " );
	if( RunRealTime() )
		//return;
	printf( " Done\n" );

	SaveData( "RealTime" );

	printf( "After - " );
	
	if( After() )
		return;
	printf( " Done\n" );
	
	//system("pause");
}
