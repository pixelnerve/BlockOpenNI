//#include "VOpenNICommon.h"
#include "VOpenNIUser.h"
#include "VOpenNIDevice.h"



namespace V
{
	using namespace xn;



	/************************************************************************/
	/* Callbacks
	*/
	/************************************************************************/

	static char g_strPose[1023];
	static bool g_bNeedPose;
	//static xn::UserGenerator *g_UserGen = NULL;
	static xn::UserGenerator g_UserGenObj;


	// Callback: New user was detected
	void XN_CALLBACK_TYPE Callback_NewUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "New User: '" << nId << "'" << std::endl;
		OutputDebugStringA( ss.str().c_str() );

		//g_UserGen = &generator;
		g_UserGenObj = generator;
		g_UserGenObj.GetPoseDetectionCap().StartPoseDetection( g_strPose, nId );

		// Add new user
		OpenNIDeviceManager::Instance().setText( ss.str() );
		OpenNIDeviceManager::Instance().addUser( &generator, nId );
	}
	// Callback: An existing user was lost
	void XN_CALLBACK_TYPE Callback_LostUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Lost User: '" << nId << "'" << std::endl;
		OutputDebugStringA( ss.str().c_str() );

		OpenNIDeviceManager::Instance().setText( ss.str() );

		// Remove user
		//g_UserGen = NULL;
		OpenNIDeviceManager::Instance().removeUser( nId );
	}
	// Callback: Detected a pose
	void XN_CALLBACK_TYPE Callback_PoseDetected( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Pose detected: '" << strPose << "' on User: " << nId << std::endl;
		OutputDebugStringA( ss.str().c_str() );

		if( g_UserGenObj ) 
		{
			OpenNIDeviceManager::Instance().setText( ss.str() );

			// Pose found! Now we need skeleton calibration.
			//capability.StopPoseDetection( nId );
			g_UserGenObj.GetPoseDetectionCap().StopPoseDetection( nId );
			g_UserGenObj.GetSkeletonCap().RequestCalibration( nId, TRUE );
		}
	}
	// Callback: When the user is out of pose
	void XN_CALLBACK_TYPE Callback_PoseDetectionEnd( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Pose detection end. User: '" << strPose << "'. Id: " << nId << std::endl;
		OutputDebugStringA( ss.str().c_str() );

		OpenNIDeviceManager::Instance().setText( ss.str() );
	}

	// Callback: Started calibration
	void XN_CALLBACK_TYPE Callback_CalibrationStart( xn::SkeletonCapability& capability, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Calibration started for user " << nId << std::endl;
		OutputDebugStringA( ss.str().c_str() );

		OpenNIDeviceManager::Instance().setText( ss.str() );
	}
	// Callback: Finished calibration
	void XN_CALLBACK_TYPE Callback_CalibrationEnd( xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie )
	{
		if( bSuccess )
		{
			// Calibration succeeded
			std::stringstream ss;
			ss << "Calibration complete, start tracking user " << nId << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			OpenNIDeviceManager::Instance().setText( ss.str() );
			if( g_UserGenObj ) 
			{
				g_UserGenObj.GetSkeletonCap().StartTracking( nId );
			}
		}
		else
		{
			// Calibration failed
			std::stringstream ss;
			ss << "Calibration failed for user " << nId << std::endl;
			OutputDebugStringA( ss.str().c_str() );

			OpenNIDeviceManager::Instance().setText( ss.str() );

			if( g_bNeedPose )
			{
				g_UserGenObj.GetPoseDetectionCap().StartPoseDetection( g_strPose, nId );
			}
			else
			{
				g_UserGenObj.GetSkeletonCap().RequestCalibration( nId, TRUE );
			}
		}
	}





	/************************************************************************/
	/* OpenNI Device Class
	*/
	/************************************************************************/
	const bool OpenNIDevice::USE_THREAD = false;

	OpenNIDevice::OpenNIDevice( xn::Context* context ) : mBitsPerPixel( 3 )
	{
		_context = context;

		//_configFile = "";
		//mDebugInfo = "No debug information\n";

		_isRunning = false;
		_isDepthInverted = false;

		_device = NULL;

		_primaryGen = NULL;
		_imageGen = NULL;
		_irGen = NULL;
		_depthGen = NULL;
		_audioGen = NULL;
		_userGen = NULL;


		mColorSurface = NULL;
		mIRSurface = NULL;
		mDepthSurface = NULL;

		//_colorData = NULL;
		_irData = NULL;
		_irData8 = NULL;
		_depthData = NULL;
		//_depthData8 = NULL;
		_depthDataRGB = NULL;
		_backDepthData = NULL;
		g_pTexMap = NULL;
		g_pDepthHist = NULL;

		_imageMetaData = NULL;
		_irMetaData = NULL;
		_depthMetaData = NULL;
		_sceneMetaData = NULL;
		_audioMetaData = NULL;

		_isImageOn = false;
		_isIROn = false;
		_isDepthOn = false;
		_isUserOn = false;
		_isAudioOn = false;

		//_callback = NULL;
		//_callback = new OpenNIDeviceCallback( this, &OpenNIDevice::CallbackFunc );
	}


	OpenNIDevice::~OpenNIDevice()
	{
		_context = NULL;
		release();
	}


	bool OpenNIDevice::init( boost::uint64_t nodeTypeFlags )
	{
		_status = _context->Init();
		if( _status != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << "Couldn't create context" << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			return false;
		}


		//
		// Find about plugged-in devices
		//
		xn::NodeInfoList list;
		_status = _context->EnumerateExistingNodes( list );
		if( _status == XN_STATUS_OK )
		{
			for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
			{
				std::stringstream ss;
				switch ((*it).GetDescription().Type)
				{
				case XN_NODE_TYPE_DEVICE:
					(*it).GetInstance( *_device );
					ss << "Device: " << _device->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					mDeviceName = _device->GetName();
					break;
				}
			}
		}



		// Pick image or IR map
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			_imageGen = new xn::ImageGenerator();
			_imageMetaData = new xn::ImageMetaData();
			_status = _imageGen->Create( *_context );
			CHECK_RC( _status, "Create image generator" );
			_isImageOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			_irGen = new xn::IRGenerator();
			_irMetaData = new xn::IRMetaData();
			_status = _irGen->Create( *_context );
			CHECK_RC( _status, "Create image generator" );
			_isIROn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			// Depth map
			_depthGen = new xn::DepthGenerator();
			_depthMetaData = new xn::DepthMetaData();
			_status = _depthGen->Create( *_context );
			CHECK_RC( _status, "Create depth generator" );
			_isDepthOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			// User
			_userGen = new xn::UserGenerator();
			_sceneMetaData = new xn::SceneMetaData();
			_status = _userGen->Create( *_context );
			CHECK_RC( _status, "Create user generator" );
			_isUserOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_AUDIO )
		{
			// Audio
			_audioGen = new xn::AudioGenerator();
			_audioMetaData = new AudioMetaData();
			_status = _audioGen->Create( *_context );
			CHECK_RC( _status, "Create audio generator" );
			_isAudioOn = true;
		}


		if( _imageGen ) _imageGen->GetMetaData( *_imageMetaData );
		if( _irGen )	_irGen->GetMetaData( *_irMetaData );
		if( _depthGen ) _depthGen->GetMetaData( *_depthMetaData );
		if( _userGen ) _userGen->GetUserPixels( 0, *_sceneMetaData );
		if( _audioGen ) _audioGen->GetMetaData( *_audioMetaData );


		if( _isUserOn )
		{
			requestUserCalibration();
		}


		// Allocate mem for bitmaps
		allocate( static_cast<int>(nodeTypeFlags), 640, 480 );


		return true;
	}




	bool OpenNIDevice::initFromXmlFile( const std::string& xmlFile, bool allocUserIfNoNode )
	{
		//_configFile = xmlFile;

		_status = _context->InitFromXmlFile( xmlFile.c_str(), &_errors );
		if( _status != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << "Couldn't create from XML file: " << xmlFile << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			return false;
		}
		//XN_IS_STATUS_OK( _status );

		xn::NodeInfoList list;
		_status = _context->EnumerateExistingNodes( list );
		if( _status == XN_STATUS_OK )
		{
			for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
			{
				std::stringstream ss;
				switch ((*it).GetDescription().Type)
				{
				case XN_NODE_TYPE_DEVICE:
					_device = new xn::Device();
					(*it).GetInstance( *_device );
					ss << "Device: " << _device->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					mDeviceName = _device->GetName();
					break;
				case XN_NODE_TYPE_IMAGE:
					_imageGen = new ImageGenerator();
					_imageMetaData = new ImageMetaData();
					(*it).GetInstance( *_imageGen );
					ss << "Image: " << _imageGen->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					_isImageOn = true;
					break;
				case XN_NODE_TYPE_IR:
					_irGen = new IRGenerator();
					_irMetaData = new IRMetaData();
					(*it).GetInstance( *_irGen );
					ss << "IR: " << _irGen->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					_isIROn = true;
					break;
				case XN_NODE_TYPE_DEPTH:
					_depthGen = new DepthGenerator();
					_depthMetaData = new DepthMetaData();
					(*it).GetInstance( *_depthGen );
					ss << "Depth: " << _depthGen->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					_isDepthOn = true;
					break;
				case XN_NODE_TYPE_USER:
					_userGen = new UserGenerator();
					_sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( *_userGen );
					ss << "User: " << _userGen->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					_isUserOn = true;
					break;
				case XN_NODE_TYPE_AUDIO:
					_audioGen = new AudioGenerator();
					_audioMetaData = new AudioMetaData();
					(*it).GetInstance( *_audioGen );
					ss << "User: " << _audioGen->GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					_isUserOn = true;
					break;
				}
			}
		}


		//
		// In case the config file has no user node, we create one if so we want to
		//
		if( !_userGen && allocUserIfNoNode )
		{
			_userGen = new UserGenerator();
			_sceneMetaData = new xn::SceneMetaData();
			_status = _userGen->Create( *_context );
			CHECK_RC( _status, "Create user generator" );
			_isUserOn = true;
		}


		//
		// First time get meta data
		//
		if( _imageGen ) _imageGen->GetMetaData( *_imageMetaData );
		if( _irGen )	_irGen->GetMetaData( *_irMetaData );
		if( _depthGen ) _depthGen->GetMetaData( *_depthMetaData );
		if( _userGen ) _userGen->GetUserPixels( 0, *_sceneMetaData );
		if( _audioGen ) _audioGen->GetMetaData( *_audioMetaData );



		//
		// Prepare for user detection
		//
		if( _isUserOn )
		{
			requestUserCalibration();
		}


		// Allocate memory for bitmaps
		int flags = 0;
		if( _isImageOn ) flags |= NODE_TYPE_IMAGE;
		if( _isIROn ) flags |= NODE_TYPE_IR;
		if( _isDepthOn ) flags |= NODE_TYPE_DEPTH;
		if( _isUserOn ) flags |= NODE_TYPE_USER;
		if( _isAudioOn ) flags |= NODE_TYPE_AUDIO;

		//allocate( flags, 640, 480 );
		XnMapOutputMode mode;
		if( flags & NODE_TYPE_IMAGE )
		{
			_imageGen->GetMapOutputMode( mode );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
		}
		if( flags & NODE_TYPE_IR )
		{
			_irGen->GetMapOutputMode( mode );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			_irData = new boost::uint16_t[mode.nXRes*mode.nYRes];
			_irData8 = new boost::uint8_t[mode.nXRes*mode.nYRes];
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			_depthGen->GetMapOutputMode( mode );
			_backDepthData = new boost::uint16_t[mode.nXRes*mode.nYRes];
			_depthData = new boost::uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new boost::uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			g_pTexMap = new XnRGB24Pixel[ mode.nXRes * mode.nYRes * sizeof(XnRGB24Pixel) ];
			g_MaxDepth = MAX_DEPTH;
			g_pDepthHist = new float[g_MaxDepth];

		}

		return true;
	}


	void OpenNIDevice::release()
	{
		/*
		// Stop thread
		if( _isRunning && USE_THREAD )
		{
			_isRunning = false;		
			OutputDebugStringA( "Stop running thread\n" );
			assert( _thread );
			_thread->join();
		}*/

		// Delete buffers

		SAFE_DELETE( mColorSurface );
		SAFE_DELETE( mIRSurface );
		SAFE_DELETE( mDepthSurface );

		//SAFE_DELETE_ARRAY( _colorData );
		SAFE_DELETE_ARRAY( _irData );
		SAFE_DELETE_ARRAY( _irData8 );
		SAFE_DELETE_ARRAY( _depthData );
		//SAFE_DELETE_ARRAY( _depthData8 );
		SAFE_DELETE_ARRAY( _depthDataRGB );
		SAFE_DELETE_ARRAY( _backDepthData );
		SAFE_DELETE_ARRAY( g_pTexMap );
		SAFE_DELETE_ARRAY( g_pDepthHist );

		_primaryGen = NULL;	// just null the pointer
		SAFE_DELETE( _device );
		SAFE_DELETE( _imageGen );
		SAFE_DELETE( _irGen );
		SAFE_DELETE( _depthGen );
		SAFE_DELETE( _audioGen );
		SAFE_DELETE( _userGen );
		
		SAFE_DELETE( _imageMetaData );
		SAFE_DELETE( _irMetaData );
		SAFE_DELETE( _depthMetaData );
		SAFE_DELETE( _sceneMetaData );
		SAFE_DELETE( _audioMetaData );

		//SAFE_DELETE( _callback );
	}



	void OpenNIDevice::start()
	{
		_isRunning = false;
		/*if( USE_THREAD ) 
		{
			assert( !_thread );
			_thread = boost::shared_ptr<boost::thread>( new boost::thread(&OpenNIDevice::run, this) );
			_isRunning = true;
		}*/
	}


	void OpenNIDevice::run()
	{
		while( _isRunning )
		{
			update();
		}
	}


	void OpenNIDevice::update()
	{
		readFrame();
	}



	void OpenNIDevice::allocate( int flags, int width, int height )
	{
		if( flags & NODE_TYPE_IMAGE )
		{
			//_colorData = new boost::uint8_t[width*height*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, width, height );
		}
		if( flags & NODE_TYPE_IR )
		{
			_irData = new boost::uint16_t[width*height];
			_irData8 = new boost::uint8_t[width*height];
			mIRSurface = new OpenNISurface8( NODE_TYPE_IR, width, height );
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, width, height );
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			_backDepthData = new boost::uint16_t[width*height];
			_depthData = new boost::uint16_t[width*height];
			//_depthData8 = new boost::uint8_t[width*height];
			_depthDataRGB = new boost::uint8_t[width*height*mBitsPerPixel];
			mDepthSurface = new OpenNISurface16( NODE_TYPE_DEPTH, width, height );
		}
	}




	bool OpenNIDevice::requestUserCalibration()
	{
		XnCallbackHandle hUserCallbacks = 0;
		XnCallbackHandle hCalibrationCallbacks = 0;
		XnCallbackHandle hPoseCallbacks = 0;
		if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
		{
			OutputDebugStringA( "Supplied user generator doesn't support skeleton\n" );
			return false;
		}
		_userGen->RegisterUserCallbacks( Callback_NewUser, Callback_LostUser, NULL, hUserCallbacks );

		_userGen->GetSkeletonCap().RegisterCalibrationCallbacks( Callback_CalibrationStart, Callback_CalibrationEnd, NULL, hCalibrationCallbacks );
		if( _userGen->GetSkeletonCap().NeedPoseForCalibration() )
		{
			g_bNeedPose = TRUE;
			if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION) )
			{
				OutputDebugStringA( "Pose required, but not supported\n" );
				return false;
			}
			_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks( Callback_PoseDetected, NULL, NULL, hPoseCallbacks );
			//_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks( Callback_PoseDetected, Callback_PoseDetectionEnd, NULL, hPoseCallbacks );
			_userGen->GetSkeletonCap().GetCalibrationPose( g_strPose );
			std::stringstream ss;
			ss << "--> User pose name: '" << g_strPose << "'" << std::endl;
			OutputDebugStringA( ss.str().c_str() );
		}
		_userGen->GetSkeletonCap().SetSkeletonProfile( XN_SKEL_PROFILE_ALL );

		return true;
	}


	/**
		Change resolution of a bitmap generator.

		Possible enumerations:
			XN_RES_CUSTOM = 0,
			XN_RES_QQVGA = 1,
			XN_RES_CGA = 2,
			XN_RES_QVGA = 3,
			XN_RES_VGA = 4,
			XN_RES_SVGA = 5,
			XN_RES_XGA = 6,
			XN_RES_720P = 7,
			XN_RES_SXGA = 8,
			XN_RES_UXGA = 9,
			XN_RES_1080P = 10,
	**/
	void OpenNIDevice::setResolution( ProductionNodeType nodeType, int res, int fps )
	{
		MapGenerator* gen = NULL;
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			gen = _imageGen;
			break;
		case NODE_TYPE_IR:
			gen = _irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = _depthGen;
			break;
		default:
			OutputDebugStringA( "Can't change resolution. Not a valid generator" );
			return;
		}

		XnMapOutputMode mode;
		gen->GetMapOutputMode( mode );
		mode.nXRes = Resolution((XnResolution)res).GetXResolution();
		mode.nYRes = Resolution((XnResolution)res).GetYResolution();
		mode.nFPS = fps;
		XnStatus nRetVal = gen->SetMapOutputMode( mode );
		if( nRetVal != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << " Failed to set resolution: " << xnGetStatusString(nRetVal) << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			return;
		}


		// Re-alloc bitmap buffers
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			//SAFE_DELETE_ARRAY( _colorData );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface->remap( mode.nXRes, mode.nYRes );
			break;
		case NODE_TYPE_IR:
			mColorSurface->remap( mode.nXRes, mode.nYRes );
			//SAFE_DELETE_ARRAY( _colorData );
			SAFE_DELETE_ARRAY( _irData );
			SAFE_DELETE_ARRAY( _irData8 );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			_irData = new boost::uint16_t[mode.nXRes*mode.nYRes];
			_irData8 = new boost::uint8_t[mode.nXRes*mode.nYRes];
			break;
		case NODE_TYPE_DEPTH:
			SAFE_DELETE_ARRAY( _depthData );
			//SAFE_DELETE_ARRAY( _depthData8 );
			SAFE_DELETE_ARRAY( _depthDataRGB );
			_depthData = new boost::uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new boost::uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		default:
			OutputDebugStringA( "Can't change bitmap size." );
			return;
		}
	}


	void OpenNIDevice::setFPS( ProductionNodeType nodeType, int fps )
	{
		MapGenerator* gen = NULL;
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			gen = _imageGen;
			break;
		case NODE_TYPE_IR:
			gen = _irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = _depthGen;
			break;
		default:
			OutputDebugStringA( "Can't change resolution. Not a valid generator" );
			return;
		}

		XnMapOutputMode mode;
		gen->GetMapOutputMode( mode );
		mode.nFPS = fps;
		XnStatus nRetVal = gen->SetMapOutputMode( mode );
		if( nRetVal != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << " Failed to set FPS: " << xnGetStatusString(nRetVal) << std::endl;
			OutputDebugStringA( ss.str().c_str() );
		}
	}


	void OpenNIDevice::setMapOutputMode( ProductionNodeType nodeType, int width, int height, int fps )
	{
		MapGenerator* gen = NULL;
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			gen = _imageGen;
			break;
		case NODE_TYPE_IR:
			gen = _irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = _depthGen;
			break;
		default:
			OutputDebugStringA( "Can't change resolution. Not a valid generator" );
			return;
		}

		XnMapOutputMode mode;
		gen->GetMapOutputMode( mode );
		mode.nXRes = width;
		mode.nYRes = height;
		mode.nFPS = fps;
		XnStatus nRetVal = gen->SetMapOutputMode( mode );
		if( nRetVal != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << " Failed to set resolution: " << xnGetStatusString(nRetVal) << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			return;
		}


		// Re-alloc bitmap buffers
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			mColorSurface->remap( mode.nXRes, mode.nYRes );
			//SAFE_DELETE_ARRAY( _colorData );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		case NODE_TYPE_IR:
			mColorSurface->remap( mode.nXRes, mode.nYRes );
			//SAFE_DELETE_ARRAY( _colorData );
			SAFE_DELETE_ARRAY( _irData );
			SAFE_DELETE_ARRAY( _irData8 );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			_irData = new uint16_t[mode.nXRes*mode.nYRes];
			_irData8 = new uint8_t[mode.nXRes*mode.nYRes];
			break;
		case NODE_TYPE_DEPTH:
			SAFE_DELETE_ARRAY( _depthData );
			//SAFE_DELETE_ARRAY( _depthData8 );
			SAFE_DELETE_ARRAY( _depthDataRGB );
			_depthData = new uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new boost::uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		default:
			OutputDebugStringA( "Can't change bitmap size." );
			return;
		}
	}



	void OpenNIDevice::readFrame()
	{
		XnStatus rc = XN_STATUS_OK;

		if( _primaryGen != NULL )
		{
			rc = _context->WaitOneUpdateAll( *_primaryGen );
		}
		else
		{
			rc = _context->WaitAnyUpdateAll();
		}

		if( rc != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << " Error: " << xnGetStatusString(rc) << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			return;
		}

		if( _depthGen && _depthGen->IsValid() )
		{
			_depthGen->GetMetaData( *_depthMetaData );
			pDepth = _depthGen->GetDepthMap();
			// Compute bitmap buffers
			int w = _depthMetaData->XRes();
			int h = _depthMetaData->YRes();
			memcpy( _backDepthData, pDepth, w*h*sizeof(XnDepthPixel) );


			//
			// Calculate the accumulative histogram (the yellow/cyan display...)
			//
			xnOSMemSet( g_pDepthHist, 0, MAX_DEPTH*sizeof(float) );

			// Histogram
			unsigned int nNumberOfPoints = 0;
			for (XnUInt y = 0; y < _depthMetaData->YRes(); ++y)
			{
				for (XnUInt x = 0; x < _depthMetaData->XRes(); ++x, ++pDepth)
				{
					if( *pDepth != 0 )
					{
						g_pDepthHist[ *pDepth ]++;
						nNumberOfPoints++;
					}
				}
			}
			for( int nIndex=1; nIndex<MAX_DEPTH; nIndex++ )
			{
				g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
			}
			if( nNumberOfPoints )
			{
				float invNOP = 1.0f / (float)nNumberOfPoints;
				for( int nIndex=1; nIndex<MAX_DEPTH; nIndex++ )
				{
					g_pDepthHist[nIndex] = static_cast<uint32_t>(256 * (1.0f - (g_pDepthHist[nIndex] * invNOP)));
					//g_pDepthHist[nIndex] = static_cast<uint32_t>(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints)));
				}
			}


			// First, clear buffer memory
			xnOSMemSet( g_pTexMap, 0, w*h*sizeof(XnRGB24Pixel) );
			xnOSMemSet( _depthData, 0, w*h );
			//xnOSMemSet( _depthData8, 0, w*h );

			const XnDepthPixel* pDepthRow = _depthMetaData->Data();
			XnRGB24Pixel* pTexRow = g_pTexMap + _depthMetaData->YOffset() * w;

			int index = 0;
			for (XnUInt y = 0; y < _depthMetaData->YRes(); ++y)
			{
				const XnDepthPixel* pDepth = pDepthRow;
				XnRGB24Pixel* pTex = pTexRow + _depthMetaData->XOffset();

				for (XnUInt x = 0; x < _depthMetaData->XRes(); ++x, ++pDepth, ++pTex)
				{
					if( *pDepth != 0 )
					{
						// Inverted or not ?
						int nHistValue = (_isDepthInverted) ? 255-g_pDepthHist[*pDepth] : g_pDepthHist[*pDepth];

						// Fill or cyan RGB depth map
						pTex->nRed = nHistValue;
						pTex->nGreen = nHistValue;
						pTex->nBlue = nHistValue;

						//// 16bit depth map
						//uint32_t v = *pDepth;
						//if( v < 0 || v > 2047 ) v = 0;
						//uint32_t d = 65535 - ((v*v) >> 4);	// move to 16bit
						//_depthData[index] = d;
						_depthData[index] = (nHistValue<<8);	// shift to 16bit

						// Luminance 8bit depth map
						//_depthData8[index] = nHistValue;
					}
					/*else
					{
						_depthData[index] = 0;
						//_depthData8[index] = 0;
					}*/
					index++;
				}

				pDepthRow += _depthMetaData->XRes();
				pTexRow += w;
			}
			memcpy( _depthDataRGB, g_pTexMap, w*h*sizeof(XnRGB24Pixel) );


			/*for( int i=0; i<w*h; i++ )
			{
				uint32_t v = pDepth[i];
				if( v > 2047 ) v = 0;
				//uint32_t d = 65535 - ((v*v) >> 4);	// move to 16bit
				uint32_t d = (v << 5);	// move to 16bit
				_depthData[i] = d;

				// Convert to 8bit precision
				_depthData8[i] = (_backDepthData[i]>>3)&0xff;
			}*/
		}

		if( _imageGen && _imageGen->IsValid() )
		{
			_imageGen->GetMetaData( *_imageMetaData );
			pImage = _imageGen->GetImageMap();
			// Compute bitmap buffers
			mColorSurface->update( (uint8_t*)(pImage) );
			//int w = _depthMetaData->XRes();
			//int h = _depthMetaData->YRes();
			//memcpy( mColorSurface->getData(), pImage, w*h*mBitsPerPixel*sizeof(XnUInt8) );
		}

		if( _irGen && _irGen->IsValid() )
		{
			_irGen->GetMetaData( *_irMetaData );
			pIR = _irMetaData->Data();	//_irGen->GetIRMap();
			// Compute bitmap buffers
			int w = _irMetaData->XRes();
			int h = _irMetaData->YRes();
			memcpy( _irData, pIR, w*h*sizeof(XnIRPixel) );
			int index = 0;
			for( int i=0; i<w*h; i++ )
			{
				uint32_t v = _irData[i];
				//uint16_t d = 65535 - ((v*v) >> 4);
				int32_t d = v << 5;	// move to 16bit
				_irData[i] = d;

				// Convert to 8bit precision (luminance)
				_irData8[i] = (v>>3)&0xff;

				// rgb grayscale image (8bit)
				if( v > 255 ) v = 255;
				mColorSurface->getData()[index++] = v;
				mColorSurface->getData()[index++] = v;
				mColorSurface->getData()[index++] = v;

			}
		}

		if( _audioGen && _audioGen->IsValid() )
		{
			_audioGen->GetMetaData( *_audioMetaData );
		}
	}


	void OpenNIDevice::setPrimaryBuffer( int type )
	{ 
		if( _imageGen && (type & NODE_TYPE_IMAGE) )	
			_primaryGen = _imageGen;
		else if( _irGen && (type & NODE_TYPE_IR) )	
			_primaryGen = _irGen;
		else if( _depthGen && (type & NODE_TYPE_DEPTH) )	
			_primaryGen = _depthGen;
		else if( _userGen && (type & NODE_TYPE_USER) )	
			_primaryGen = _userGen;
		else if( _audioGen && (type & NODE_TYPE_AUDIO) )	
			_primaryGen = _audioGen;
		else 
			_primaryGen = NULL;
	}


	void OpenNIDevice::setDepthInvert( bool flag )
	{
		_isDepthInverted = flag;
	}

	boost::uint8_t* OpenNIDevice::getColorMap()
	{
		return mColorSurface->getData();
	}

	boost::uint16_t* OpenNIDevice::getIRMap()
	{
		return _irData;
	}

	boost::uint8_t* OpenNIDevice::getIRMap8i()
	{
		return _irData8;
	}


	boost::uint16_t* OpenNIDevice::getRawDepthMap()
	{
		return _backDepthData;
	}


	boost::uint16_t* OpenNIDevice::getDepthMap()
	{
		return _depthData;
	}

	//boost::uint8_t* OpenNIDevice::getDepthMap8i()
	//{
	//	return _depthData8;
	//}

	boost::uint8_t* OpenNIDevice::getDepthMap24()
	{
		return _depthDataRGB;
	}




	/************************************************************************/
	/* Device Manager
	/************************************************************************/

	const bool OpenNIDeviceManager::USE_THREAD = true;

	//std::shared_ptr<OpenNIDeviceManager> OpenNIDeviceManager::_singletonPointerRef;
	OpenNIDeviceManager OpenNIDeviceManager::_singletonPointer;
	//OpenNIDeviceManager* OpenNIDeviceManager::_singletonPointer = NULL;
	//std::list<OpenNIDeviceManager::DeviceInfo*> OpenNIDeviceManager::mDeviceList;


	OpenNIDeviceManager::OpenNIDeviceManager()
	{
		//assert( !_singletonPointer && "Tried to create multiple instances of a singleton class!" );
		//_singletonPointer = (OpenNIDeviceManager*)this;

		/*XnStatus status = _context.Init();
		if( status != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << "Couldn't create context" << std::endl;
			OutputDebugStringA( ss.str().c_str() );
		}*/

		_isRunning = false;
		_idCount = 0;
		mDebugInfo = "No debug information\n";

		_context1 = NULL;
	}


	OpenNIDeviceManager::~OpenNIDeviceManager()
	{
		destroyAll();
		//_singletonPointer = NULL;
	}


	uint32_t OpenNIDeviceManager::enumDevices( void )
	{
		int count = 0;


		//
		// Find about plugged-in devices
		//
		xn::NodeInfoList list;
		XnStatus status = _context.EnumerateExistingNodes( list );
		if( status == XN_STATUS_OK )
		{
			for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
			{
				std::stringstream ss;
				switch ((*it).GetDescription().Type)
				{
				case XN_NODE_TYPE_DEVICE:
					xn::ProductionNode device;
					(*it).GetInstance( device );
					ss << "Device: " << device.GetName() << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					count++;
					break;
				}
			}
		}


		return count;
	}


	OpenNIDevice* OpenNIDeviceManager::createDevice( const std::string& xmlFile, bool allocUserIfNoNode )
	{
		if( mDeviceList.size() >= MAX_DEVICES ) return NULL;

		if( xmlFile == "" )
		{
			OutputDebugStringA( "not implemented" );
			return NULL;
		}
		else
		{
			/*DeviceInfo devinfo;
			devinfo.dev = new OpenNIDevice( &_context );
			devinfo.dev->initFromXmlFile( xmlFile, allocUserIfNoNode );	
			devinfo.id = mDeviceList.size();
			mDeviceList.push_back( devinfo );
			return devinfo.dev;*/

			DeviceInfo* devinfo = new DeviceInfo;
			devinfo->dev = new OpenNIDevice( &_context );
			devinfo->dev->initFromXmlFile( xmlFile, allocUserIfNoNode );	
			devinfo->id = mDeviceList.size();
			mDeviceList.push_back( devinfo );
			return devinfo->dev;
}
		return NULL;
	}

	OpenNIDevice* OpenNIDeviceManager::createDevice2( const std::string& xmlFile, bool allocUserIfNoNode )
	{
		if( mDeviceList.size() >= MAX_DEVICES ) return NULL;

		_context1 = new xn::Context();

		if( xmlFile == "" )
		{
			OutputDebugStringA( "not implemented" );
			return NULL;
		}
		else
		{
			/*DeviceInfo devinfo;
			devinfo.dev = new OpenNIDevice( _context1 );
			devinfo.dev->initFromXmlFile( xmlFile, allocUserIfNoNode );	
			devinfo.id = mDeviceList.size();
			mDeviceList.push_back( devinfo );
			return devinfo.dev;*/

			DeviceInfo* devinfo = new DeviceInfo;
			devinfo->dev = new OpenNIDevice( &_context );
			devinfo->dev->initFromXmlFile( xmlFile, allocUserIfNoNode );	
			devinfo->id = mDeviceList.size();
			mDeviceList.push_back( devinfo );
			return devinfo->dev;
}
		return NULL;
	}


	OpenNIDevice* OpenNIDeviceManager::createDevice( int nodeTypeFlags )
	{
		if( mDeviceList.size() >= MAX_DEVICES ) return NULL;

		/*DeviceInfo devinfo;
		devinfo.dev = new OpenNIDevice( &_context );
		devinfo.dev->init( nodeTypeFlags );
		devinfo.id = mDeviceList.size();
		mDeviceList.push_back( devinfo );
		return devinfo.dev;*/

		DeviceInfo* devinfo = new DeviceInfo;
		devinfo->dev = new OpenNIDevice( &_context );
		devinfo->dev->init( nodeTypeFlags );
		devinfo->id = mDeviceList.size();
		mDeviceList.push_back( devinfo );
		return devinfo->dev;
	}


	void OpenNIDeviceManager::destroyDevice( OpenNIDevice* device )
	{
		for ( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		//for ( std::list<DeviceInfo>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{		
			if ( device == (*it)->dev )
			{
				//mDeviceList.remove( *it );
				DeviceInfo* devinfo = *it;
				//destroy_device( devinfo->id );
				//devinfo->dev->release();
				SAFE_DELETE( devinfo->dev );
				//SAFE_DELETE( devinfo );
				//mDeviceList.erase( it );

				return;
			}
		}
	}


	void OpenNIDeviceManager::destroyAll( void )
	{
		_isRunning = false;

		// Stop thread
		if( USE_THREAD )
		{
			OutputDebugStringA( "Stop running thread on device manager\n" );
			assert( _thread );
			_thread->join();
		}


		// Delete device list
		for( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		//for ( std::list<DeviceInfo>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{		
			//destroy_device( (*it).id );
			//(*it).dev->release();
			SAFE_DELETE( (*it)->dev );
			SAFE_DELETE( *it );
			//mDeviceList.remove( *it );
			//mDeviceList.erase( it );
		}
		mDeviceList.clear();


		// Now delete users
		/*for( std::list<OpenNIUser>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			//SAFE_DELETE( *it );
			//mUserList.remove( *it );
			//mUserList.erase( it );
		}*/
		mUserList.clear();



		if( _context1 ) _context1->Shutdown();
		SAFE_DELETE( _context1 );

		//
		// Shutdown openNI context
		//
		_context.Shutdown();
	}



	OpenNIUser* OpenNIDeviceManager::addUser( xn::UserGenerator* userGen, uint32_t id )
	{
		// TODO! associate user generator to its current device
		// for the moment we always pick first device available
		std::list<DeviceInfo*>::iterator it = mDeviceList.begin();
		//std::list<DeviceInfo>::iterator it = mDeviceList.begin();

		OpenNIUser* newUser = new OpenNIUser( id, (*it)->dev );
		mUserList.push_back( *newUser );
		return newUser;

		/*OpenNIUser* newUser = new OpenNIUser( id, (*it).dev );
		mUserList.push_back( newUser );
		return newUser;*/
	}

	void OpenNIDeviceManager::removeUser( OpenNIUser* user )
	{
		if( mUserList.size() <= 0 ) return;
		//mUserList.remove( user );
		//SAFE_DELETE( user );
	}

	void OpenNIDeviceManager::removeUser( uint32_t id )
	{
		if( mUserList.size() <= 0 ) return;
		for ( std::list<OpenNIUser>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it).getId() )
			{
				mUserList.erase( it );
				//mUserList.remove( *it );
				//SAFE_DELETE( *it );
				return;
			}
		}
	}



	void OpenNIDeviceManager::run()
	{
		while( _isRunning )
		{
			update();
		}
	}


	void OpenNIDeviceManager::start()
	{
		_isRunning = false;
		if( USE_THREAD ) 
		{
			OutputDebugStringA( "Starting thread on device manager" );
			assert( !_thread );
			_thread = boost::shared_ptr<boost::thread>( new boost::thread(&OpenNIDeviceManager::run, this) );
			_isRunning = true;
		}

		// Start all devices
		for ( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		//for ( std::list<DeviceInfo>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{
			if( (*it)->dev ) 
				(*it)->dev->start();
		}

		// Start generation
		_context.StartGeneratingAll();
	}



	void OpenNIDeviceManager::update()
	{
		boost::lock_guard<boost::recursive_mutex> lock( _mutex );

		if( !_isRunning ) return;

		// Handle device update
		for( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		//for( std::list<DeviceInfo>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{
			if( (*it)->dev )
				(*it)->dev->readFrame();
		}
		// Handle user update
		if( mUserList.size() > 0 )
		{
			for( std::list<OpenNIUser>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
			{
				(*it).update();
			}
		}
	}


	void OpenNIDeviceManager::renderJoints( float pointSize )
	{
		for( std::list<OpenNIUser>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			(*it).renderJoints( pointSize );
		}
	}



	OpenNIUser* OpenNIDeviceManager::getUser( int id )
	{
		if( mUserList.size() == 0 ) return NULL;
		for( std::list<OpenNIUser>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it).getId() )
				return &(*it);
		}
		return NULL;
	}

	/*void OpenNIDeviceManager::DeviceJoin( int deviceId )
	{
		for( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{
			if( deviceId == (*it)->id )
				( *it )->dev->OnJoin();
		}

	}

	void OpenNIDeviceManager::DeviceLeave( int deviceId )
	{
		for( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
		{
			if( deviceId == (*it)->id )
				( *it )->dev->OnLeave();
		}
	}*/

}