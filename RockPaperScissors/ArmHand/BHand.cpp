/////////////////////////////////////////////////////////////////////
//
//					(C) Barrett Technology Inc. 1998
//					Written Emanuel Todorov, 12/1998
//
//	Implementation for version 4.0 of the BHand C++ library.
//  "BHand.h" must be included in the user program, and "BHand.lib" 
//  must be linked to the project.  
//

/******************************************************************/
//						Revision History
//v4.01 - et 990818 added BHMODE_RETURN functionality to Library
//			This allows the user to send commands to the Library without
//			sending commands to the BarrettHand.
//
//v4.1 - BZ - 991217:
//				-Changed the ComRead and memcpy function parameters to include
//					the last byte of information sent from the BarrettHand. 
//				-In RTStart() corrected code to read data stream correctly
//				-Corrected RTUpdate to receive correct number of bytes
//				-Corrected RTGetPosition() to return correct position (low
//					byte subtraction problem)
//				-Corrected RTSetGain() to set the gain properly
//
//v4.2 - BZ - July 2001: Added TorqueOpen(), TorqueClose(), PSet(), PGet()
//			RTGetTemp(), and a new RTSetFlags() to set all 11 flags 
//
//v4.3 - ET July 2001
//				added _clearFlag
//				modifed InitSoftware to clear hand array on first call
//				see list of changes in BHand.h
/******************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <ctype.h>
#include <iostream>
#include "BHand.h"

using namespace std;

// global variables defined in BHand.h as extern
BHand* _pBH = NULL;
BHand* _BHandArray[BH_MAXPORT];


// flag used to clear _BHandArray in the first call to InitSoftware
bool _clearFlag = true;



/////////////////////////////////////////////////////////////////////
// Decoding of error messages


// BHand error messages - text description
const static char _ErrorMessage[22][100] = {
	"No error",
	"An instance of class BHand has already been initialized",
	"Open comm port failed",
	"Get comm port state failed",
	"Set comm port state failed",
	"Set comm timeouts failed",
	"Unable to start hand i/o thread",
	"Unable to set hand i/o thread priority",
	"Error writing to comm port (or write timeout)",
	"Error reading from comm port (or read timeout)",
	"Hand sent incorrect sequence of characters - possible loss of data",
	"Clear comm buffer failed",
	"Request to hand i/o thread timed out",
	"Processing of previous request is not completed",
	"Request pending - in asynch mode",
	"No instances of class BHand have been initialized",
	"Unrecognized parameter name in call to Set",
	"String too long",
	"Parameter value out of range (0-30000)",
	"Specified motor was not activated in RTStart",
	"Hand error - check hand manual",
	"Invalid error number"
};


// return error string corresponding to err
const char* BHand::ErrorMessage( int err )
{
	if( err>0 )
		return _ErrorMessage[20];
	else if( err>-20 )
		return _ErrorMessage[-err];
	else
		return _ErrorMessage[21];
}



/////////////////////////////////////////////////////////////////////
// Serial communications implementation


// Close communication port, stop thread
BHand::~BHand()
{
	// ask thread to stop, wait for 1 sec.
	syncMode = BHMODE_ASYNCWAIT;
	ComRequest( BHREQ_EXIT );
	WaitForSingleObject( thread, 1000 );

	// close com port, mark uninitialized
	if( com )
		CloseHandle( com );

	// remove hand from array
	if( comPort>=0 && comPort<BH_MAXPORT )
		_BHandArray[comPort] = NULL;
}


// Request service from the thread
int BHand::ComRequest( int requestNumber )
{
	int result;

	// check for pending request
	if( ComIsPending() )
	{
		// immediate async - return error
		if( syncMode==BHMODE_ASYNCNOW )
			return BHERR_NOTCOMPLETED;

		// sync or wait async - just wait
		else
			if(	(result=ComWaitForCompletion( requestTimeout )) )
				return result;		
	}

	// RETURN - return immediately
	if( syncMode==BHMODE_RETURN )
	{
		// call callback anyway
		comErr = 0;
		if( pCallback )
			(*pCallback)( this );

		return 0;
	}

	// send request
	request = requestNumber;
	SetEvent( requestPending );

	// wait for completion in sync mode

	if( syncMode==BHMODE_SYNC )
	{
		if( result=ComWaitForCompletion( requestTimeout ) )
			return result;
		else
			return comErr;
	}
	else
		return BHERR_PENDING;
}


// Check for pending request
bool BHand::ComIsPending( void )
{
	// return immediately
	return( WaitForSingleObject( requestPending, 0 ) == WAIT_OBJECT_0 );
}


// Clear input and output buffers
bool BHand::ComClear( void )
{
	return( PurgeComm( com, PURGE_RXCLEAR | PURGE_TXCLEAR )!=0 );
}


// Read characters, check for errors
int BHand::ComRead( char* buffer, int size )
{
	unsigned long n;

	// read size characters
	if( !ReadFile( com, buffer, size, &n, NULL ) )
		return BHERR_READCOM;

	// make sure char number is correct
	if( size != (int)n )
		return BHERR_READCOM;
	else
		return 0;
}


// Write characters, check for errors
int BHand::ComWrite( char* buffer, int size )
{
	unsigned long n;

	// read size characters
	if( !WriteFile( com, buffer, size, &n, NULL ) )
		return BHERR_WRITECOM;

	// make sure char number is correct
	if( size != (int)n )
		return BHERR_WRITECOM;
	else
		return 0;
}


// Wait for thread to complete processing
int BHand::ComWaitForCompletion( DWORD timeout )
{
	// only if request pending
	if( !ComIsPending() )
		return 0;

	if( WaitForSingleObject( requestComplete, timeout ) == WAIT_OBJECT_0 )
		return 0;
	else
		return BHERR_TIMEOUT;
}


// Set com port timeout parameters
int BHand::ComSetTimeouts( DWORD readInterval, DWORD readMultiplier, DWORD readConstant, 
			  		       DWORD writeMultiplier, DWORD writeConstant)
{
	COMMTIMEOUTS tmout;
	tmout.ReadIntervalTimeout = readInterval;
	tmout.ReadTotalTimeoutMultiplier = readMultiplier;
	tmout.ReadTotalTimeoutConstant = readConstant;
	tmout.WriteTotalTimeoutMultiplier = writeMultiplier;
	tmout.WriteTotalTimeoutConstant = writeConstant;

	// attempt to set timeouts
	if( !SetCommTimeouts( com, &tmout ) ) 
		return BHERR_SETCOMMTIMEOUT;
	else
		return 0;
}


// Set com port baud rate and default parameters
int BHand::ComSetBaudrate( DWORD baud )
{
	DCB dcb;
	if( !GetCommState(com, &dcb) ) 
		return BHERR_GETCOMMSTATE;
	dcb.BaudRate = baud;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	// attempt to set device control block
	if( !SetCommState(com, &dcb) ) 
		return BHERR_SETCOMMSTATE;
	else
		return 0;
}


// Initialize communications, return 0 if everything is OK
int BHand::ComInitialize( int comport, int priority ) 
{
	int result;

	// range checking, save comport-1
	if( comport<1 || comport>BH_MAXPORT )
		return BHERR_PORTOUTOFRANGE;

	cout << "HHH" << endl;

	comPort = comport-1;

	// check for previous instance of BHand attached to this port
	if( _BHandArray[comPort] )
		return BHERR_BHANDEXISTS;

	cout << "HHH1" << endl;
	// save instance in port array
	_BHandArray[comPort] = this;

	// make port name
	char portname[] = "COM";
	portname[3] += (unsigned char) comport;

	 //open com port - synch mode 
	//com = CreateFile( LPCTSTR(portname), GENERIC_READ | GENERIC_WRITE, 0,
	com = CreateFile( TEXT("COM8"), 
		GENERIC_READ | GENERIC_WRITE, 0,
   					  NULL, OPEN_EXISTING, 0, NULL );
	if( com == INVALID_HANDLE_VALUE ) 
		return BHERR_OPENCOMMPORT;
	// configure port with default parameters
	if( (result=ComSetBaudrate( 9600 )) > 0 )
		return result;

	// set default timeouts
	if( (result=ComSetTimeouts( 0, 100, 15000, 100, 5000 )) > 0 )
		return result;

	// clear buffers
	if( !ComClear() )
		return BHERR_CLEARBUFFER;

	// create synchronization events
	request = 0;
	requestPending = CreateEvent( NULL, true, false, NULL );
	requestComplete = CreateEvent( NULL, false, false, NULL );

	// start separate thread, set priority
	thread = CreateThread( NULL, 0, _ComThreadFunction, this, 0, &threadId );
	if( !thread )
		return BHERR_THREADSTART;
	if( !SetThreadPriority( thread, priority ) )
		return BHERR_THREADPRIORITY;

	// initialization succeeded
	return 0;
}



/////////////////////////////////////////////////////////////////////
// This is the separate thread that handles all serial communications.
// All other functions place requests to it and wait for a response.
// The thread runs with high priority to minimize delays.


// end thread processing, invoke callback function
#define THREADEND(err)								\
{													\
	h->comErr = (err);								\
	ResetEvent( h->requestPending );				\
	PulseEvent( h->requestComplete );				\
	if( h->pCallback )								\
		(*h->pCallback)( h );						\
	break;											\
} 

// thread function - suspended while waiting for serial I/O
DWORD _stdcall _ComThreadFunction(void* pHand) 
{
	char dummy[BH_MAXCHAR];
	int result;

	// pointer to the ONLY hand object
	BHand* h = (BHand*)pHand;

	// loop forever - user must ask thread to exit
	while( true ) 
	{
		// wait for request
		WaitForSingleObject( h->requestPending, INFINITE );
		h->comErr = 0;

		// handle all requests
		switch( h->request )
		{
		case BHREQ_EXIT:	// exit thread

			// reset events
			ResetEvent( h->requestPending );
			PulseEvent( h->requestComplete );
			return 0;


		case BHREQ_REALTIME:	// loop mode command

			// clear buffers - eliminate echo chars
			if( !h->ComClear() )
				THREADEND( BHERR_CLEARBUFFER )

			// send data
			if( h->ComWrite( h->outbuf, h->nout ) )
				THREADEND( BHERR_WRITECOM )

			// check for * in first position
			if( h->ComRead( h->inbuf, 1 ) )
				THREADEND( BHERR_READCOM )

			// '*' received - proceed with remaining characters
			if( h->inbuf[0]=='*' )
			{
				if( h->nin>1 )
				{
					if( h->ComRead( h->inbuf, h->nin ) ) //BCZ 11/15/99, WAS: if( h->ComRead( h->inbuf, h->nin-1 ) )
						THREADEND( BHERR_READCOM )

					memcpy( h->rtIn, h->inbuf, h->nin ); //BCZ 10/28/99, WAS: memcpy( h->rtIn, h->inbuf, h->nin-1 );
				}

				THREADEND(0)
			}

			// something else received - proceed with interactive command
			else
			{
				h->nin = 1;
				h->inbuf[1] = 0;
				goto _INTERACTIVE;
			}


		case BHREQ_SUPERVISE:		// interactive mode command - return => 

			// check for delay
			sscanf( h->outbuf, "%s", dummy );
			if( !strcmp( dummy, "DELAY" ) )
			{
				// read delay value
				DWORD msec;
				sscanf( h->outbuf+6, "%ld", &msec );
				if( msec<1 )
					msec = 1;
				else if( msec>100000 )
					msec = 100000;
				Sleep( msec );
				h->inbuf[0] = 0;

				THREADEND(0)
			}


			// clear buffers - eliminate echo chars
			if( !h->ComClear() )
				THREADEND( BHERR_CLEARBUFFER )

			// send data
			if( h->ComWrite( h->outbuf, h->nout ) )
				THREADEND( BHERR_WRITECOM )

			// clear echo chracters - except the last one (CR or *)
			if( h->nout>1 )
			{
				if( h->ComRead( dummy, h->nout-1 ) )
					THREADEND( BHERR_READCOM )
			}

			// prepare to receive response and wait for correct ending
			h->nin = 0; 
			h->inbuf[0] = 0;

			// set new baud rate if necessary
			if( h->requestBaud )
			{
				if( (result=h->ComSetBaudrate( h->requestBaud )) > 0 )
					THREADEND( result );
								
				if( !strncmp(h->outbuf,"PSET BAUD",9) )
				{
					Sleep( 1000 );
					if( !h->ComClear() )
						THREADEND( BHERR_CLEARBUFFER )
					if( h->ComWrite( "\r", 1 ) )
						THREADEND( BHERR_WRITECOM )
				}

				h->requestBaud = 0;
			}

			// terminate with * in LOOP command
			if( !strcmp( "LOOP\r", h->outbuf+max(0,h->nout-5) ) )
			{
				if( h->ComRead( h->inbuf, 1 ) )
					THREADEND( BHERR_READCOM )

				if( h->inbuf[0]!='*' )
				{
					h->inbuf[0] = 0;
					h->nin = 0;
				}
				else
					THREADEND(0)
			}

_INTERACTIVE:
			// read one character
			if( h->ComRead( h->inbuf+h->nin, 1 ) )
				THREADEND( BHERR_READCOM )

			// remove leading white space
			if( h->nin==0 && isspace(h->inbuf[0]) )
				h->nin--;

			// terminate string
			h->inbuf[++(h->nin)] = 0;

			// ending not yet correct - read more chars
			if( h->nin<BH_MAXCHAR-1 && 
				strcmp( "=> ", (const char*)(h->inbuf+max(h->nin-3,0)) ) )
				goto _INTERACTIVE;

			// check for buffer overflow
			if( h->nin>=BH_MAXCHAR-1 )
				THREADEND( BHERR_BADRESPONSE )

			// remove end marker and trailing white space
			h->nin -= 3;
			h->inbuf[h->nin] = 0;
			while( h->nin>0 && isspace( h->inbuf[h->nin-1] ) )
				h->inbuf[--h->nin] = 0;

			// check for ERR X - return as positive
			if( !strncmp(h->inbuf,"ERR",3) )
				sscanf( h->inbuf+4, "%d", &result );
			else 
				result = 0;

			THREADEND(result)

			
		case BHREQ_CLEAR:	// clear com port buffers

			// clear
			if( !h->ComClear() )
				THREADEND( BHERR_CLEARBUFFER )

			THREADEND(0)
		}
	}
}



///////////////////////////////////////////////////////////////////////////////
// Interactive commands from version 1.0 implementation


// Open specified (trapeziod control) fingers to their limits
int	BHand::Open( char* motor )
{
	// prepare output char buffer
	sprintf( outbuf, "%sO\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Close specified (trapeziod control) fingers to their limits
int	BHand::Close( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sC\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Torque Open (velocity control) specified fingers to their limits
int	BHand::TorqueOpen( char* motor )
{
	// prepare output char buffer
	sprintf( outbuf, "%sTO\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Torque Close (velocity control) specified fingers to their limits
int	BHand::TorqueClose( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sTC\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Open specified fingers by specified amount
int	BHand::StepOpen( char* motor , int value )
{
	// range check
	if( value<0 || value>30000 )
		return BHERR_OUTOFRANGE;

	// prepare output char buffer
	sprintf( outbuf, "%sIO %d\r", motor, value );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Close specified fingers by specified amount
int	BHand::StepClose( char* motor , int value )
{
	// range check
	if( value<0 || value>30000 )
		return BHERR_OUTOFRANGE;

	// prepare output char buffer
	sprintf( outbuf, "%sIC %d\r", motor, value );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Move specified fingers to position
int	BHand::GoToPosition( char* motor , int value )
{
	// range check
	if( value<0 || value>30000 )
		return BHERR_OUTOFRANGE;

	// prepare output char buffer
	sprintf( outbuf, "%sM %d\r", motor, value );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Call GoToPosition for each motor
int	BHand::GoToDifferentPositions( int value1, int value2, int value3, int value4 )
{
	// range check
	if( value1<0 || value1>30000 ||
		value2<0 || value2>30000 ||
		value3<0 || value3>30000 ||
		value4<0 || value4>30000 )
		return BHERR_OUTOFRANGE;

	// set all default positions
	int result;
	if( result = Set( "1", "DP", value1 ) )
		return result;
	if( result = Set( "2", "DP", value2 ) )
		return result;
	if( result = Set( "3", "DP", value3 ) )
		return result;
	if( result = Set( "4", "DP", value4 ) )
		return result;

	return GoToDefault( "GS" );
}


// Move all fingers to home position
int	BHand::GoToHome( void )
{
	// prepare output char buffer
	sprintf( outbuf, "GSHOME\r" );
	nout = strlen( outbuf );

	// ask thread to send command
	int result = ComRequest( BHREQ_SUPERVISE );
	if( result )
		return result;

	// prepare output char buffer
	sprintf( outbuf, "SHOME\r" );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Stop specified fingers
int	BHand::StopMotor( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sT\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Set parameter for specified motors;
// refer to documentation for names - different from version 1.0 !
int	BHand::Set( char* motor, char* parameter, int value )
{
	// prepare output char buffer
	sprintf( outbuf, "%sFSET %s %d\r", motor, parameter, value );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Set global parameter;
// refer to documentation for names - different from version 1.0 !
int	BHand::PSet( char* parameter, int value )
{
	// prepare output char buffer
	sprintf( outbuf, "PSET %s %d\r", parameter, value );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Get parameter for specified motors;
int	BHand::Get( char* motor, char* parameter, int* result)
{
	// prepare output char buffer
	sprintf( outbuf, "%sFGET %s\r", motor, parameter );
	nout = strlen( outbuf );

	// ask thread to send command
	bool immediate = (syncMode==BHMODE_RETURN);
	if( immediate )
		syncMode = BHMODE_SYNC;
	int ret = ComRequest( BHREQ_SUPERVISE );
	if( immediate )
		syncMode = BHMODE_RETURN;
	if( ret )
		return ret;

	// read parameters - as many as present
	int sz = strlen( Response() ), pos = 0, cnt = 0;
	while( pos<sz )
	{
		// skip white space
		while( isspace(Response()[pos]) && pos<sz )
			pos++;

		// read field
		if( pos<sz )
			sscanf( Response()+pos, "%d", result+cnt++ );

		// skip digits
		while( (isdigit(Response()[pos]) || Response()[pos]=='-') && pos<sz )
			pos++;
	}

	return 0;
}

// Get global parameters;
int	BHand::PGet( char* parameter, int* result)
{
	// prepare output char buffer
	sprintf( outbuf, "PGET %s\r", parameter );
	nout = strlen( outbuf );

	// ask thread to send command
	bool immediate = (syncMode==BHMODE_RETURN);
	if( immediate )
		syncMode = BHMODE_SYNC;
	int ret = ComRequest( BHREQ_SUPERVISE );
	if( immediate )
		syncMode = BHMODE_RETURN;
	if( ret )
		return ret;

	// read parameters - as many as present
	int sz = strlen( Response() ), pos = 0, cnt = 0;
	while( pos<sz )
	{
		// skip white space
		while( isspace(Response()[pos]) && pos<sz )
			pos++;

		// read field
		if( pos<sz )
			sscanf( Response()+pos, "%d", result+cnt++ );

		// skip digits
		while( (isdigit(Response()[pos]) || Response()[pos]=='-') && pos<sz )
			pos++;
	}

	return 0;
}

// Save specified fingers to permanent storage
int	BHand::Save( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sFSAVE\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Load specified fingers from permanent storage
int	BHand::Load( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sFLOAD\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Set specified fingers to defaults
int	BHand::Default( char* motor)
{
	// prepare output char buffer
	sprintf( outbuf, "%sFDEF\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Return CPU temperature
int	BHand::Temperature( int* result)
{
	// prepare output char buffer
	sprintf( outbuf, "PGET TEMP\r" );
	nout = strlen( outbuf );

	// ask thread to send command
	bool immediate = (syncMode==BHMODE_RETURN);
	if( immediate )
		syncMode = BHMODE_SYNC;
	int ret = ComRequest( BHREQ_SUPERVISE );
	if( immediate )
		syncMode = BHMODE_RETURN;
	
	// read temperature
	sscanf( inbuf, "%d", result );
	*result = *result/10;

	return ret;
}


// Initialize com port, send Reset to hand - called only once per hand
int	BHand::InitSoftware( int port, int priority )
{
	// clear port array in first call
	if( _clearFlag )
	{
		for( int i=0; i<BH_MAXPORT; i++ )
			_BHandArray[i] = NULL;
		_clearFlag = false;
	}

	// set all internal variables to their defaults
	syncMode = BHMODE_SYNC;
	pCallback = NULL;
	requestTimeout = INFINITE;

	// attempt to initialize the com port
	int ret = ComInitialize( port, priority );
	if( ret )
		return ret;

	//  reset hand
	return Reset();
}


// Initialize hand
int	BHand::InitHand( char* motor )
{
	// prepare output char buffer
	sprintf( outbuf, "%sHI\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}



///////////////////////////////////////////////////////////////////////////////
// New interactive commands

// Move to default positions
int	BHand::GoToDefault( char* motor )
{
	// prepare output char buffer
	sprintf( outbuf, "%sM\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Reset hand
int	BHand::Reset( void )
{
	// send empty command - to clear possible errors
	int result = Command( "" );
	if( result<0 )
		return result;

	// prepare output char buffer, reset baud rate
	sprintf( outbuf, "RESET\r" );
	nout = strlen( outbuf );
	requestBaud = 9600;

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Set baud rate
int	BHand::Baud( DWORD newbaud )
{
	// prepare output char buffer, set new baudrate
	sprintf( outbuf, "PSET BAUD %d\r", newbaud/100 );
	nout = strlen( outbuf );
	requestBaud = newbaud;

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Send any command and receive hand response
int	BHand::Command( char* send, char* receive )
{
	int result;

	// assign input, check size, add \r
	if( (nout = strlen( send )+1) >= BH_MAXCHAR )
		return BHERR_LONGSTRING;
	strcpy( outbuf, send );
	strcat( outbuf, "\r" );

	// exchange characters
	result = ComRequest( BHREQ_SUPERVISE );

	// copy result only if successful
	if( !result && receive )
		strcpy( receive, inbuf );
	else if( receive )
		receive[0] = 0;

	return result;
}


// Provide read access to receive buffer
const char* BHand::Response( void )
{
	// return inbuf address
	return( (const char*) inbuf );
}


// Provide read access to transmit buffer
const char* BHand::Buffer( void )
{
	// return outbuf address
	return( (const char*) outbuf );
}


// Delay - introduce delay
int BHand::Delay( DWORD msec )
{
	// prepare output char buffer
	sprintf( outbuf, "DELAY %lu\r", msec );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}



///////////////////////////////////////////////////////////////////////////////
// New loop mode commands implementation


// Enter loop mode for specified fingers
int	BHand::RTStart( char* motor )
{
	// set active flags
	rtFlags[0][0] = (strrchr(motor,'1')||strrchr(motor,'G'));
	rtFlags[1][0] = (strrchr(motor,'2')||strrchr(motor,'G'));
	rtFlags[2][0] = (strrchr(motor,'3')||strrchr(motor,'G'));
	rtFlags[3][0] = (strrchr(motor,'4')||strrchr(motor,'S'));

	// get flags from hand
	Get( "1", "LCV LCPG LFV LFS LFAP LFDP", &rtFlags[0][1] );
	Get( "2", "LCV LCPG LFV LFS LFAP LFDP", &rtFlags[1][1] );
	Get( "3", "LCV LCPG LFV LFS LFAP LFDP", &rtFlags[2][1] );
	Get( "4", "LCV LCPG LFV LFS LFAP LFDP", &rtFlags[3][1] );

	PGet( "LFT", rtGlobalFlags);

	// compute control and feedback positions
	int myFeedbackPos = 0; //BCZ 10/29/99, Line Added
	int myControlPos = 0; //BCZ 10/29/99, Line Added
	for( int m=0; m<4; m++ )
	{
		/* ORIGINAL CODE
		rtControl[m][0] = (m>0) ? rtControl[m-1][1]+rtFlags[m-1][0]*rtFlags[m-1][2] : 0;
		rtControl[m][1] = rtControl[m][0] + rtFlags[m][0]*rtFlags[m][1];

		rtFeedback[m][0] = (m>0) ? rtFeedback[m-1][3]+rtFlags[m-1][0]*rtFlags[m-1][6] : 0;
		rtFeedback[m][1] = rtFeedback[m][0] + rtFlags[m][0]*rtFlags[m][3];
		rtFeedback[m][2] = rtFeedback[m][1] + rtFlags[m][0]*rtFlags[m][4];
		rtFeedback[m][3] = rtFeedback[m][2] + rtFlags[m][0]*rtFlags[m][5]*2;
		*/

		//BCZ 10/29/99, NEW CODE
		rtControl[m][0] = myControlPos; if(rtFlags[m][0]*rtFlags[m][1]) myControlPos++; /* Velocity */
		rtControl[m][1] = myControlPos; if(rtFlags[m][0]*rtFlags[m][2]) myControlPos++; /* Proportional Gain */

		rtFeedback[m][0] = myFeedbackPos; if(rtFlags[m][0]*rtFlags[m][3]) myFeedbackPos++; /* Velocity */
		rtFeedback[m][1] = myFeedbackPos; if(rtFlags[m][0]*rtFlags[m][4]) myFeedbackPos++; /* Strain */
		rtFeedback[m][2] = myFeedbackPos; if(rtFlags[m][0]*rtFlags[m][5]) myFeedbackPos+=2; /* Position */
		rtFeedback[m][3] = myFeedbackPos; if(rtFlags[m][0]*rtFlags[m][6]) myFeedbackPos++; /*Delta Position */
		//END NEW CODE
	}

	rtGlobalFeedback[0] = myFeedbackPos; if(rtGlobalFlags[0]) myFeedbackPos+=2; /* Temperature */

	// assign totals
	nSend = myControlPos; //BCZ 10/29/99, WAS: nSend = rtControl[3][1];
	nReceive = myFeedbackPos; //BCZ 10/29/99, WAS: nReceive = rtFeedback[3][3];

	// prepare output char buffer
	sprintf( outbuf, "%sLOOP\r", motor );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Send and receive character in loop mode
