/***
	OpenNI Wrapper.

	Victor Martins
	2010-2011 Pixelnerve 
	http://www.pixelnerve.com


	Current version:
		OpenNI	1.3.4.3
		NITE	1.4.2.4
 ***/

#pragma once

#include "VOpenNICommon.h"
#include "VOpenNISurface.h"
#include "VOpenNIUser.h"

namespace V
{

	// Forward declarations
	class OpenNIDevice;
	class OpenNIDeviceManager;
	class OpenNINetwork;
	struct OpenNIBone;
	class OpenNIUser;
	class OpenNIDevice;
	class UserListener;
	struct sDevice;


	// Typedefs
	typedef boost::shared_ptr<OpenNIUser> OpenNIUserRef;
	typedef boost::shared_ptr<OpenNIDevice> OpenNIDeviceRef;
	typedef std::list< OpenNIDeviceRef > OpenNIDeviceList;
	typedef std::list< OpenNIUserRef > OpenNIUserList;
	typedef std::vector<UserListener*> UserListenerList;
	//typedef std::list< boost::shared_ptr<sDevice> > OpenNISDeviceList;





	struct UserEvent
	{
		UserEvent()
		{
			mId = 0;
			mDevice = OpenNIDeviceRef();
			mUser = OpenNIUserRef();
		}

		uint32_t				mId;
		OpenNIDeviceRef			mDevice;
		OpenNIUserRef			mUser;
	};

	class UserListener
	{
	public:
		virtual void onNewUser( UserEvent event ) {};
		virtual void onLostUser( UserEvent event ) {};
		virtual void onCalibrationStart( UserEvent event ) {};
		virtual void onCalibrationComplete( UserEvent event ) {};
		virtual void onUserExit( UserEvent event ) {};
		virtual void onUserReEnter( UserEvent event ) {};
	};




	/************************************************************************/
	/*                                                                      */
	/************************************************************************/

	class OpenNIDevice
	{
	public:
		typedef boost::shared_ptr<OpenNIDevice> Ref;

	public:
		OpenNIDevice( int index, OpenNIDeviceManager* mgr );
		OpenNIDevice( int index, OpenNIDeviceManager* mgr, xn::Device* device );
		~OpenNIDevice();
		bool init( boost::uint64_t nodeTypeFlags, int colorResolution, int depthResolution );
//		bool initFromXmlFile( const std::string& xmlFile, bool allocUserIfNoNode=false );

		void release();

		void start();

		void allocate( uint64_t flags ); //, uint32_t width, uint32_t height );

		bool requestUserCalibration();
		void setResolution( ProductionNodeType nodeType, int res, int fps );
		//void setFPS( ProductionNodeType nodeType, int fps );
		//void setMapOutputMode( ProductionNodeType nodeType, int width, int height, int fps );
		void readFrame();
        
		void calcDepthImageRealWorld();
		void calcDepthImageRealWorld( uint16_t* pixelData, XnPoint3D* worldData );
		void calcDepthImageRealWorld( XnPoint3D* buffer );
		void getLabelMap( uint32_t labelId, uint16_t* labelMap );
		void calculateHistogram();
        void remapDepthMap( uint16_t* newDepthMap, uint16_t depthExtraScale = 1, bool invertDepth = false );


		void setLimits( int nearClip, int farClip );

		void setPrimaryBuffer( int type );
		void setMirrorMode( int type, bool flag );
		void setSkeletonSmoothing( float value );
		
		/*
			Get Horizontal FOV
		*/
		float FieldOfViewHorz()
		{
			XnFieldOfView fieldOfView;
			_depthGen->GetFieldOfView( fieldOfView );
			return (float)fieldOfView.fHFOV;
		}

		/*
			Get Vertical FOV
		*/
		float FieldOfViewVert()
		{
			XnFieldOfView fieldOfView;
			_depthGen->GetFieldOfView( fieldOfView );
			return (float)fieldOfView.fVFOV;
		}


