#pragma once

#include "VOpenNICommon.h"
//#include "SkeletonPoseDetector.h"


namespace V
{
	// Forward declaration
	class OpenNIDevice;
	struct OpenNIBone;


	enum UserStateEnum
	{
		USER_NONE,
		USER_INVALID,
		USER_LOOKING_FOR_POSE,
		USER_CALIBRATING,
		USER_CALIBRATED,
		USER_TRACKING
	};


	// Typedefs
	typedef std::vector<OpenNIBone*> OpenNIBoneList;



	// Class
	class OpenNIUser
	{
	public:
		typedef boost::shared_ptr<OpenNIUser> Ref;

	public:
		OpenNIUser( boost::int32_t id, OpenNIDevice* device );
		~OpenNIUser();
		void            init();
		void            update();
		void            updatePixels();
		void            updateBody();
		void            renderJoints( float width, float height, float depth, float pointSize=5, bool renderDepth=false );
		void            renderJointsRealWorld( float pointSize, float zScale = 1.0f );
		void            renderBone( int joint1, int joint2, float width=640, float height=480, float depth=1, bool doProjective = true, bool renderDepthInProjective=false );

		//bool			isTracking()		{ return _device->getUserGenerator()->GetSkeletonCap().IsTracking(mId); }

		OpenNIBoneList	getBoneList();
		OpenNIBone*		getBone( int id );

		bool			getUserPosition();
		void			calcDepthImageRealWorld( XnPoint3D* points );

		void			loadCalibrationDataToFile( const std::string& filename );
		void			saveCalibrationDataToFile( const std::string& filename );

		void			setText( const std::string& info )	{ _debugInfo = info; }
		const std::string& getText()		{ return _debugInfo; }

		//bool			hasPixels()			{ return (_userPixels)?true:false;}
		//uint8_t*		getPixels()			{ return _userPixels; }
		//uint16_t*		getDepthPixels();
		//XnPoint3D*		getDepthMapRealWorld()	{ return _depthMapRealWorld;	}
		uint32_t		getId()				{ return mId; }
		
		float*			getCenterOfMass( bool doProjectiveCoords=false );

		UserStateEnum	getUserState()		{ return mUserState;	}

		//uint32_t		getWidth()			{ return mWidth;	}
		//uint32_t		getHeight()			{ return mHeight;	}

		void			setEnablePixels( bool flag )	{ _enablePixels = flag;	}

		// Get closest and farest z distance for current user
		// This can be used to find a center point for local transformations
		uint16_t		getMinZDistance()				{ return mUserMinZDistance;	}
		uint16_t	
		getMaxZDistance()				{ return mUserMaxZDistance;	}

		XnVector3D		GetForwardVector();
		XnVector3D		GetUpVector();

	protected:
		void			allocate( int width, int height );
	protected:
		OpenNIDevice*	_device;
		boost::shared_ptr<OpenNIDevice>	_deviceRef;

		std::string		_debugInfo;


		//xn::UserGenerator*	mUserGen;
		xn::DepthGenerator*	mDepthGen;
//		xn::SkeletonCapability& mSkelCap;

		bool			_enablePixels;

		// User pixels for convenience
		//uint8_t*		_userPixels, *_backUserPixels;
		//uint16_t*		_userDepthPixels, *_backUserDepthPixels;
		//XnPoint3D*		_depthMapRealWorld, *_backDepthMapRealWorld;

		uint16_t		mUserMinZDistance, mUserMaxZDistance;
		uint16_t		mUserMinPixelIdx, mUserMaxPixelIdx;

		uint32_t		mId;
		float			mCenter[3];	// Center point
		float			mColor[3];

		//uint32_t		mWidth;		//
		//uint32_t		mHeight;	// Current dimensions of depthmap

		float           mAvgPosConfidence;
		
		OpenNIBoneList	mBoneList;

		XnBoundingBox3D mBoundingBox;

		UserStateEnum	mUserState;

		//StartPoseDetector*	mStartPoseDetector;
		//EndPoseDetector*		mEndPoseDetector;
	};
}