int BHand::RTUpdate( bool control, bool feedback )
{
	// prepare output buffer
	if( control )
	{
		outbuf[0] = (feedback ? 'C' : 'c');
		memcpy( outbuf+1, rtOut, nSend);
		cout << "speed buffer = " << rtOut << endl;
		cout << "speed buffer size = " << nSend << endl;
		nout = nSend+1;
	}
	else
	{
		outbuf[0] = (feedback ? 'A' : 'a');
		nout = 1;
	}
	
	// ask thread to send command
	nin = (feedback ? nReceive : 1); //BCZ 11/16/99, WAS: nin = (feedback ? 1+nReceive: 1);
	return ComRequest( BHREQ_REALTIME );
}


// Abort loop mode or any other operation - Ctrl C
int	BHand::RTAbort( void )
{
	// prepare output char buffer
	outbuf[0] = 3; 
	outbuf[1] = 0;
	nout = 1;

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}


// Set original 9 parameters affecting loop mode
int	BHand::RTSetFlags( char* motor, bool LCV, int LCVC, bool LCPG, 
					   bool LFV, int LFVC, bool LFS, bool LFAP, bool LFDP, int LFDPC )
{
	// prepare output char buffer
	sprintf( outbuf, "%sFSET LCV %d LCVC %d LCPG %d LFV %d LFVC %d LFS %d LFAP %d LFDP %d LFDPC %d\r", 
		motor, LCV, LCVC, LCPG, LFV, LFVC, LFS, LFAP, LFDP, LFDPC );
	nout = strlen( outbuf );

	// ask thread to send command
	return ComRequest( BHREQ_SUPERVISE );
}

