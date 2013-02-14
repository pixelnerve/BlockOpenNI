#pragma once


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <windows.h>
#undef min
#undef max
#include <gl/gl.h>
#ifndef DEBUG_MESSAGE
	#define DEBUG_MESSAGE( x ) OutputDebugStringA( x )
#endif
#elif defined(linux) || defined(__linux) || defined(__linux__)
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#ifndef DEBUG_MESSAGE
	#define DEBUG_MESSAGE( x ) printf( "%s", x )
#endif
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#define BOOST_NO_CXX11_NUMERIC_LIMITS 1
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#ifndef DEBUG_MESSAGE
	#define DEBUG_MESSAGE( x ) printf( "%s", x )
#endif
#else
#error "Unknown platform"
#endif

// Boost
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// Std
#ifndef _DEBUG
    #define _HAS_ITERATOR_DEBUGGING 0
    #define _SECURE_SCL 0
#endif
#include <string>
#include <vector>
#include <iostream>

// OpenNI
#include "XnOpenNI.h"
#include "XnLog.h"
#include "XnCppWrapper.h"
#include "XnFPSCalculator.h"
#include "XnPropNames.h"
#include "XnVersion.h"
#include "XnUtils.h"
#include <XnUSB.h>




#define OPENNI_POSE_SAVE_CAPABILITY



namespace V
{
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

#define VALIDATE_GENERATOR( type, desc, generator, context) \
	{ \
		rc = context.EnumerateExistingNodes(nodes, type); \
		if (nodes.IsEmpty()) \
		{ \
			printf("No %s generator!\n", desc); \
			return 1; \
		} \
		(*(nodes.Begin())).GetInstance(generator); \
	}

	#ifndef SAFE_DELETE
	#define SAFE_DELETE( x )		if( x ) { delete x; x = NULL; }
	#endif
	#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY( x )	if( x ) { delete[] x; x = NULL; }
	#endif
	#ifndef SAFE_RELEASE
	#define SAFE_RELEASE( x )		if( x ) { x->Release(); }
	#endif



	// Max number of users available
	#define MAX_DEVICES 4
	// Maximum depth bins for the histogram image
	#define MAX_DEPTH 10000
	
	
	
	
	enum NodeResolution
	{
		RES_320x240 = XN_RES_QVGA,
		RES_640x480 = XN_RES_VGA,
		RES_800x600 = XN_RES_SVGA,
		RES_1024x768 = XN_RES_XGA,
		RES_1280x1024 = XN_RES_SXGA,
		RES_1600x1200 = XN_RES_UXGA
	};
	
	

	// Enumeration of production nodes
	enum ProductionNodeType
	{
		NODE_TYPE_NONE	= (1<<0),
		NODE_TYPE_IMAGE	= (1<<1),
		NODE_TYPE_IR	= (1<<2),
		NODE_TYPE_DEPTH	= (1<<3),
		NODE_TYPE_SCENE	= (1<<4),
		NODE_TYPE_USER	= (1<<5),
		NODE_TYPE_AUDIO	= (1<<6),
		NODE_TYPE_HANDS = (1<<7)
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
		SKEL_RIGHT_FOOT = 24,
		SKEL_TOTAL_COUNT = 24
	};
	// Max count of bones
	static const int BONE_COUNT = SKEL_TOTAL_COUNT;

}
