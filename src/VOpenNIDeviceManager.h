#pragma once


#include "VOpenNIDevice.h"
#include <thread>


namespace V
{
	class OpenNIDevice;
	class OpenNIUser;

	// A singleton
	class OpenNIDeviceManager : private boost::noncopyable
	{
	public:
		OpenNIDeviceManager();
		~OpenNIDeviceManager();

//        OpenNIDeviceRef         createDevice( const std::string& xmlFile, bool allocUserIfNoNode=false );
		V::OpenNIDeviceRef		createDevice( int nodeTypeFlags, int colorResolution=RES_640x480, int depthResolution=RES_640x480 );
		void                    createDevices( uint32_t deviceCount, int nodeTypeFlags, int colorResolution=RES_640x480, int depthResolution=RES_640x480 );

		void                    Release();

		void                    Init();

		OpenNIDevice::Ref       getDevice( uint32_t deviceIdx=0 );

		OpenNIUserRef           addUser( xn::UserGenerator* userGen, uint32_t id );
		void                    removeUser( uint32_t id );
		OpenNIUserRef           getFirstUser();
		OpenNIUserRef           getSecondUser();
		OpenNIUserRef           getLastUser();
		OpenNIUserRef           getUser( int id );
		bool                    hasUser( int32_t id );
		bool                    hasUsers();
		const uint32_t          getNumOfUsers();
		OpenNIUserList          getUserList();

		void					start();

		void					renderJoints( float width, float height, float depth, float pointSize, bool renderDepth=false );		
		void					renderJointsWS( float pointSize, float zScale = 1.0f );


		const std::string& getDebugInfo()			{ return mDebugInfo;}
		void setText( const std::string& info )		{ mDebugInfo = info; }

		const uint32_t getMaxNumOfUsers()			{ return mMaxNumOfUsers; }
		void setMaxNumOfUsers( uint32_t count )		{ mMaxNumOfUsers = count; }

		xn::Context*	getContext()				{ return &_context;	}


		//
		// Instance
		//
		static OpenNIDeviceManager& Instance() 
		{
			return _singletonPointer;
		}
		static OpenNIDeviceManager* InstancePtr() 
		{
			return &_singletonPointer;
		}

		void update();
		void stop()                                 { _isRunning = false; }

		
	private:
		// Copy constructor
		OpenNIDeviceManager( const OpenNIDeviceManager& ) {};
		// Operators
		OpenNIDeviceManager& operator = ( const OpenNIDeviceManager& ) { return *this; };

		void run();
	public:
		static bool						USE_THREAD;


	protected:
		static OpenNIDeviceManager		_singletonPointer;

		bool							mIsContextInit;
#ifdef WIN32
		std::shared_ptr<boost::thread> _thread;
#else
		boost::shared_ptr<std::thread> _thread;
#endif
		boost::mutex					 _mutex;

		bool							_isRunning;
		bool							_isStarted;

		xn::Context						_context;

		std::string						mDebugInfo;

		uint32_t						mMaxNumOfUsers;

		int								_idCount;

		// Device list
		OpenNIDeviceList				mDevices;

		// Generic user list. These users have no knowledge of which device they come from
		OpenNIUserList					mUserList;





		//
		// New Wrapper
		//
	public:
		void			AllocateMem( uint32_t width, uint32_t height );

		uint16_t*		getDepthMap( uint32_t deviceIdx=0 );
		uint16_t*		getDepthMapShift( uint32_t deviceIdx=0, uint32_t shiftMul=3 );
		uint8_t*		getColorMap( uint32_t deviceIdx=0 );
		void			SetPrimaryBuffer( uint32_t deviceIdx, uint32_t type );
		void			CalcDepthImageRealWorld( uint32_t deviceIdx, uint16_t* pixelData, XnPoint3D* worldData );
		void			SetFrameSync( uint32_t deviceIdx, uint32_t index1 );
		void			AlignRGBAndDepth( uint32_t deviceIdx=0 );
		void			CalcFieldOfView( uint32_t deviceIdx=0 );

		// Getters
		void			GetUserMap( uint32_t deviceIdx, uint32_t labelId, uint16_t* labelMap );
		float			GetFieldOfViewH( uint32_t deviceIdx=0 );
		float			GetFieldOfViewV( uint32_t deviceIdx=0 );

		void			UpdateFrame( uint32_t deviceIdx=0 );
		void			UpdateUsers( uint32_t deviceIdx=0 );

#ifdef _WIN32
		void			EnableNetworking( bool flag, const std::string& hostName="127.0.0.1", uint16_t port=8888 );
		void			SendNetworkUserData();
		void			SendNetworkUserData( uint32_t userId );
		void			SendNetworkPixels( uint32_t deviceIdx, uint16_t* pixels, uint32_t sendBlocksSize/*=65535*/ );
		void			SendNetworkUserPixels( uint32_t deviceIdx, uint32_t userId, uint32_t sendBlocksSize/*=65535*/ );
#endif
	public:
		//typedef boost::shared_ptr<sDevice> SDeviceRef;
		//std::vector<SDevice::Ref>		mDeviceList;

		xn::Query						mQuery;
		xn::Generator*					mPrimaryGen;
		std::vector<xn::Device>         mDevicesList;
		std::vector<xn::DepthGenerator>	mDepthGenList;
		std::vector<xn::ImageGenerator>	mImageGenList;
		std::vector<xn::IRGenerator>	mIRGenList;
		std::vector<xn::SceneAnalyzer>	mSceneAnalyzerList;
		std::vector<xn::UserGenerator>	mUserGenList;
		std::vector<xn::HandsGenerator>	mHandsGenList;

//		std::vector<xn::DepthMetaData>	mDepthMDList;
//		std::vector<xn::ImageMetaData>	mImageMDList;
//		std::vector<xn::IRMetaData>		mIRMDList;
//		std::vector<xn::SceneMetaData>	mSceneMDList;

		xn::DepthMetaData				mDepthMD;
		xn::ImageMetaData				mImageMD;
		xn::IRMetaData					mIRMD;
		xn::SceneMetaData				mSceneMD;

		uint32_t						mDeviceCount;
		uint32_t						mDepthGenCount;
		uint32_t						mImageGenCount;
		uint32_t						mIRGenCount;
		uint32_t						mSceneAnalyzerCount;
		uint32_t						mUserGenCount;
		uint32_t						mHandsGenCount;

		uint16_t*						mDepthMap;	// Holds Depth map data
		uint8_t*						mColorMap;	// Holds Image/IR map data

		// Users
		uint32_t						mNumOfUsers;
		std::vector<SUser>				mSUserList;


		// FOV
		float							mHFov, mVFov;
		float							mRealWorldXtoZ; 
		float							mRealWorldYtoZ;


#ifdef _WIN32
		bool							mEnableNetwork;	// Enable sending skeleton data over the network. default port: 8888
		std::string						mNetworkMessage;
		OpenNINetwork*					mNetworkMsg;
		uint16_t*						mTempPixels;
#endif
	};


}  // namespace