// Set all 11 parameters affecting loop mode
int	BHand::RTSetFlags( char* motor, bool LCV, int LCVC, bool LCPG, 
					   bool LFV, int LFVC, bool LFS, bool LFAP, bool LFDP, int LFDPC,
					   bool LFT, bool LFDPD)
{
	int err;

	// prepare output char buffer
	sprintf( outbuf, "%sFSET LCV %d LCVC %d LCPG %d LFV %d LFVC %d LFS %d LFAP %d LFDP %d LFDPC %d\r", 
		motor, LCV, LCVC, LCPG, LFV, LFVC, LFS, LFAP, LFDP, LFDPC );
	nout = strlen( outbuf );

	// ask thread to send command
	err = ComRequest( BHREQ_SUPERVISE );

	// prepare output char buffer
	sprintf( outbuf, "PSET LFT %d LFDPD %d\r", LFT, LFDPD );
	nout = strlen( outbuf );

	// ask thread to send command
	err |= ComRequest( BHREQ_SUPERVISE );

	return err;
}

// Dump the RT feedback packets to a file
void BHand::RTDumpFeedback( void )
{
	FILE *myFile;
	myFile = fopen("dump.dat","a");
	for(int i=0;i<20;i++) fprintf(myFile,"%d\t", (unsigned char)rtIn[i]);
	fprintf(myFile, "\n");
	fclose(myFile);

}

