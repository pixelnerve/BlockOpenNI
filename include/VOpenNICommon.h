#pragma once


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#undef min
#undef max
#include <gl/gl.h>
#define DEBUG_MESSAGE( x ) OutputDebugStringA( x )
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define DEBUG_MESSAGE( x ) printf( x )
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#define DEBUG_MESSAGE( x ) printf( x )
#else
#error "Unknown platform"
#endif

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <string>
#include <vector>

#include "XnOpenNI.h"
#include "XnLog.h"
#include "XnCppWrapper.h"
#include "XnFPSCalculator.h"
#include "XnPropNames.h"
#include "XnVersion.h"
#include "XnUtils.h"




namespace V
{
	using namespace boost;
	//---------------------------------------------------------------------------
	// Macros
	//---------------------------------------------------------------------------
	#define CHECK_RC( rc, what ) \
		if( rc != XN_STATUS_OK ) \
		{ \
			DEBUG_MESSAGE( what ); \
			DEBUG_MESSAGE( ": '" ); \
			DEBUG_MESSAGE( xnGetStatusString( rc ) ); \
			DEBUG_MESSAGE( "'\n" ); \
		}


	#ifndef SAFE_DELETE
	#define SAFE_DELETE( x )		if( x ) { delete x; x = NULL; }
	#endif
	#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY( x )	if( x ) { delete[] x; x = NULL; }
	#endif
	#ifndef SAFE_RELEASE
	#define SAFE_RELEASE( x )		if( x ) { x->release(); }
	#endif



	// Max number of users available
	#define MAX_DEVICES 4
	// Maximum depth bins for the histogram image
	#define MAX_DEPTH 10000

	// Enumeration of production nodes
	enum ProductionNodeType
	{
		NODE_TYPE_NONE	= 0x00000000,
		NODE_TYPE_IMAGE	= 0x00000001,
		NODE_TYPE_IR	= 0x00000010,
		NODE_TYPE_DEPTH	= 0x00000100,
		NODE_TYPE_USER	= 0x00001000,
		NODE_TYPE_AUDIO	= 0x00010000
	};


	// Enumeration for skeleton joints
	enum SkeletonBoneType
	{
		SKEL_HEAD = 1, 
		SKEL_NECK = 2, 
		SKEL_TORSO = 3, 
		SKEL_WAIST = 4, 
		SKEL_LEFT_COLLAR = 5, 
		SKEL_LEFT_SHOULDER = 6, 
		SKEL_LEFT_ELBOW = 7, 
		SKEL_LEFT_WRIST = 8, 
		SKEL_LEFT_HAND = 9, 
		SKEL_LEFT_FINGERTIP = 10, 
		SKEL_RIGHT_COLLAR = 11, 
		SKEL_RIGHT_SHOULDER = 12, 
		SKEL_RIGHT_ELBOW = 13, 
		SKEL_RIGHT_WRIST = 14, 
		SKEL_RIGHT_HAND = 15, 
		SKEL_RIGHT_FINGERTIP = 16, 
		SKEL_LEFT_HIP = 17, 
		SKEL_LEFT_KNEE = 18, 
		SKEL_LEFT_ANKLE = 19, 
		SKEL_LEFT_FOOT = 20, 
		SKEL_RIGHT_HIP = 21, 
		SKEL_RIGHT_KNEE = 22, 
		SKEL_RIGHT_ANKLE = 23, 
		SKEL_RIGHT_FOOT = 24 
	};
	// Mac count of bones
	static const int BONE_COUNT = 24;

}
