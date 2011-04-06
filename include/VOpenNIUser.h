#pragma once

#include "VOpenNICommon.h"



namespace V
{
	// Forward declaration
	class OpenNIDevice;
	typedef boost::shared_ptr<OpenNIDevice> OpenNIDeviceRef;
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
		OpenNIUser( boost::int32_t id, OpenNIDeviceRef device );
		~OpenNIUser();
		void init();
		void update();
		void updatePixels();
		void updateBody();
		void renderJoints( float width, float height, float depth, float pointSize=5, bool renderDepth=false );
		void renderJointsRealWorld( float pointSize );
		void renderBone( int joint1, int joint2, float width=640, float height=480, float depth=1, bool doProjective = true, bool renderDepthInProjective=false );

		//bool			isTracking()		{ return _device->getUserGenerator()->GetSkeletonCap().IsTracking(mId); }

		OpenNIBoneList	getBoneList();
		OpenNIBone*		getBone( int id );

		void			setText( const std::string& info )	{ _debugInfo = info; }
		const std::string& getText()		{ return _debugInfo; }

		bool			hasPixels()			{ return (_userPixels)?true:false;}
		boost::uint8_t*	getPixels()			{ return _userPixels; }
		boost::uint16_t*	getDepthPixels(){ return _userDepthPixels; }
		uint32_t		getId()				{ return mId; }
		
		float*			getCenterOfMass( bool doProjectiveCoords=false );

		UserStateEnum	getUserState()		{ return mUserState;	}

		uint32_t		getWidth()			{ return mWidth;	}
		uint32_t		getHeight()			{ return mHeight;	}

		void			setSkeletonSmoothing( float t );

		void			setEnablePixels( bool flag )	{ _enablePixels = flag;	}

	protected:
		void			allocate( int width, int height );

	protected:
		OpenNIDevice*	_device;
		OpenNIDeviceRef	_deviceRef;

		std::string		_debugInfo;

		bool			_enablePixels;

		// User pixels for convenience
		uint8_t*		_userPixels;
		uint8_t*		_backUserPixels;
		uint16_t*		_userDepthPixels, *_backUserDepthPixels;

		float			mSkeletonSmoothing;

		uint32_t		mId;
		float			mCenter[3];	// Center point
		float			mColor[3];

		uint32_t		mWidth;		//
		uint32_t		mHeight;	// Current dimensions of depthmap

		OpenNIBoneList	mBoneList;

		UserStateEnum	mUserState;
	};
}