// Get feedback velocity for specified motor
char BHand::RTGetVelocity( char motor )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// get result
	if( !rtFlags[m][0] || !rtFlags[m][3] )
		return 0;
	else
		return rtIn[rtFeedback[m][0]];
}


// Get feedback strain for specified motor
unsigned char BHand::RTGetStrain( char motor )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// get result
	if( !rtFlags[m][0] || !rtFlags[m][4] )
		return 0;
	else
		return (unsigned char)rtIn[rtFeedback[m][1]];
}


// Get feedback absolute position for specified motor
int BHand::RTGetPosition( char motor )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// get result
	if( !rtFlags[m][0] || !rtFlags[m][5] )
		return 0;
	else //BCZ 10/15/99, Added (unsigned char) to the following line
		return( (((int)rtIn[rtFeedback[m][2]])<<8) + (int)(unsigned char)rtIn[rtFeedback[m][2]+1] );
}

// Get feedback temperature
int BHand::RTGetTemp()
{
	// get result
	if( !rtGlobalFlags[0] )
		return 0;
	else 
		return( (((int)rtIn[rtGlobalFeedback[0]])<<8) + (int)(unsigned char)rtIn[rtGlobalFeedback[0]+1] );
}

// Get feedback delta position for specified motor
char BHand::RTGetDeltaPos( char motor )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// get result
	if( !rtFlags[m][0] || !rtFlags[m][6] )
		return 0;
	else
		return rtIn[rtFeedback[m][3]];
}