		/*Unused variable 'index'
			Get floor plane.
			Only works with SceneAnalyzer enabled and when a user is found.
		*/
		XnPlane3D GetFloor() const
		{
			XnPlane3D plane; 
			XnStatus st = _sceneAnalyzer->GetFloor( plane );
			if( st != XN_STATUS_OK )
			{
				DEBUG_MESSAGE( "Failed to find floor plane\n" );
			}
			return plane;
		}


		// Shifts depth pixel (bit operator) NOTE!this fucks with the correct distance values
		// To get correct distances, set this value to 0 (zero)
		int getDepthShiftMul()						{ return mDepthShiftValue;          }
		void setDepthShiftMul( int value )			{ mDepthShiftValue = value;         }
		void resetDepthShiftMul()					{ mDepthShiftValue = 0;             }

		void setDepthInvert( bool flag );
		bool getDepthInvert()						{ return _isDepthInverted; }

		void setHistogram( bool flag )				{ _enableHistogram = flag;	}
		bool getHistogram()							{ return _enableHistogram;	}

		void setUserCalibration( bool flag )		{ _enableUserCalibration = flag;	}
		bool getUserCalibration()					{ return _enableUserCalibration;	}
		
		void setConfidenceThreshold( float value )  { _confidenceThreshold = value;     }
		float getConfidenceThreshold()              { return _confidenceThreshold;      } 

		boost::uint8_t* getColorMap();
		boost::uint16_t* getIRMap();
		boost::uint8_t* getIRMap8i();
		boost::uint16_t* getDepthMap();
		boost::uint8_t* getDepthMap24();
		XnPoint3D* getDepthMapRealWorld();
		boost::uint16_t* getRawDepthMap();

//		xn::DepthMetaData* getDepthMetaData()		{ return &_depthMetaData; }
//		xn::SceneMetaData* getSceneMetaData()		{ return &_sceneMetaData; }
		xn::ImageGenerator*	getImageGenerator()		{ return _imageGen;	}
		xn::IRGenerator* getIRGenerator()			{ return _irGen;	}
		xn::DepthGenerator*	getDepthGenerator()		{ return _depthGen;	}
		xn::UserGenerator* getUserGenerator()		{ return _userGen;	}
		xn::HandsGenerator* getHandsGenerator()		{ return _handsGen;	}
		xn::Context*	getContext()				{ return _context;	}
        
        bool isImageDataNew()                       { return _imageGen->IsDataNew(); };
        bool isIRDataNew()                          { return _irGen->IsDataNew(); };
        bool isDepthDataNew()                       { return _depthGen->IsDataNew(); };
        bool isUserDataNew()                        { return _userGen->IsDataNew(); };
        bool isHandsDataNew()                       { return _handsGen->IsDataNew(); };

		bool isOneTimeCalibration()					{ return _isOneTimeCalibration;	}
		void enableOneTimeCalibration( bool value )	{ _isOneTimeCalibration = value;	}

		// Set calibration state. true/false
		void setCalibrationState( bool value )		{ _isFirstCalibrationComplete = value;	}

		float getMinDistance()						{ return mMinDistance; }
		float getMaxDistance()						{ return mMaxDistance; }

		void setAlignWithDepthGenerator();

		void resetUser( int userId );

		//const std::string& getDebugInfo()			{ return mDebugInfo;}

		/*void addUser( uint32_t id );
		OpenNIUserRef getUser( uint32_t id );
		void removeUser( uint32_t id );
		bool hasUser( int32_t id );
		bool hasUsers()								{ return (mUserList.size()>0)?true:false; }*/


