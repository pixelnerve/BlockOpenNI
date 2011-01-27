#pragma once

#include "VOpenNICommon.h"



namespace V
{
	// Forward declaration
	class OpenNIDevice;
	struct OpenNIBone;

	enum UserState
	{
		USER_NONE,
		USER_TRACKING,
		USER_LOOKING_FOR_POSE,
		USER_CALIBRATING
	};


	typedef std::vector<OpenNIBone*> UserBoneList;

	class OpenNIUser
	{
	public:
		typedef boost::shared_ptr<OpenNIUser> Ref;

	public:
		OpenNIUser( boost::int32_t id, OpenNIDevice* device );
		~OpenNIUser();
		void init();
		void update();
		void updatePixels();
		void updateBody();
		void renderJoints( float pointSize );
		void renderBone( int joint1, int joint2 );


		UserBoneList	getBoneList();
		OpenNIBone*		getBone( int id );

		void			setText( const std::string& info )	{ _debugInfo = info; }
		const std::string& getText()		{ return _debugInfo; }

		bool			hasPixels()			{ return (_userPixels)?true:false;}
		boost::uint8_t*	getPixels()			{ return _userPixels; }
		uint32_t		getId()				{ return mId; }
		float*			getCenterOfMass()	{ return mCenter; }

		uint32_t		getWidth()			{ return mWidth;	}
		uint32_t		getHeight()			{ return mHeight;	}

		void			setSkeletonSmoothing( float t );

	protected:
		OpenNIDevice*	_device;
		std::string		_debugInfo;
		uint8_t*		_userPixels;

		uint32_t		mId;
		float			mCenter[3];	// Center point
		float			mColor[3];

		uint32_t		mWidth;		//
		uint32_t		mHeight;	// Current dimensions of depthmap

		UserBoneList	mBoneList;
	};


	//
	// Manager
	//
	/***class OpenNIPlayerManager
	{
	public:
		OpenNIPlayerManager( OpenNI* openni );
		~OpenNIPlayerManager();
		boost::int32_t OpenNIPlayerManager::registerPlayer( uint32_t id, void* newUserFunc, void* lostUserFunc );


	protected:
		OpenNI*		_openni;

		boost::shared_ptr<std::vector<OpenNIPlayer*>> mPlayerList;
	};***/
}