// Get control velocity for specified motor
int BHand::RTSetVelocity( char motor, int velocity )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// active check
	if( !rtFlags[m][0] )
		return BHERR_MOTORINACTIVE;

	// range check
	if( velocity>127 )
		velocity = 127;
	else if( velocity<-127 )
		velocity = -127;

	// assign
	rtOut[rtControl[m][0]] = velocity;
	return 0;
}


// Get control position for specified motor
int BHand::RTSetGain( char motor, int gain )
{
	// convert '1'-'4' to 0-3
	int m = motor-'1';
	if( m<0 )
		m=0;
	else if( m>3 )
		m=3;

	// active check
	if( !rtFlags[m][0] )
		return BHERR_MOTORINACTIVE;

	// range check
	if( gain>255 )
		gain = 255;
	else if( gain<0 )
		gain = 0;

	// assign
	rtOut[rtControl[m][1]] = (unsigned char)(gain & 0xFF); /* BCZ 11/12/99 WAS: rtOut[rtControl[m][1]] = (char)gain; */
	cout << "Gain = " << rtOut[rtControl[m][1]] << endl;
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Comaptibility commands for vesion 1.0 implementation


// Open specified fingers to their limits
int	Open( char motor )
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->Open(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Close specified fingers to their limits
int	Close( char motor)
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->Close(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Open specified fingers by specified amount
int	StepOpen( char motor , int value )
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->StepOpen(str,value);
	else
		return BHERR_NOTINITIALIZED;
}


// Close specified fingers by specified amount
int	StepClose( char motor , int value )
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->StepClose(str,value);
	else
		return BHERR_NOTINITIALIZED;
}


// Move specified fingers to position
int	GoToPosition( char motor , int value )
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->GoToPosition(str,value);
	else
		return BHERR_NOTINITIALIZED;
}


