#pragma once


#include <XnOpenNI.h>
#include <XnLog.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>

namespace V
{

	//---------------------------------------------------------------------------
	// Macros
	//---------------------------------------------------------------------------
	#define CHECK_RC( rc, what )								\
		if( rc != XN_STATUS_OK )									\
		{															\
		OutputDebugStringA( what ); \
		OutputDebugStringA( ": '" ); \
		OutputDebugStringA( xnGetStatusString( rc ) ); \
		OutputDebugStringA( "'\n" ); \
		}


	#ifndef SAFE_DELETE
	#define SAFE_DELETE( x )		if( x ) { delete x; x = NULL; }
	#endif
	#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY( x )	if( x ) { delete[] x; x = NULL; }
	#endif



	// Max number of users available
	#define MAX_DEVICES 4
	// Maximum depth bins on the histogram image
	#define MAX_DEPTH 10000

	// Enumeration of production nodes
	enum ProductionNodeType
	{
		NODE_TYPE_IMAGE	= 0x00000001,
		NODE_TYPE_IR	= 0x00000010,
		NODE_TYPE_DEPTH	= 0x00000100,
		NODE_TYPE_USER	= 0x00001000,
		NODE_TYPE_AUDIO	= 0x00010000
	};

}
