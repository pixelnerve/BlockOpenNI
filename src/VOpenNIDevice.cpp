#include "VOpenNICommon.h"
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <direct.h>
#endif
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


	// Callback: New user was detected
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_NewUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "New User: '" << nId << "'" << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		generator.GetPoseDetectionCap().StartPoseDetection( g_strPose, nId );


		// Add new user
		//if( !device->getUser(nId) )
			//device->addUser( new OpenNIUser(nId, device) );

		OpenNIDeviceManager::Instance().setText( ss.str() );
		if( !OpenNIDeviceManager::Instance().getUser(nId) )
			OpenNIDeviceManager::Instance().addUser( &generator, nId );
	}

	// Callback: An existing user was lost
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_LostUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Lost User: '" << nId << "'" << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Remove user
		//if( device->getUser(nId) )
			//device->removeUser( nId );

		OpenNIDeviceManager::Instance().setText( ss.str() );
		if( OpenNIDeviceManager::Instance().getUser(nId) )
			OpenNIDeviceManager::Instance().removeUser( nId );
	}

	// Callback: Detected a pose
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseDetected( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Pose detection start: '" << strPose << "' on User: " << nId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		if( OpenNIDeviceManager::Instance().getUser(nId) ) 
		{
			OpenNIDeviceManager::Instance().setText( ss.str() );

			// Pose found! Now we need skeleton calibration.
			//capability.StopPoseDetection( nId );
			device->getUserGenerator()->GetPoseDetectionCap().StopPoseDetection( nId );
			device->getUserGenerator()->GetSkeletonCap().RequestCalibration( nId, TRUE );
		}
	}
	// Callback: When the user is out of pose
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseDetectionEnd( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Pose detection end. User: '" << strPose << "'. Id: " << nId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDeviceManager::Instance().setText( ss.str() );
	}

	// Callback: Started calibration
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_CalibrationStart( xn::SkeletonCapability& capability, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Calibration started for user " << nId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDeviceManager::Instance().setText( ss.str() );
	}
	// Callback: Finished calibration
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_CalibrationEnd( xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie )
	{
		if( bSuccess )
		{
			// Calibration succeeded
			std::stringstream ss;
			ss << "Calibration complete, start tracking user " << nId << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			OpenNIDeviceManager::Instance().setText( ss.str() );

			OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

			device->getUserGenerator()->GetSkeletonCap().StartTracking( nId );
		}
		else
		{
			// Calibration failed
			std::stringstream ss;
			ss << "Calibration failed for user " << nId << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );

			OpenNIDeviceManager::Instance().setText( ss.str() );

			OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);
			
			if( g_bNeedPose )
			{
				device->getUserGenerator()->GetPoseDetectionCap().StartPoseDetection( g_strPose, nId );
			}
			else
			{
				device->getUserGenerator()->GetSkeletonCap().RequestCalibration( nId, TRUE );
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

		mNearClipPlane = 0;
		mFarClipPlane = 10000;

		//_callback = NULL;
		//_callback = new OpenNIDeviceCallback( this, &OpenNIDevice::CallbackFunc );
	}


	OpenNIDevice::~OpenNIDevice()
	{
		_context = NULL;
		release();
	}


	bool OpenNIDevice::init( uint64_t nodeTypeFlags )
	{
		_status = _context->Init();
		if( _status != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << "Couldn't create context" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			return false;
		}

/*
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
					DEBUG_MESSAGE( ss.str().c_str() );
					mDeviceName = _device->GetName();
					break;
				}
			}
		}*/



		// Pick image or IR map
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			_imageGen = new xn::ImageGenerator();
			_imageMetaData = new xn::ImageMetaData();
			_status = _imageGen->Create( *_context );	//, NULL, &_errors );
			CHECK_RC( _status, "Create image generator" );
			_isImageOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			_irGen = new xn::IRGenerator();
			_irMetaData = new xn::IRMetaData();
			_status = _irGen->Create( *_context );	//, NULL, &_errors );
			CHECK_RC( _status, "Create ir generator" );
			_isIROn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			// Depth map
			_depthGen = new xn::DepthGenerator();
			_depthMetaData = new xn::DepthMetaData();
			_status = _depthGen->Create( *_context );	//, NULL, &_errors );
			CHECK_RC( _status, "Create depth generator" );
			_isDepthOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			// User
			_userGen = new xn::UserGenerator();
			_sceneMetaData = new xn::SceneMetaData();
			_status = _context->FindExistingNode( XN_NODE_TYPE_USER, *_userGen );
			if( _status != XN_STATUS_OK ) {
				_status = _userGen->Create( *_context );	//, NULL, &_errors );
				CHECK_RC( _status, "Create user generator" );
			}
			_isUserOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_AUDIO )
		{
			// Audio
			_audioGen = new xn::AudioGenerator();
			_audioMetaData = new AudioMetaData();
			_status = _audioGen->Create( *_context );	//, NULL, &_errors );
			CHECK_RC( _status, "Create audio generator" );
			_isAudioOn = true;
		}


		if( _imageGen ) {
			setResolution( NODE_TYPE_IMAGE, XN_RES_VGA, 30 );
			_imageGen->GetMetaData( *_imageMetaData );
		}
		if( _irGen )	{
			setResolution( NODE_TYPE_IR, XN_RES_VGA, 30 );
			_irGen->GetMetaData( *_irMetaData );
		}
		if( _depthGen ) {
			setResolution( NODE_TYPE_DEPTH, XN_RES_VGA, 30 );
			_depthGen->GetMetaData( *_depthMetaData );
		}
		//if( _userGen ) {
		//	_userGen->GetUserPixels( 0, *_sceneMetaData );
		//}
		if( _audioGen ) {
			_audioGen->GetMetaData( *_audioMetaData );
		}


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

			ss << "(Device)  Error!  " << xnGetStatusName(_status) << std::endl;
			//ss << "(Device)  Couldn't create from XML file: " << xmlFile << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			return false;
		}
		//XN_IS_STATUS_OK( _status );


		xn::NodeInfoList list;
		_status = _context->EnumerateExistingNodes( list );
		if( _status == XN_STATUS_OK )
		{
			for( NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it )
			{
				std::stringstream ss;
				switch ((*it).GetDescription().Type)
				{
				case XN_NODE_TYPE_DEVICE:
					_device = new xn::Device();
					(*it).GetInstance( *_device );
					ss << "Device: " << _device->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					mDeviceName = _device->GetName();
					break;
				case XN_NODE_TYPE_IMAGE:
					_imageGen = new ImageGenerator();
					_imageMetaData = new ImageMetaData();
					(*it).GetInstance( *_imageGen );
					ss << "Image: " << _imageGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isImageOn = true;
					break;
				case XN_NODE_TYPE_IR:
					_irGen = new IRGenerator();
					_irMetaData = new IRMetaData();
					(*it).GetInstance( *_irGen );
					ss << "IR: " << _irGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isIROn = true;
					break;
				case XN_NODE_TYPE_DEPTH:
					_depthGen = new DepthGenerator();
					_depthMetaData = new DepthMetaData();
					(*it).GetInstance( *_depthGen );
					ss << "Depth: " << _depthGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isDepthOn = true;
					break;
				case XN_NODE_TYPE_USER:
					_userGen = new UserGenerator();
					_sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( *_userGen );
					ss << "User: " << _userGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isUserOn = true;
					break;
				case XN_NODE_TYPE_AUDIO:
					_audioGen = new AudioGenerator();
					_audioMetaData = new AudioMetaData();
					(*it).GetInstance( *_audioGen );
					ss << "User: " << _audioGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
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


		//
		// First time get meta data
		//
		if( _imageGen ) _imageGen->GetMetaData( *_imageMetaData );
		if( _irGen )	_irGen->GetMetaData( *_irMetaData );
		if( _depthGen ) _depthGen->GetMetaData( *_depthMetaData );
		//if( _userGen ) _userGen->GetUserPixels( 0, *_sceneMetaData );
		if( _audioGen ) _audioGen->GetMetaData( *_audioMetaData );


		return true;
	}


	void OpenNIDevice::release()
	{
		/*
		// Stop thread
		if( _isRunning && USE_THREAD )
		{
			_isRunning = false;		
			DEBUG_MESSAGE( "Stop running thread\n" );
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
			readFrame();	//update();
		}
	}



	void OpenNIDevice::allocate( uint64_t flags, uint32_t width, uint32_t height )
	{
		if( flags & NODE_TYPE_IMAGE )
		{
			//_colorData = new boost::uint8_t[width*height*mBitsPerPixel];
			if( !mColorSurface ) mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, width, height );
		}
		if( flags & NODE_TYPE_IR )
		{
			if( !_irData ) _irData = new boost::uint16_t[width*height];
			if( !_irData8 ) _irData8 = new boost::uint8_t[width*height];
			if( !mIRSurface ) mIRSurface = new OpenNISurface8( NODE_TYPE_IR, width, height );
			if( !mColorSurface ) mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, width, height );
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			g_MaxDepth = MAX_DEPTH;
			if( !_backDepthData ) _backDepthData = new boost::uint16_t[width*height];
			if( !_depthData ) _depthData = new boost::uint16_t[width*height];
			//_depthData8 = new boost::uint8_t[width*height];
			if( !_depthDataRGB ) _depthDataRGB = new boost::uint8_t[width*height*mBitsPerPixel];
			if( !mDepthSurface ) mDepthSurface = new OpenNISurface16( NODE_TYPE_DEPTH, width, height );
			if( !g_pTexMap ) g_pTexMap = new XnRGB24Pixel[width*height*sizeof(XnRGB24Pixel)];
			if( !g_pDepthHist ) g_pDepthHist = new float[g_MaxDepth];
		}
	}




	bool OpenNIDevice::requestUserCalibration()
	{
		XnCallbackHandle hUserCallbacks = 0;
		XnCallbackHandle hCalibrationCallbacks = 0;
		XnCallbackHandle hPoseCallbacks = 0;
		if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
		{
			DEBUG_MESSAGE( "Supplied user generator doesn't support skeleton\n" );
			return false;
		}
		_userGen->RegisterUserCallbacks( &V::OpenNIDevice::Callback_NewUser, &V::OpenNIDevice::Callback_LostUser, this, hUserCallbacks );
		

		_userGen->GetSkeletonCap().RegisterCalibrationCallbacks( &V::OpenNIDevice::Callback_CalibrationStart, &V::OpenNIDevice::Callback_CalibrationEnd, this, hCalibrationCallbacks );
		if( _userGen->GetSkeletonCap().NeedPoseForCalibration() )
		{
			g_bNeedPose = TRUE;
			if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION) )
			{
				DEBUG_MESSAGE( "Pose required, but not supported\n" );
				return false;
			}
			//_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks( &V::OpenNIDevice::Callback_PoseDetected, NULL, this, hPoseCallbacks );
			_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks( &V::OpenNIDevice::Callback_PoseDetected, &V::OpenNIDevice::Callback_PoseDetectionEnd, this, hPoseCallbacks );
			_userGen->GetSkeletonCap().GetCalibrationPose( g_strPose );
			std::stringstream ss;
			ss << "--> User pose name: '" << g_strPose << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
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
			DEBUG_MESSAGE( "Can't change resolution. Not a valid generator" );
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
			DEBUG_MESSAGE( ss.str().c_str() );
			return;
		}


		// Re-alloc bitmap buffers
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			//SAFE_DELETE_ARRAY( _colorData );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			if( !mColorSurface ) mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			else mColorSurface->remap( mode.nXRes, mode.nYRes );
			break;
		case NODE_TYPE_IR:
			if( !mColorSurface ) mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			else mColorSurface->remap( mode.nXRes, mode.nYRes );
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
			SAFE_DELETE_ARRAY( g_pTexMap );
			_depthData = new uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			g_pTexMap = new XnRGB24Pixel[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		default:
			DEBUG_MESSAGE( "Can't change bitmap size." );
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
			DEBUG_MESSAGE( "Can't change resolution. Not a valid generator" );
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
			DEBUG_MESSAGE( ss.str().c_str() );
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
			DEBUG_MESSAGE( "Can't change resolution. Not a valid generator" );
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
			DEBUG_MESSAGE( ss.str().c_str() );
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
			DEBUG_MESSAGE( "Can't change bitmap size." );
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
			DEBUG_MESSAGE( ss.str().c_str() );
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
			calculateHistogram();



			// First, clear bitmap data
			xnOSMemSet( g_pTexMap, 0, w*h*sizeof(XnRGB24Pixel) );
			//xnOSMemSet( _depthData, 0, w*h );
			//xnOSMemSet( _depthData8, 0, w*h );

			const XnDepthPixel* pDepthRow = _depthMetaData->Data();
			XnRGB24Pixel* pTexRow = g_pTexMap + _depthMetaData->YOffset() * w;

			int index = 0;
			for( XnUInt y = 0; y < _depthMetaData->YRes(); y++ )
			{
				const XnDepthPixel* pDepth = pDepthRow;
				XnRGB24Pixel* pTex = pTexRow + _depthMetaData->XOffset();

				for( XnUInt x = 0; x < _depthMetaData->XRes(); ++x, ++pDepth, ++pTex )
				{
					XnDepthPixel pixel = *pDepth;
					if( pixel > mNearClipPlane && pixel < mFarClipPlane )	//pixel != 0 )
					{
						// Inverted or not ?
						uint32_t nHistValue = (_isDepthInverted) ? static_cast<uint32_t>(-g_pDepthHist[*pDepth]) : static_cast<uint32_t>(g_pDepthHist[*pDepth]);

						// Fill or cyan RGB depth map
						pTex->nRed = nHistValue;
						pTex->nGreen = nHistValue;
						pTex->nBlue = nHistValue;

						//// 16bit depth map
						//_depthData[index] = *pDepth;
						//_depthData[index] <<= 5;
						_depthData[index] = (*pDepth>0 && *pDepth<MAX_DEPTH)?*pDepth:0;
						_depthData[index] <<= 5;
						//_depthData[index] = (nHistValue*nHistValue);	// shift to 16bit
						//_depthData[index] = nHistValue << 8;

						// Luminance 8bit depth map
						//_depthData8[index] = nHistValue;
					}
					else
					{
						pTex->nRed = 0;
						pTex->nGreen = 0;
						pTex->nBlue = 0;
						_depthData[index] = 0;
						//_depthData8[index] = 0;
					}
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

			// Save original data
			XnUInt32 nDataSize = _irGen->GetDataSize();
			memcpy( _irData, pIR, nDataSize );

			// Compute 8bit and RGB color bitmap
			int index = 0;
			uint8_t* pArray = mColorSurface->getData();
			for( uint32_t i=0; i<nDataSize/sizeof(XnIRPixel); i++ )
			{
				uint32_t v = _irData[i];
				//uint16_t d = 65535 - ((v*v) >> 4);
				int32_t d = v << 5;	// move to 16bit
				_irData[i] = d;

				// Convert to 8bit precision (luminance)
				_irData8[i] = (v>>3)&0xff;

				// rgb grayscale image (8bit)
				v = (v<255)?v:255;
				*pArray++ = v;
				*pArray++ = v;
				*pArray++ = v;
			}
		}

		if( _audioGen && _audioGen->IsValid() )
		{
			_audioGen->GetMetaData( *_audioMetaData );
		}
	}


	void OpenNIDevice::calculateHistogram()
	{
		if( _depthGen == NULL)	
			return;

		xnOSMemSet( g_pDepthHist, 0, MAX_DEPTH*sizeof(float) );
		int nNumberOfPoints = 0;

		XnUInt32 nDataSize = _depthGen->GetDataSize();
		const XnDepthPixel* pDepth = _depthGen->GetDepthMap();
		const XnDepthPixel* pDepthEnd = pDepth + (nDataSize / sizeof(XnDepthPixel));

		XnDepthPixel nValue;
		while( pDepth != pDepthEnd )
		{
			nValue = *pDepth;
			XN_ASSERT( nValue <= MAX_DEPTH );

			if( nValue != 0 )
			{
				g_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}

			pDepth++;
		}

		float invNOP = 1.0f / static_cast<float>(nNumberOfPoints);
		XnUInt32 nIndex;
		for( nIndex=1; nIndex<MAX_DEPTH; nIndex++ )
		{
			g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
		}
		for( nIndex=1; nIndex<MAX_DEPTH; nIndex++ )
		{
			if( g_pDepthHist[nIndex] != 0 )
			{
				// White is near, Black is far
				//g_pDepthHist[nIndex] = static_cast<uint32_t>(256 * (1.0f - (g_pDepthHist[nIndex] * invNOP)));
				g_pDepthHist[nIndex] = (nNumberOfPoints-g_pDepthHist[nIndex]) * invNOP * 256;	// / nNumberOfPoints;
			}
		}
	}


	void OpenNIDevice::setLimits( int nearClip, int farClip )
	{
		mNearClipPlane = nearClip;
		mFarClipPlane = farClip;
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


	void OpenNIDevice::addUser( OpenNIUser* user )
	{
		mUserList.push_back( boost::shared_ptr<OpenNIUser>(user) );
	}

	OpenNIUser::Ref OpenNIDevice::getUser( uint32_t id )
	{
		if( mUserList.size() == 0 ) return OpenNIUser::Ref();
		for( std::list< boost::shared_ptr<OpenNIUser> >::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
				return (*it);
		}
		return OpenNIUser::Ref();
	}

	void OpenNIDevice::removeUser( uint32_t id )
	{
		for( std::list< boost::shared_ptr<OpenNIUser> >::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
			{
				mUserList.remove( *it );
				(*it).reset();
				return;
			}
		}

	}









	/************************************************************************/
	/* Device Manager
	/************************************************************************/

	const bool OpenNIDeviceManager::USE_THREAD = true;
	OpenNIDeviceManager OpenNIDeviceManager::_singletonPointer;


	OpenNIDeviceManager::OpenNIDeviceManager()
	{
		_isRunning = false;
		_idCount = 0;
		mDebugInfo = "No debug information\n";
	}


	OpenNIDeviceManager::~OpenNIDeviceManager()
	{
		destroyAll();
	}


	uint32_t OpenNIDeviceManager::enumDevices( void )
	{
		return -1;
	}


	OpenNIDevice* OpenNIDeviceManager::createDevice__( const std::string& xmlFile, bool allocUserIfNoNode )
	{
		if( mDevices.size() >= MAX_DEVICES ) return NULL;

		if( xmlFile == "" )
		{
			DEBUG_MESSAGE( "not implemented" );
			return NULL;
		}
		else
		{
			boost::shared_ptr<OpenNIDevice> dev = boost::shared_ptr<OpenNIDevice>( new OpenNIDevice(&_context) );
			if( !dev->initFromXmlFile( xmlFile, allocUserIfNoNode ) ) 
				return NULL;
			mDevices.push_back( dev );
			return dev.get();
		}

		return NULL;
	}


	OpenNIDevice* OpenNIDeviceManager::createDevice__( int nodeTypeFlags )
	{
		if( mDevices.size() >= MAX_DEVICES ) return NULL;

		boost::shared_ptr<OpenNIDevice> dev = boost::shared_ptr<OpenNIDevice>( new OpenNIDevice(&_context) );
		if( !dev->init( nodeTypeFlags ) ) 
			return NULL;
		mDevices.push_back( dev );
		return dev.get();
	}

	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( const std::string& xmlFile/*=""*/, bool allocUserIfNoNode/*=false */ )
	{
		if( mDevices.size() >= MAX_DEVICES ) return boost::shared_ptr<OpenNIDevice>();

		// Bail out if its an empty filename
		if( xmlFile == "" )
		{
			DEBUG_MESSAGE( "not implemented" );
			return boost::shared_ptr<OpenNIDevice>();
		}

		// Local copy of the filename
		std::string path = xmlFile;

		#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		char AppAbsPath[1024];
		path = "/" + path;
		path = _getcwd( AppAbsPath, 1024 ) + path;
		//DEBUG_MESSAGE( path.c_str() );
		#else
		char AppAbsPath[1024];
		path = "/" + path;
		path = getcwd( AppAbsPath, 1024 ) + path;
		//DEBUG_MESSAGE( path.c_str() );
		#endif

		// Initialize device
		boost::shared_ptr<OpenNIDevice> dev = boost::shared_ptr<OpenNIDevice>( new OpenNIDevice(&_context) );
		if( !dev->initFromXmlFile( path, allocUserIfNoNode ) ) 
		{
			DEBUG_MESSAGE( "[OpenNIDeviceManager]  Couldn't create device from xml\n" );
			return boost::shared_ptr<OpenNIDevice>();
		}
		// By default set depth as primary generator
		dev->setPrimaryBuffer( V::NODE_TYPE_DEPTH );

		// Save device to our list
		mDevices.push_back( dev );
		return dev;
	}

	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( int nodeTypeFlags )
	{
		if( mDevices.size() >= MAX_DEVICES ) 
			return boost::shared_ptr<OpenNIDevice>();

		boost::shared_ptr<OpenNIDevice> dev = boost::shared_ptr<OpenNIDevice>( new OpenNIDevice(&_context) );
		if( !dev->init( nodeTypeFlags ) ) 
		{
			DEBUG_MESSAGE( "[OpenNIDeviceManager]  Couldn't create device manually\n" );
			return boost::shared_ptr<OpenNIDevice>();
		}
		// By default set depth as primary generator
		dev->setPrimaryBuffer( V::NODE_TYPE_DEPTH );
		mDevices.push_back( dev );
		return dev;
	}

	//void OpenNIDeviceManager::destroyDevice( OpenNIDevice* device )
	//{
	//	for ( std::list<boost::shared_ptr<OpenNIDevice>>::iterator it = mDevices.begin(); it != mDevices.end(); it++ )
	//	{
	//		mDevices.remove( *it );
	//	}
	//	mDevices.erase( device );

	//	/*for ( std::list<DeviceInfo*>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
	//	//for ( std::list<DeviceInfo>::iterator it = mDeviceList.begin(); it != mDeviceList.end(); it++ )
	//	{		
	//		if ( device == (*it)->dev )
	//		{
	//			mDeviceList.remove( *it );
	//			//destroy_device( devinfo->id );
	//			SAFE_DELETE( (*it) );
	//			return;
	//		}
	//	}*/
	//}


	void OpenNIDeviceManager::destroyAll( void )
	{
		// Stop thread
		if( USE_THREAD && _isRunning )
		{
			_isRunning = false;
			DEBUG_MESSAGE( "Stop running thread on device manager\n" );
			assert( _thread );
			_thread->join();
		}

		// Delete device list
		mDevices.clear();

		// Now delete user list
		//for( std::list<OpenNIUser*>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		//{
		//	SAFE_DELETE( *it );
		//}
		mUserList.clear();



		//
		// Shutdown openNI context
		//
		_context.Shutdown();
	}



	OpenNIUserRef OpenNIDeviceManager::addUser( xn::UserGenerator* userGen, uint32_t id )
	{
		std::list< boost::shared_ptr<OpenNIDevice> >::iterator it = mDevices.begin();

		OpenNIUserRef newUser = OpenNIUserRef( new OpenNIUser(id, (*it).get()) );
		mUserList.push_back( OpenNIUserRef(newUser) );
		return newUser;
	}

	void OpenNIDeviceManager::removeUser( uint32_t id )
	{
		if( mUserList.size() == 0 ) return;
		for ( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		//for ( std::list<OpenNIUser*>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
			{
				mUserList.remove( *it );
				return;
			}
		}
	}


	OpenNIUserRef OpenNIDeviceManager::getUser( int id )
	{
		if( mUserList.size() == 0 ) return OpenNIUserRef();
		for ( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		//for( std::list<OpenNIUser*>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
				return (*it);
		}
		return OpenNIUserRef();
	}



	bool OpenNIDeviceManager::hasUser( int32_t id )
	{
		if( mUserList.size() == 0 ) return false;
		for ( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
				return true;
		}
		return false;
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
			DEBUG_MESSAGE( "Starting thread on device manager\n" );
			assert( !_thread );
			_thread = boost::shared_ptr<boost::thread>( new boost::thread(&OpenNIDeviceManager::run, this) );
			_isRunning = true;
		}

		// Start all devices
		for ( std::list< boost::shared_ptr<OpenNIDevice> >::iterator it = mDevices.begin(); it != mDevices.end(); it++ )
		{
			(*it)->start();
		}

		// Start generators
		_context.StartGeneratingAll();
	}



	void OpenNIDeviceManager::update()
	{
		boost::lock_guard<boost::recursive_mutex> lock( _mutex );

		if( !_isRunning ) return;

		// Handle device update
		for ( std::list< boost::shared_ptr<OpenNIDevice> >::iterator it = mDevices.begin(); it != mDevices.end(); it++ )
		{
			(*it)->readFrame();
		}

		// Handle user update
		if( mUserList.size() > 0 )
		{
			for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
			//for( std::list<OpenNIUser*>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
			{
				(*it)->update();
			}
		}
	}


	void OpenNIDeviceManager::renderJoints( float pointSize )
	{
		glDisable( GL_TEXTURE_2D );
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		//for( std::list<OpenNIUser*>::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			(*it)->renderJoints( pointSize );
		}
	}
}