// Call GoToPosition for each motor
int	GoToDifferentPositions( int value1, int value2, int value3, int value4 )
{
	if( _pBH )
		return _pBH->GoToDifferentPositions(value1,value2,value3,value4);
	else
		return BHERR_NOTINITIALIZED;
}


// Move all fingers to home position
int	GoToHome( void )
{
	if( _pBH )
		return _pBH->GoToHome();
	else
		return BHERR_NOTINITIALIZED;
}


// Stop specified fingers
int	StopMotor( char motor)
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->StopMotor(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Set parameter for specified motors;
int	Set( char motor, char parameter, int value )
{
	char str[2] = { motor, 0 };
	if( _pBH )
	{
		switch( parameter )
		{
		case 'C':
			return _pBH->Set(str,"MCV",value);
		case 'O':
			return _pBH->Set(str,"MOV",value);
		case 'M':
			return _pBH->Set(str,"HOLD",value);
		case 'S':
			return _pBH->Set(str,"MSG",value);
		default:
			return BHERR_BADPARAMETER;
		}
	}
	else
		return BHERR_NOTINITIALIZED;
}


// Get parameter for specified motors;
int	Get( char motor, char parameter, int* result)
{
	char str[2] = { motor, 0 };
	if( _pBH )
	{
		switch( parameter )
		{
		case 'C':
			return _pBH->Get(str,"MCV",result);
		case 'O':
			return _pBH->Get(str,"MOV",result);
		case 'P':
			return _pBH->Get(str,"P",result);
		case 'M':
			return _pBH->Get(str,"S",result);
		case 'S':
			return _pBH->Get(str,"SG",result);
		default:
			return BHERR_BADPARAMETER;
		}
	}
	else
		return BHERR_NOTINITIALIZED;
}


// Save specified fingers to permanent storage
int	Save( char motor)
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->Save(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Load specified fingers from permanent storage
int	Load( char motor)
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->Load(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Set specified fingers to defaults
int	Default( char motor)
{
	char str[2] = { motor, 0 };
	if( _pBH )
		return _pBH->Default(str);
	else
		return BHERR_NOTINITIALIZED;
}


// Return CPU temperature
int	Temperature( int* result)
{
	if( _pBH )
		return _pBH->Temperature(result);
	else
		return BHERR_NOTINITIALIZED;
}


// Initialize com port, send Reset to hand - called once per hand
int	InitSoftware( int comport )
{
	// range checking
	if( comport<1 || comport>BH_MAXPORT )
		return BHERR_PORTOUTOFRANGE;

	if( (!_clearFlag) && _BHandArray[comport-1] )
		return BHERR_BHANDEXISTS;
	else
	{
		_pBH = new BHand;
		return _pBH->InitSoftware(comport);
	}
}


// Initialize hand
int	InitHand( void)
{
	if( _pBH )
		return _pBH->InitHand( "GS" );
	else
		return BHERR_NOTINITIALIZED;
}


// Select default hand
int	SelectHand( int comport )
{
	// range checking
	if( comport<1 || comport>BH_MAXPORT )
		return BHERR_PORTOUTOFRANGE;

	// set default hand if present
	if( _BHandArray[comport-1] )
	{
		_pBH = _BHandArray[comport-1];
		return 0;
	}
	else
		return BHERR_NOTINITIALIZED;
}