		XnSkeletonProfile getSkeletonProfile()	{ return mSkeletonProfile;	}
		void setSkeletonProfile( XnSkeletonProfile profile ) 
		{ 
			mSkeletonProfile = profile; 
			if( _isUserOn && _userGen->IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
			{
				_userGen->GetSkeletonCap().SetSkeletonProfile( mSkeletonProfile );
			}
		}


		uint32_t addListener( UserListener* listener )
		{
			mListeners.push_back( listener );
			return mListeners.size()-1;
		}
		void removeListener( uint32_t id )
		{
			//mListeners.erase( id );
		}
		UserListenerList getListeners()			{ return mListeners;	}

		static void XN_CALLBACK_TYPE Callback_UserExit( xn::UserGenerator& generator, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_UserReEnter( xn::UserGenerator& generator, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_PoseDetected( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_PoseInProgress( xn::PoseDetectionCapability& pose, const XnChar* strPose, XnUserID nId, XnPoseDetectionStatus poseError, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_PoseDetectionEnd( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_CalibrationStart( xn::SkeletonCapability& capability, XnUserID nId, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_CalibrationInProgress( xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus calibrationError, void* pCookie );
		static void XN_CALLBACK_TYPE Callback_CalibrationComplete( xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus calibrationError, void* pCookie );


	private:
		void run();
		uint32_t enumDevices( void );
		void privateInit();
	public:
		int                     _index;
		std::string				mDeviceName;

		OpenNIDeviceManager*    _mgr;

		bool					_isDepthInverted;
		bool					_enableHistogram;
		bool                    _enableUserCalibration;

		//std::string				_configFile;

		xn::Context*            _context;	// Pointer to context in device manager
		xn::ScriptNode			mScriptNode;

//		xn::Device				_device;	// Device object

		xn::EnumerationErrors	_errors;
		XnStatus				_status;

		//XnFPSData					_fpsData;
		XnMapOutputMode			_mapMode; 


		UserListenerList		mListeners;

		// Buffers
		OpenNISurface8*			mColorSurface;
		OpenNISurface8*			mIRSurface;
		OpenNISurface16*		mDepthSurface;

		const int				mBitsPerPixel;
		boost::uint16_t*		_irData;
		boost::uint8_t*			_irData8;
		boost::uint16_t*		_depthData;
		boost::uint16_t*		_backDepthData;
		boost::uint8_t*			_depthDataRGB;
		XnPoint3D*				_depthMapRealWorld;
		XnPoint3D*				_backDepthMapRealWorld;

		XnRGB24Pixel*			g_pTexMap;
		int						g_MaxDepth;
		float*					g_pDepthHist;

		int						mDepthShiftValue;	// pixel shift left value (intensifies the distance map)
		float					mMinDistance, mMaxDistance;

		XnSkeletonProfile		mSkeletonProfile;

		// Temp buffers
		const XnUInt8*			pImage;
		const XnDepthPixel*		pDepth;
		const XnIRPixel*		pIR;

		// Flags for nodes
		bool					_isImageOn;
		bool					_isIROn;
		bool					_isDepthOn;
		bool					_isSceneOn;
		bool					_isUserOn;
		bool					_isAudioOn;
		bool					_isHandsOn;

		// User Calibrations
		bool					_isOneTimeCalibration;
		bool					_isFirstCalibrationComplete;
		
		// User confidence threshold (below this value, user is not computed. Still it remains active until openni removes it)
		float                   _confidenceThreshold;

		xn::Generator*			_primaryGen;
		xn::DepthGenerator*		_depthGen;
		xn::ImageGenerator*		_imageGen;
		xn::IRGenerator*		_irGen;
		xn::UserGenerator*		_userGen;
		xn::HandsGenerator*		_handsGen;
		//xn::AudioGenerator*		_audioGen;
		xn::SceneAnalyzer*      _sceneAnalyzer;

		// scene
		xn::ImageMetaData		_imageMetaData;
		xn::IRMetaData			_irMetaData;
		xn::DepthMetaData		_depthMetaData;
		xn::SceneMetaData		_sceneMetaData;
		xn::AudioMetaData		_audioMetaData;


		// Internal clip planes
		uint32_t				mNearClipPlane, mFarClipPlane;

		// Users
		OpenNIUserList			mUserList;
	};





	/************************************************************************/
	/* Device Manager */
	/************************************************************************/

	struct NIDepthGenerator
	{
		NIDepthGenerator() : mWidth(0), mHeight(0), mHFov(0), mVFov(0), mMap(NULL), mGen(NULL) {};

		uint32_t			mWidth, mHeight;
		uint32_t			mFPS;
		xn::DepthGenerator*	mGen;
		xn::DepthMetaData	mMetaData;
		uint16_t*			mMap;
		float				mHFov, mVFov;
	};

	struct NIIRGenerator
	{
		NIIRGenerator() : mWidth(0), mHeight(0), mHFov(0), mVFov(0), mMap(NULL), mGen(NULL) {};

		uint32_t			mWidth, mHeight;
		uint32_t			mFPS;
		xn::IRGenerator*	mGen;
		xn::IRMetaData		mMetaData;
		uint8_t*			mMap;
		float				mHFov, mVFov;
	};

	struct NIImageGenerator
	{
		NIImageGenerator() : mWidth(0), mHeight(0), mMap(NULL), mGen(NULL) {};

		uint32_t			mWidth, mHeight;
		uint32_t			mFPS;
		xn::ImageGenerator*	mGen;
		xn::ImageMetaData	mMetaData;
		uint8_t*			mMap;
	};

	struct NIUserGenerator
	{
		NIUserGenerator() : mWidth(0), mHeight(0), mGen(NULL) {};

		uint32_t			mWidth, mHeight;
		xn::UserGenerator*	mGen;
		xn::SceneMetaData	mMetaData;
		uint32_t			mNumOfUsers;
	};

	struct NISceneGenerator
	{
		NISceneGenerator() : mWidth(0), mHeight(0), mGen(NULL) {};

		uint32_t			mWidth, mHeight;
		xn::SceneAnalyzer*	mGen;
		xn::SceneMetaData	mMetaData;
	};

	struct NIHandsGenerator
	{
		NIHandsGenerator() : mGen(NULL) {};

		xn::HandsGenerator*	mGen;
	};

	// Define a kinect device
	struct SDevice
	{
		typedef boost::shared_ptr<SDevice> Ref;

		NIDepthGenerator	mDepthGen_;
		NIIRGenerator		mIRGen_;
		NIImageGenerator	mImageGen_;
		NIUserGenerator		mUserGen_;
		NISceneGenerator	mSceneGen_;
		NIHandsGenerator	mHandsGen_;

		// Depth
		xn::DepthGenerator	mDepthGen;
		xn::DepthMetaData	mDepthMD;

		// RGB Image
		xn::ImageGenerator	mImageGen;
		xn::ImageMetaData	mImageMD;

		// Infrared
		xn::IRGenerator		mIRGen;
		xn::IRMetaData		mIRMD;

		// Scene/User analyzer
		xn::SceneAnalyzer	mSceneAnalyzer;
		xn::UserGenerator	mUserGen;
		xn::SceneMetaData	mSceneMD;

		// Fov, defines the view frustum
		float				mHFov, mVFov;
		float				mRealWorldXtoZ, mRealWorldYtoZ;

		uint32_t			mNumOfUsers;
		uint32_t			mMaxNumOfUsers;
	};

	
	struct SBone
	{
		XnPoint3D			mPos;	// Real world position
		XnPoint3D			mProjPos;	// Projective position
		XnMatrix3X3			mOrientation;	// Orientation matrix
	};

	struct SLimb
	{
		SBone				mB0, mB1;
	};

	struct SUser
	{
		typedef boost::shared_ptr<SUser> Ref;

		SUser()	: mBoneArray(NULL), doUpdateProjective(false) {};

		uint32_t			mNumOfBones;
		SBone*				mBoneArray;
		bool				doUpdateProjective;	// Enable projective computation?
	};


	// Flags that control internal computations
	struct SUpdateInfo
	{
		uint32_t			mMaxUsers;	// Max number of valid users
		bool				mUpdateUserProjective;	// Update bone's projective position?
		bool				mUpdateSceneLabelMap;	// Update scene labels?
		bool				mUpdateDepth8bit;		// Update Depthmap in 8bit?
	};

}	// V