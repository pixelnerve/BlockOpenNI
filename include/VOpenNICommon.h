#pragma once


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#undef min
#undef max
#include <gl/gl.h>
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#error "Unknown platform"
#endif

#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>

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
