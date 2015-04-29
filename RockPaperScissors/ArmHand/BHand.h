/////////////////////////////////////////////////////////////////////
//
//					(C) Barrett Technology Inc. 1998-2001
//					
//
//	Header file for version 4.XX of the BHand C++ library.
//  "BHand.h" must be included in the user program, and "BHand.lib" 
//  must be linked to the project.  
//


/******************************************************************/
//						Revision History
//v4.01 - et 990818 added BHMODE_RETURN functionality to Library
//			This allows the user to send commands to the Library without
//			sending commands to the BarrettHand.
//
//v4.1 - BZ - 991217: Added RTDumpFeedback() function 
//
//v4.2 - BZ - July 2001: Added TorqueOpen(), TorqueClose(), PSet(), PGet()
//			RTGetTemp(), and a new RTSetFlags() to set all 11 flags 
//
//v4.3 - ET July 2001: Multiple Hands Allowed, one per comport
//			removed _BHInitialized flag
//			added BHand* _BHandArray[BH_MAXPORT] with a pointer to
//				to the hand on each comport
//			added int SelectHand(comport): used to select the
//				hand that ver 1.0 functions refer to
//			modified ver 1.0 InitSoftware() to take comport parameter
//				default = 1
//			added BHERR_PORTOUTOFRANGE error message
//			added comPort field in BHand class
//			changed unsigned char to int in ComInitialize()
/******************************************************************/


#ifndef _INCLUDE_BHAND_H_JUST_ONCE_
#define _INCLUDE_BHAND_H_JUST_ONCE_

#include <windows.h>


// callback function type
typedef	void (*BHCallback)( class BHand* );


// char buffer size, max comport
#define BH_MAXCHAR				5000 
#define BH_MAXPORT				256


// Software Library Error Messages (hand errors are positive)
#define BHERR_BHANDEXISTS		-1
#define BHERR_OPENCOMMPORT		-2
#define BHERR_GETCOMMSTATE		-3
#define BHERR_SETCOMMSTATE		-4
#define BHERR_SETCOMMTIMEOUT	-5
#define BHERR_THREADSTART		-6
#define BHERR_THREADPRIORITY	-7
#define BHERR_WRITECOM			-8
#define BHERR_READCOM			-9
#define BHERR_BADRESPONSE		-10
#define BHERR_CLEARBUFFER		-11
#define BHERR_TIMEOUT			-12
#define BHERR_NOTCOMPLETED		-13
#define BHERR_PENDING			-14
#define BHERR_NOTINITIALIZED	-15
#define BHERR_BADPARAMETER		-16
#define BHERR_LONGSTRING		-17
#define BHERR_OUTOFRANGE		-18
#define BHERR_MOTORINACTIVE		-19
#define BHERR_PORTOUTOFRANGE	-20

		
// Requests to Communications Thread
#define BHREQ_EXIT				1
#define BHREQ_REALTIME			2
#define BHREQ_SUPERVISE			3
#define BHREQ_CLEAR				4


// Synchronization Modes
#define BHMODE_SYNC				1
#define BHMODE_ASYNCNOW			2
#define BHMODE_ASYNCWAIT		3
#define BHMODE_RETURN			4



/////////////////////////////////////////////////////////////////////
// Class BHand declaration

class BHand
{
	friend DWORD _stdcall _ComThreadFunction( void* pHand);

public:
	BHand() {};
	~BHand();
	const char* ErrorMessage(int err);


	////////////////////////////////////////////////////////////////
	// Serial communications 

	int		ComInitialize( int comport, int priority );
	int		ComSetTimeouts( DWORD readInterval, DWORD readMultiplier, DWORD readConstant,
							DWORD writeMultiplier, DWORD writeConstant );
	int		ComSetBaudrate( DWORD baud );
	int		ComRequest( int requestNumber );
	bool	ComIsPending( void );
	int		ComWaitForCompletion( DWORD timeout );
	bool	ComClear( void );
	int		ComRead( char* buffer, int size );
	int		ComWrite( char* buffer, int size );

	int		syncMode;		
	DWORD	requestTimeout;	
	BHCallback pCallback;	


	////////////////////////////////////////////////////////////////
	// Interactive commands from version 1.0
	
	int		Open( char* motor );
	int		Close( char* motor );
	int		StepOpen( char* motor, int value );
	int		StepClose( char* motor, int value );
	int		GoToPosition( char* motor, int value );
	int		GoToDifferentPositions( int value1, int value2, int value3, int value4 );
	int		GoToHome( void );
	int		StopMotor( char* motor );
	int		Set( char* motor, char* parameter, int value );
	int		Get( char* motor, char* parameter, int* result );
	int		Save( char* motor );
	int		Load( char* motor );
	int		Default( char* motor );
	int		Temperature( int* result);
	int		InitSoftware( int port, int priority = THREAD_PRIORITY_TIME_CRITICAL );
	int		InitHand( char* motor );


	////////////////////////////////////////////////////////////////
	// New interactive commands

	int		Delay( DWORD msec );
	int		GoToDefault( char* motor );
	int		Reset( void );
	int		Baud( DWORD baud );
	int		Command( char* send, char* receive=NULL );
	const char* Response( void ); 
	const char* Buffer( void ); 
	int		TorqueOpen( char* motor );
	int		TorqueClose( char* motor );
	int		PSet( char* parameter, int value );
	int		PGet( char* parameter, int* result);

	
	////////////////////////////////////////////////////////////////
	// Real-time (loop) mode commands

	int		RTStart( char* motor );
	int		RTUpdate( bool control=true, bool feedback=true );
	int		RTSetFlags( char* motor, bool LCV, int LCVC, bool LCPG, 
						bool LFV, int LFVC, bool LFS, bool LFAP, bool LFDP, int LFDPC );
	int		RTSetFlags( char* motor, bool LCV, int LCVC, bool LCPG, 
					   bool LFV, int LFVC, bool LFS, bool LFAP, bool LFDP, int LFDPC,
					   bool LFT, bool LFDPD);
	int		RTAbort( void );
	char	RTGetVelocity( char motor );
	unsigned char RTGetStrain( char motor );
	int		RTGetPosition( char motor );
	char	RTGetDeltaPos( char motor );
	int		RTGetTemp( void );
	int		RTSetVelocity( char motor, int velocity );
	int		RTSetGain( char motor, int gain );

	void	RTDumpFeedback(void);

	int		rtFlags[4][7];
	int		rtGlobalFlags[1];
	int		nSend;
	int		nReceive;	
	char	rtIn[22];


	////////////////////////////////////////////////////////////////
	// Internal variables - not accessible by the user program

	HANDLE	com;				
private:
	HANDLE	thread;				
	DWORD	threadId;
	int		request;			
	HANDLE	requestPending;		
	HANDLE	requestComplete;	
	DWORD	requestBaud;		
	char	inbuf[BH_MAXCHAR];	
	int		nin;				
	char	outbuf[BH_MAXCHAR]; 
	int		nout;				
	int		comErr;				
	char	rtOut[8];
	int		rtControl[4][2];
	int		rtFeedback[4][4];
	int		rtGlobalFeedback[1];
	int		comPort;
};


// pointer to hand corresponding to each port-1 (1 : BH_MAXPORT)
extern BHand* _BHandArray[];


// Obsolete fuctions provided for version 1.0 compatibility
int	Open( char motor );
int	Close( char motor );
int	StepOpen( char motor, int value );
int	StepClose( char motor, int value );
int	GoToPosition( char motor, int value );
int	GoToDifferentPositions( int value1, int value2, int value3, int value4 );
int	GoToHome( void );
int	StopMotor( char motor );
int	Set( char motor, char parameter, int value );
int	Get( char motor, char parameter, int* result );
int	Save( char motor );
int	Load( char motor );
int	Default( char motor );
int	Temperature( int* result );
int	InitSoftware( int comport = 1 );
int	InitHand( void );
int	SelectHand( int comport );


// pointer to selected hand for vers 1.0 compatibility
extern BHand* _pBH;


#endif
