#include "VOpenNICommon.h"
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <direct.h>
#endif
#include "VOpenNIBone.h"
#include "VOpenNIUser.h"
#include "VOpenNINetwork.h"
#include "VOpenNIDevice.h"
#include <iostream>



// Make sure we link libraries
#pragma comment( lib, "openni.lib" )


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
		// Bail out if we reach the max amount of users allowed to interact
		if( OpenNIDeviceManager::Instance().getNumOfUsers() >= OpenNIDeviceManager::Instance().getMaxNumOfUsers() )
		{
			std::stringstream ss;
			ss << "NO MORE USERS ALLOWED. MAX: '" << OpenNIDeviceManager::Instance().getNumOfUsers() << " of " << OpenNIDeviceManager::Instance().getMaxNumOfUsers() << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			return;
		}

		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Add new user if not available already
		if( !OpenNIDeviceManager::Instance().hasUser(nId) )
		{
			OpenNIDeviceManager::Instance().addUser( &generator, nId );
			//std::stringstream ss;
			//ss << "New User: '" << nId << "'     Total: " << OpenNIDeviceManager::Instance().getNumOfUsers() << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );


			// TODO! Enable slot change. For now only slot 0 is used
			int slot = 0;

			if( device->isOneTimeCalibration() && device->_isFirstCalibrationComplete )
			{
				// Load Data For Each User
				if( device->getUserGenerator()->GetSkeletonCap().IsCalibrationData(slot) )
				{
					device->getUserGenerator()->GetSkeletonCap().LoadCalibrationData( nId, slot );
					device->getUserGenerator()->GetSkeletonCap().StartTracking( nId );

					std::stringstream ss2;
					ss2 << "Loaded Calibration Data For User: '" << nId << "'  Slot: " << slot << std::endl;
					DEBUG_MESSAGE( ss2.str().c_str() );
				}
			}
			else
			{
				std::stringstream ss2;
				ss2 << "Start Pose Detection For User: '" << nId << "'  Slot: " << slot << std::endl;
				DEBUG_MESSAGE( ss2.str().c_str() );
				generator.GetPoseDetectionCap().StartPoseDetection( g_strPose, nId );
			}


			// Send events to all listeners
			//UserListenerList listeners = device->getListeners();
			//for( uint32_t i=0; i<listeners.size(); i++ )
			//if( !device->mListeners.empty() )
			for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
			{
				UserListener* listener = *it;
				//UserListener* listener = listeners[i];
				//if( listener )
				{
					UserEvent event;
					event.mId = nId;
					//event.mUser = OpenNIDeviceManager::Instance().getUser( nId );

					std::stringstream ss2;
					ss2 << "Send new user event: '" << nId << std::endl;
					DEBUG_MESSAGE( ss2.str().c_str() );

					listener->onNewUser( event );
					//(*it)->onNewUser( event );
				}
			}
		}
	}

	// Callback: An existing user was lost
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_LostUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Remove user
		//if( device->getUser(nId) )
				//device->removeUser( nId );
		//OpenNIDeviceManager::Instance().setText( ss.str() );
		if( OpenNIDeviceManager::Instance().hasUser(nId) )
		{
			//std::stringstream ss;
			//ss << "Lost User: '" << nId << "'    Total: " << OpenNIDeviceManager::Instance().getNumOfUsers() << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );

			OpenNIDeviceManager::Instance().removeUser( nId );

			for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
			{
				UserListener* listener = *it;
				//if( listener )
				{
					UserEvent event;
					event.mId = nId;
					//event.mUser = OpenNIUserRef();

					std::stringstream ss2;
					ss2 << "Send lost user event: '" << nId << std::endl;
					DEBUG_MESSAGE( ss2.str().c_str() );

					listener->onLostUser( event );
					//(*it)->onLostUser( event );
				}
			}
		}
	}

	// Callback: Detected a pose
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseDetected( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		std::stringstream ss;
		ss << "Pose detection start: '" << strPose << "' on User: " << nId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDevice* device = static_cast<OpenNIDevice*>( pCookie );
		
		// If calibration done, skip this
		//if( device->_isFirstCalibrationComplete )
		//	return;

		//if( OpenNIDeviceManager::Instance().hasUser(nId) ) 
		{
			OpenNIDeviceManager::Instance().setText( ss.str() );

			// Pose found! Now we need skeleton calibration.
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
		OpenNIDevice* device = static_cast<OpenNIDevice*>( pCookie );

		// If calibration done, skip this
		//if( device->_isFirstCalibrationComplete )
		//{
		//	// Start tracking
		//	// Calibration succeeded
		//	std::stringstream ss0;
		//	ss0 << "Calibration loaded, start tracking user " << nId << std::endl;
		//	DEBUG_MESSAGE( ss0.str().c_str() );
		//	device->getUserGenerator()->GetSkeletonCap().StartTracking( nId );
		//	return;
		//}


		if( bSuccess )
		{
			// Calibration succeeded
			std::stringstream ss;
			ss << "Calibration complete, start tracking user " << nId << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			OpenNIDeviceManager::Instance().setText( ss.str() );

			// Save calibration data on slot 0
			// TODO! We should be able to choose a slot
			int slot = 0;

			if( !device->getUserGenerator()->GetSkeletonCap().IsCalibrationData(slot) &&
				device->isOneTimeCalibration() )
			{
				device->getUserGenerator()->GetSkeletonCap().SaveCalibrationData( nId, slot );
				std::stringstream ss2;
				ss2 << "Saving Calibration Data From User: '" << nId << "'  Slot: " << slot << std::endl;
				DEBUG_MESSAGE( ss2.str().c_str() );
				device->_isFirstCalibrationComplete = true;
			}

			// Start tracking
			device->getUserGenerator()->GetSkeletonCap().StartTracking( nId );
		}
		else
		{
			// Calibration failed
			std::stringstream ss;
			ss << "Calibration failed for user " << nId << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );

			OpenNIDeviceManager::Instance().setText( ss.str() );

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
	OpenNIDevice::OpenNIDevice( xn::Context* context ) : mBitsPerPixel( 3 )
	{
		_context = context;

		privateInit();
	}

	OpenNIDevice::OpenNIDevice( xn::Context* context, xn::Device* device ) : mBitsPerPixel( 3 )
	{
		_context = context;
		_device = *device;

		privateInit();
	}


	OpenNIDevice::~OpenNIDevice()
	{
		release();
	}


	void OpenNIDevice::privateInit()
	{
		//_configFile = "";
		//mDebugInfo = "No debug information\n";

		_isDepthInverted = false;
		_enableHistogram = false;

		_primaryGen	= NULL;
		_imageGen	= NULL;
		//_irGen		= NULL;
		//_depthGen	= NULL;
		_audioGen	= NULL;
		//_userGen	= NULL;
		_handsGen	= NULL;

		mColorSurface	= NULL;
		mIRSurface		= NULL;
		mDepthSurface	= NULL;

		//_colorData = NULL;
		_irData = NULL;
		_irData8 = NULL;
		_depthData = NULL;
		//_depthData8 = NULL;
		_depthDataRGB = NULL;
		_depthMapRealWorld = NULL;
		_backDepthMapRealWorld = NULL;
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
		_isSceneOn = false;
		_isUserOn = false;
		_isAudioOn = false;
		_isHandsOn = false;

		_isOneTimeCalibration = false;
		_isFirstCalibrationComplete = false;

		mNearClipPlane = 0;
		mFarClipPlane = 6000;

		mDepthShiftValue = 3;

		mSkeletonProfile = XN_SKEL_PROFILE_ALL;

		//_callback = NULL;
		//_callback = new OpenNIDeviceCallback( this, &OpenNIDevice::CallbackFunc );

		mListeners.clear();
	}


	bool OpenNIDevice::init( uint64_t nodeTypeFlags )
	{      
		// Pick image or IR map
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			//_imageGen = new xn::ImageGenerator();
			_imageMetaData = new xn::ImageMetaData();
			_status = _imageGen.Create( *_context );	//, NULL, &_errors );
			//CHECK_RC( _status, "Create image generator" );
			_isImageOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			//_irGen = new xn::IRGenerator();
			_irMetaData = new xn::IRMetaData();
			_status = _irGen.Create( *_context );	//, NULL, &_errors );
			//CHECK_RC( _status, "Create IR generator" );
			_isIROn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			// Depth map
			//_depthGen = new xn::DepthGenerator();
			_depthMetaData = new xn::DepthMetaData();
			_status = _depthGen.Create( *_context );	//, NULL, &_errors );
			//CHECK_RC( _status, "Create depth generator" );
			_isDepthOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			// User
			//_userGen = new xn::UserGenerator();
			_sceneMetaData = new xn::SceneMetaData();
			_status = _userGen.Create( *_context );	//, NULL, &_errors );
			//CHECK_RC( _status, "Create user generator" );
			_isUserOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_SCENE )
		{
			// Make sure we do not re-allocate
			if( !_sceneMetaData ) _sceneMetaData = new xn::SceneMetaData();
			_status = _sceneAnalyzer.Create( *_context );
			CHECK_RC( _status, "Create scene analyzer" );
			_isSceneOn = true;
		}

		//if( nodeTypeFlags & NODE_TYPE_AUDIO )
		//{
		//	// Audio
		//	//_audioGen = new xn::AudioGenerator();
		//	_audioMetaData = new AudioMetaData();
		//	_status = _audioGen.Create( *_context );	//, NULL, &_errors );
		//	CHECK_RC( _status, "Create audio generator" );
		//	_isAudioOn = true;
		//}
		//if( nodeTypeFlags & NODE_TYPE_HANDS )
		//{
		//	// Hand Gestures
		//	//_handsGen = new HandsGenerator();
		//	_status = _handsGen.Create( *_context );	//, NULL, &_errors );
		//	CHECK_RC( _status, "Create hands generator" );
		//	_isHandsOn = true;
		//}


		if( _isImageOn ) {
			setResolution( NODE_TYPE_IMAGE, XN_RES_VGA, 30 );
			_imageGen.GetMetaData( *_imageMetaData );
		}
		if( _isIROn )	{
			setResolution( NODE_TYPE_IR, XN_RES_VGA, 30 );
			_irGen.GetMetaData( *_irMetaData );
		}
		if( _isDepthOn )
		{
			setResolution( NODE_TYPE_DEPTH, XN_RES_VGA, 30 );
			_depthGen.GetMetaData( *_depthMetaData );
		}
		if( _isSceneOn )
		{
			setResolution( NODE_TYPE_SCENE, XN_RES_VGA, 30 );
			_sceneAnalyzer.GetMetaData( *_sceneMetaData );
		}

		//if( _userGen ) {
		//	_userGen->GetUserPixels( 0, *_sceneMetaData );
		//}
		//if( _isAudioOn ) {
		//	_audioGen.GetMetaData( *_audioMetaData );
		//}



		// Request calibration if user generator is enabled
		if( _isUserOn )
		{
			requestUserCalibration();
		}


		// Allocate memory for bitmaps (fixed to 640*480)
		// TODO: Give user a new parameter for dimensions
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
					(*it).GetInstance( _device );
					//ss << "Device: " << _device->GetName() << std::endl;
					//DEBUG_MESSAGE( ss.str().c_str() );
					mDeviceName = _device.GetName();
					break;
				case XN_NODE_TYPE_IMAGE:
					//_imageGen = new ImageGenerator();
					_imageMetaData = new ImageMetaData();
					(*it).GetInstance( _imageGen );
					ss << "Image: " << _imageGen.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isImageOn = true;
					break;
				case XN_NODE_TYPE_IR:
					//_irGen = new IRGenerator();
					_irMetaData = new IRMetaData();
					(*it).GetInstance( _irGen );
					ss << "IR: " << _irGen.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isIROn = true;
					break;
				case XN_NODE_TYPE_DEPTH:
					//_depthGen = new DepthGenerator();
					_depthMetaData = new DepthMetaData();
					(*it).GetInstance( _depthGen );
					ss << "Depth: " << _depthGen.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isDepthOn = true;
					break;
				case XN_NODE_TYPE_USER:
					//_userGen = new UserGenerator();
					_sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( _userGen );
					ss << "User: " << _userGen.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isUserOn = true;
					break;
				case XN_NODE_TYPE_SCENE:
					if( !_sceneMetaData ) _sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( _sceneAnalyzer );
					ss << "Scene: " << _sceneAnalyzer.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isSceneOn = true;
					break;
				//case XN_NODE_TYPE_AUDIO:
				//	//_audioGen = new AudioGenerator();
				//	_audioMetaData = new AudioMetaData();
				//	(*it).GetInstance( _audioGen );
				//	ss << "Audio: " << _audioGen.GetName() << std::endl;
				//	DEBUG_MESSAGE( ss.str().c_str() );
				//	_isAudioOn = true;
				//	break;
				case XN_NODE_TYPE_HANDS:
					//_handsGen = new HandsGenerator();
					(*it).GetInstance( _handsGen );
					ss << "Hands: " << _handsGen.GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isHandsOn = true;
					break;
				}
			}
		}


		//
		// In case the config file has no user node, we create one if so we want to
		//
		if( !_isUserOn && allocUserIfNoNode )
		{
			//_userGen = new UserGenerator();
			_sceneMetaData = new xn::SceneMetaData();
			_status = _userGen.Create( *_context );
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
		if( _isSceneOn ) flags |= NODE_TYPE_SCENE;
		if( _isAudioOn ) flags |= NODE_TYPE_AUDIO;
		if( _isHandsOn ) flags |= NODE_TYPE_HANDS;


		XnMapOutputMode mode;
		if( flags & NODE_TYPE_IMAGE )
		{
			_imageGen.GetMapOutputMode( mode );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
		}
		if( flags & NODE_TYPE_IR )
		{
			_irGen.GetMapOutputMode( mode );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			_irData = new uint16_t[mode.nXRes*mode.nYRes];
			_irData8 = new uint8_t[mode.nXRes*mode.nYRes];
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			_depthGen.GetMapOutputMode( mode );
			_backDepthData = new uint16_t[mode.nXRes*mode.nYRes];
			_depthData = new uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			_depthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			_backDepthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			g_pTexMap = new XnRGB24Pixel[ mode.nXRes * mode.nYRes * sizeof(XnRGB24Pixel) ];
			g_MaxDepth = MAX_DEPTH;
			g_pDepthHist = new float[g_MaxDepth];
		}


		//
		// First time get meta data
		//
		if( _isImageOn ) _imageGen.GetMetaData( *_imageMetaData );
		if( _isIROn )	_irGen.GetMetaData( *_irMetaData );
		if( _isDepthOn ) _depthGen.GetMetaData( *_depthMetaData );
		if( _isSceneOn ) _sceneAnalyzer.GetMetaData( *_sceneMetaData );
		if( _isAudioOn) _audioGen.GetMetaData( *_audioMetaData );


		//
		// Handle things for hands generator (NITE stuff)
		//


		return true;
	}


	void OpenNIDevice::setAlignWithDepthGenerator()
	{
		//if( _depthGen.IsCapabilitySupported(XN_CAPABILITY_FRAME_SYNC) )
		//{
		//	_status = _depthGen.GetFrameSyncCap().FrameSyncWith( _imageGen );
		//	CHECK_RC( _status, "FrameSync" );
		//}

		// Align depth and image generators
		if( _depthGen.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT) )
		{
			_depthGen.GetAlternativeViewPointCap().ResetViewPoint();
			if( _imageGen ) _depthGen.GetAlternativeViewPointCap().SetViewPoint( _imageGen );
		}

	}

	void OpenNIDevice::release()
	{
		mListeners.clear();

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
		SAFE_DELETE_ARRAY( _depthMapRealWorld );
		SAFE_DELETE_ARRAY( _backDepthMapRealWorld );
		SAFE_DELETE_ARRAY( _backDepthData );
		SAFE_DELETE_ARRAY( g_pTexMap );
		SAFE_DELETE_ARRAY( g_pDepthHist );

		_primaryGen = NULL;	// just null the pointer 
		
		//_device.Release();

		_context = NULL;	// just null the pointer 

		SAFE_DELETE( _imageMetaData ); 
		SAFE_DELETE( _irMetaData );
		SAFE_DELETE( _depthMetaData );
		SAFE_DELETE( _sceneMetaData );
		SAFE_DELETE( _audioMetaData );

		//SAFE_DELETE( _callback );
	}



	void OpenNIDevice::start()
	{
		// empty
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
			if( !_depthMapRealWorld ) _depthMapRealWorld = new XnPoint3D[width*height];
			if( !_backDepthMapRealWorld ) _backDepthMapRealWorld = new XnPoint3D[width*height];
			if( !mDepthSurface ) mDepthSurface = new OpenNISurface16( NODE_TYPE_DEPTH, width, height );
			if( !g_pTexMap ) g_pTexMap = new XnRGB24Pixel[ width * height * sizeof(XnRGB24Pixel) ];
			if( !g_pDepthHist ) g_pDepthHist = new float[g_MaxDepth];
		}
	}




	bool OpenNIDevice::requestUserCalibration()
	{
		XnCallbackHandle hUserCallbacks = 0;
		XnCallbackHandle hCalibrationCallbacks = 0;
		if( !_userGen.IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
		{
			DEBUG_MESSAGE( "Supplied user generator doesn't support skeleton\n" );
			return false;
		}
		_userGen.RegisterUserCallbacks( &V::OpenNIDevice::Callback_NewUser, &V::OpenNIDevice::Callback_LostUser, this, hUserCallbacks );
	

		_userGen.GetSkeletonCap().RegisterCalibrationCallbacks( &V::OpenNIDevice::Callback_CalibrationStart, &V::OpenNIDevice::Callback_CalibrationEnd, this, hCalibrationCallbacks );
		if( _userGen.GetSkeletonCap().NeedPoseForCalibration() )
		{
			g_bNeedPose = TRUE;
			if( !_userGen.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION) )
			{
				DEBUG_MESSAGE( "Pose required, but not supported\n" );
				return false;
			}
			
			XnCallbackHandle hPoseCallbacks = 0;

			//_userGen->GetPoseDetectionCap().RegisterToPoseCallbacks( &V::OpenNIDevice::Callback_PoseDetected, NULL, this, hPoseCallbacks );
			_userGen.GetPoseDetectionCap().RegisterToPoseCallbacks( &V::OpenNIDevice::Callback_PoseDetected, &V::OpenNIDevice::Callback_PoseDetectionEnd, this, hPoseCallbacks );
			_userGen.GetSkeletonCap().GetCalibrationPose( g_strPose );
			std::stringstream ss;
			ss << "--> User pose name: '" << g_strPose << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
		}
		_userGen.GetSkeletonCap().SetSkeletonProfile( mSkeletonProfile );

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
			gen = &_imageGen;
			break;
		case NODE_TYPE_IR:
			gen = &_irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = &_depthGen;
			break;
		case NODE_TYPE_SCENE:
			gen = &_sceneAnalyzer;
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
			SAFE_DELETE_ARRAY( _depthMapRealWorld );
			SAFE_DELETE_ARRAY( _backDepthMapRealWorld );
			_depthData = new uint16_t[mode.nXRes*mode.nYRes];
			//_depthData8 = new uint8_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			_depthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			_backDepthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			g_pTexMap = new XnRGB24Pixel[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		case NODE_TYPE_SCENE:
			// Set scene analyzer mode and get out
			_sceneAnalyzer.SetMapOutputMode( mode );
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
			gen = &_imageGen;
			break;
		case NODE_TYPE_IR:
			gen = &_irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = &_depthGen;
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
			gen = &_imageGen;
			break;
		case NODE_TYPE_IR:
			gen = &_irGen;
			break;
		case NODE_TYPE_DEPTH:
			gen = &_depthGen;
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
		//
		// Handle depth map
		//
		if( _isDepthOn && _depthGen.IsValid() )
		{
			_depthGen.GetMetaData( *_depthMetaData );
			pDepth = _depthGen.GetDepthMap();

			// Compute bitmap buffers
			int w = _depthMetaData->XRes();
			int h = _depthMetaData->YRes();
			//memcpy( _backDepthData, pDepth, w*h*sizeof(XnDepthPixel) );


			//
			// Calculate the accumulative histogram (the yellow/cyan display...)
			//
			if( _enableHistogram )
			{
				calculateHistogram();
				xnOSMemSet( g_pTexMap, 0, w*h*sizeof(XnRGB24Pixel) );
			}


			// First, clear bitmap data
			xnOSMemSet( _backDepthData, 0, w*h*sizeof(uint16_t) );
			//xnOSMemSet( _depthData8, 0, w*h );

			const XnDepthPixel* pDepthRow = _depthMetaData->Data();


			XnRGB24Pixel* pTexRow = NULL;
			XnRGB24Pixel* pTex = NULL;
			if( _enableHistogram )
				pTexRow = g_pTexMap + _depthMetaData->YOffset() * w;


			mMinDistance = 99999;
			mMaxDistance = -99999;


			uint16_t* backDepthPtr = _backDepthData;

			for( XnUInt y = 0; y < _depthMetaData->YRes(); y++ )
			{
				const XnDepthPixel* pDepth = pDepthRow;
				if( _enableHistogram )
					pTex = pTexRow + _depthMetaData->XOffset();

				for( XnUInt x = 0; x < _depthMetaData->XRes(); x++ )
				{
					XnDepthPixel pixel = *pDepth;

					if( mMinDistance > pixel ) mMinDistance = pixel;
					if( mMaxDistance < pixel ) mMaxDistance = pixel;

					if( pixel > mNearClipPlane && pixel < mFarClipPlane )	//pixel != 0 )
					{
						if( _enableHistogram )
						{
							// Inverted or not ?
							uint32_t nHistValue = (_isDepthInverted) ? (uint32_t)(255-g_pDepthHist[*pDepth]) : (uint32_t)(g_pDepthHist[*pDepth]);

							// Fill or cyan RGB depth map
							pTex->nRed = nHistValue;
							pTex->nGreen = nHistValue;
							pTex->nBlue = nHistValue;
						}

						//// 16bit depth map
						if( _isDepthInverted )
						{
							*backDepthPtr = (*pDepth>0 && *pDepth<MAX_DEPTH) ? MAX_DEPTH-*pDepth : 0;
							//_backDepthData[index] = (*pDepth>0 && *pDepth<MAX_DEPTH) ? MAX_DEPTH-*pDepth : 0;
						}
						else
						{
							*backDepthPtr = *pDepth;
							//_backDepthData[index] = *pDepth;
						}
					}
					/*else
					{
						if( _enableHistogram )
						{
							pTex->nRed = 0;
							pTex->nGreen = 0;
							pTex->nBlue = 0;
						}

						*backDepthPtr = 0;
						//_depthData[index] = 0;
					}*/

					*backDepthPtr = (*backDepthPtr << mDepthShiftValue);
					//_backDepthData[index] = _backDepthData[index] << mDepthShiftValue;

					backDepthPtr++;
					//index++;


					pDepth++;
					pTex++;
				}

				pDepthRow += _depthMetaData->XRes();
				if( _enableHistogram )
					pTexRow += w;
			}

			if( _enableHistogram )	memcpy( _depthDataRGB, g_pTexMap, w*h*sizeof(XnRGB24Pixel) );
			memcpy( _depthData, _backDepthData, w*h*sizeof(uint16_t) );
		}


		if( _imageGen.IsValid() )
		{
			_imageGen.GetMetaData( *_imageMetaData );
			pImage = _imageGen.GetImageMap();
			// Compute bitmap buffers
			mColorSurface->update( (uint8_t*)(pImage) );
			//int w = _depthMetaData->XRes();
			//int h = _depthMetaData->YRes();
			//memcpy( mColorSurface->getData(), pImage, w*h*mBitsPerPixel*sizeof(XnUInt8) );
		}


		if( _irGen.IsValid() )
		{
			_irGen.GetMetaData( *_irMetaData );
			pIR = _irMetaData->Data();	//_irGen->GetIRMap();

			// Save original data
			XnUInt32 nDataSize = _irGen.GetDataSize();
			memcpy( _irData, pIR, nDataSize );

			// Compute 8bit and RGB color bitmap
			//int index = 0;
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


		//if( _isAudioOn && _audioGen.IsValid() )
		//{
		//	_audioGen.GetMetaData( *_audioMetaData );
		//}
	}


	void OpenNIDevice::getLabelMap( uint32_t userId, uint16_t* labelMap )
	{
		if( !_sceneAnalyzer.IsValid() ) return;

		_sceneAnalyzer.GetMetaData( *_sceneMetaData );
		//CHECK_RC( _status, "calcLabelMap()" );

		const XnLabel* labels = _sceneMetaData->Data();
		if( labels )
		{
			int depthWidth = _sceneMetaData->XRes();
			int depthHeight = _sceneMetaData->YRes();

			
			std::cout << "depth width:" << depthWidth << ", height:" << depthHeight << std::endl;
			
			//if( !labelMap )
				//labelMap = new uint16_t[depthWidth * depthHeight];


			// Same as below but no label checking
			//memcpy( labelMap, pDepth, depthWidth*depthHeight*sizeof(uint16_t) );

			// Copy label map
			memset( labelMap, 0, depthWidth*depthHeight*sizeof(uint16_t) );
			uint16_t* pDepth = _depthData;
			uint16_t* map = labelMap;

			//int index = 0;
			for( int j=0; j<depthHeight; j++ )
			{
				for( int i=0; i<depthWidth; i++ )
				{
					XnLabel label = *labels;

					if( label == userId )
					{
						// If a user pixel, take depth value from our depthmap
						*map = *pDepth;
					}
					//else
					//{
					//	// If pixel is null, set color to black
					//	*map = 0;
					//}

					pDepth++;
					map++;
					labels++;
				}
			}
		}
	}


	void OpenNIDevice::calcDepthImageRealWorld()
	{
		if( !_depthMapRealWorld ) return;
		const XnDepthPixel* pDepth = _depthMetaData->Data();
		XnPoint3D* map = _backDepthMapRealWorld;

		uint32_t bufSize = _depthMetaData->XRes() * _depthMetaData->YRes();
		for( uint32_t y=0; y<_depthMetaData->YRes(); y++ )
		{
			for(uint32_t x=0; x<_depthMetaData->XRes(); x++ )
			{
				if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				else
				{
					map->X = 0;
					map->Y = 0;
					map->Z = -9999;
				}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		_status = _depthGen.ConvertProjectiveToRealWorld( bufSize, _backDepthMapRealWorld, _depthMapRealWorld );
		//CHECK_RC( _status, "calcDepthImageRealWorld()" );
	}

	void OpenNIDevice::calcDepthImageRealWorld( uint16_t* pixelData, XnPoint3D* worldData )
	{
		if( !_depthMapRealWorld ) return;

		const XnDepthPixel* pDepth = pixelData;
		XnPoint3D* map = worldData;

		uint32_t bufSize = _depthMetaData->XRes() * _depthMetaData->YRes();
		for( uint32_t y=0; y<_depthMetaData->YRes(); y++ )
		{
			for(uint32_t x=0; x<_depthMetaData->XRes(); x++ )
			{
				if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				else
				{
					map->X = 0;
					map->Y = 0;
					map->Z = -9999;
				}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		_status = _depthGen.ConvertProjectiveToRealWorld( bufSize, worldData, worldData );
	}




	void OpenNIDevice::calculateHistogram()
	{
		if( !_isDepthOn )	//_depthGen == NULL)	
			return;

		xnOSMemSet( g_pDepthHist, 0, MAX_DEPTH*sizeof(float) );
		int nNumberOfPoints = 0;

		XnUInt32 nDataSize = _depthGen.GetDataSize();
		const XnDepthPixel* pDepth = _depthGen.GetDepthMap();
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
			if( g_pDepthHist[nIndex] > 0.0f )
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
		if( _isImageOn && (type & NODE_TYPE_IMAGE) )	
			_primaryGen = &_imageGen;
		else if( _irGen && (type & NODE_TYPE_IR) )	
			_primaryGen = &_irGen;
		else if( _isDepthOn && (type & NODE_TYPE_DEPTH) )	
			_primaryGen = &_depthGen;
		else if( _userGen && (type & NODE_TYPE_USER) )	
			_primaryGen = &_userGen;
		else if( _audioGen && (type & NODE_TYPE_AUDIO) )	
			_primaryGen = &_audioGen;
		else 
			_primaryGen = NULL;
	}


	void OpenNIDevice::setMirrorMode( int type, bool flag )
	{ 
		if( _isImageOn && (type & NODE_TYPE_IMAGE) )	
			_imageGen.GetMirrorCap().SetMirror( flag ); 
		else if( _irGen && (type & NODE_TYPE_IR) )	
			_irGen.GetMirrorCap().SetMirror( flag ); 
		else if( _isDepthOn && (type & NODE_TYPE_DEPTH) )	
			_depthGen.GetMirrorCap().SetMirror( flag ); 
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


	XnPoint3D* OpenNIDevice::getDepthMapRealWorld()
	{
		return _depthMapRealWorld;
	}

	/*void OpenNIDevice::addUser( uint32_t id )
	{
		OpenNIUserRef user = OpenNIUserRef( new OpenNIUser(id, this) );
		mUserList.push_back( user );
	}

	OpenNIUserRef OpenNIDevice::getUser( uint32_t id )
	{
		if( mUserList.empty() ) return OpenNIUserRef();
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
				return (*it);
		}
		return OpenNIUserRef();
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


	bool OpenNIDevice::hasUser( int32_t id )
	{
		if( mUserList.size() == 0 ) return false;
		for ( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); it++ )
		{
			if( id == (*it)->getId() )
				return true;
		}
		return false;
	}*/







	/*
	   Device Manager
	*/

	bool OpenNIDeviceManager::USE_THREAD = true;
	bool OpenNIDeviceManager::USE_NEW_WRAPPER_CODE = false;
	OpenNIDeviceManager OpenNIDeviceManager::_singletonPointer;


	void XN_CALLBACK_TYPE onErrorStateChanged( XnStatus errorState, void* pCookie )
	{
		if (errorState != XN_STATUS_OK)
		{
			//setErrorState(xnGetStatusString(errorState));
			std::stringstream ss;
			ss << " + " << xnGetStatusString(errorState) << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );

		}
		else
		{
			std::stringstream ss;
			ss << " - " << "Everything is ok" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
		}
	}


	OpenNIDeviceManager::OpenNIDeviceManager()
	{
		_isRunning = false;
		mIsContextInit = false;

		_idCount = 0;
		mMaxNumOfUsers = 2;
		mDebugInfo = "No debug information\n";
#ifdef WIN32
		mNetworkMsg = NULL;
		mTempPixels = NULL;
#endif

		mPrimaryGen = NULL;
		mDeviceCount = 0;
		mDepthGenCount = 0;
		mImageGenCount = 0;
		mIRGenCount = 0;
		mSceneAnalyzerCount = 0;
		mUserGenCount = 0;

		mDepthMap = NULL;
		mColorMap = NULL;
	}


	OpenNIDeviceManager::~OpenNIDeviceManager()
	{
		destroyAll();
	}

	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( const std::string& xmlFile/*=""*/, bool allocUserIfNoNode/*=false */ )
	{
		if( mDevices.size() >= MAX_DEVICES ) return boost::shared_ptr<OpenNIDevice>();
		
		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();
        
		
        // Make sure we have a device connected
        NodeInfoList list;
        EnumerationErrors errors;
        XnStatus status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEVICE, NULL, list, &errors );
        CHECK_RC( status, "No kinect device was found" );
        if( status != XN_STATUS_OK )
        {
            return OpenNIDeviceRef();            
        }

		// Bail out if its an empty filename
		if( xmlFile == "" )
		{
			DEBUG_MESSAGE( "not implemented" );
			return OpenNIDeviceRef();
		}        
        
		// Local copy of the filename
		std::string path = xmlFile;
		
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
		
		/* DEBUG STUFF *
		char msg[255];

		sprintf(msg, "Now there are %d OpenNI devices", mDevices.size());
		
		DEBUG_MESSAGE ( msg );
		*/
		
		return dev;
	}

	
	/* ORIGINAL VERSION **
	 
	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( const std::string& xmlFile, bool allocUserIfNoNode //=false 
														)
	{
		if( mDevices.size() >= MAX_DEVICES ) return boost::shared_ptr<OpenNIDevice>();

		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();

		// Bail out if its an empty filename
		if( xmlFile == "" )
		{
			DEBUG_MESSAGE( "not implemented" );
			return OpenNIDeviceRef();
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
		OpenNIDeviceRef dev = OpenNIDeviceRef( new OpenNIDevice(&_context) );

		if( !dev->initFromXmlFile( path, allocUserIfNoNode ) ) 
		{
			DEBUG_MESSAGE( "[OpenNIDeviceManager]  Couldn't create device from xml\n" );
			return OpenNIDeviceRef();
		}
		// By default set depth as primary generator
		//dev->setPrimaryBuffer( V::NODE_TYPE_DEPTH );

		// Save device to our list
		mDevices.push_back( dev );
		return dev;
	}
*/

	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( int nodeTypeFlags )
	{
		if( mDevices.size() >= MAX_DEVICES ) 
			return OpenNIDeviceRef();

		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();

        // Make sure we have a device connected
        NodeInfoList list;
        EnumerationErrors errors;
        XnStatus status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEVICE, NULL, list, &errors );
        CHECK_RC( status, "No kinect device was found" );
        if( status != XN_STATUS_OK )
        {
            return OpenNIDeviceRef();            
        }

        
		OpenNIDeviceRef dev = OpenNIDeviceRef( new OpenNIDevice(&_context) );
		if( !dev->init( nodeTypeFlags ) ) 
		{
			DEBUG_MESSAGE( "[OpenNIDeviceManager]  Couldn't create device manually\n" );
			return OpenNIDeviceRef();
		}
		// By default set depth as primary generator
		//dev->setPrimaryBuffer( V::NODE_TYPE_DEPTH );
		mDevices.push_back( dev );
		return dev;
	}


	void OpenNIDeviceManager::createDevices( uint32_t deviceCount, int nodeTypeFlags )
	{
		if( mDevices.size() >= MAX_DEVICES ) 
			return;

		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();

		XnStatus status;

		// Count max devices connected to this machine
		uint32_t devicesCount = 0;
		xn::NodeInfoList device_nodes; 
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEVICE, NULL, device_nodes, NULL );
		for( xn::NodeInfoList::Iterator nodeIt=device_nodes.Begin(); nodeIt!=device_nodes.End(); ++nodeIt ) 
		{
			devicesCount++;
			mDeviceCount++;
		}
        
        // No device is connected. Abort!
        if( deviceCount == 0 ) 
            return;

		// Make sure we do not allocate more than the ones connected
		if( deviceCount > devicesCount ) deviceCount = devicesCount;
		mDeviceCount = deviceCount;


		// Allocate devices
		for( uint32_t i=0; i<deviceCount; i++ )
		{
			OpenNIDeviceRef dev = OpenNIDeviceRef( new OpenNIDevice(&_context) );
			mDevices.push_back( dev );

			SDevice::Ref sdev = SDevice::Ref( new SDevice() );
			mDeviceList.push_back( sdev );
		}


		// Allocate all generators we ask for
		int imageCount = 0;
		int irCount = 0;
		int depthCount = 0;
		int userCount = 0;
		//int audioCount = 0;
		xn::NodeInfoList image_nodes; 
		xn::NodeInfoList ir_nodes; 
		xn::NodeInfoList depth_nodes;
		xn::NodeInfoList user_nodes; 
		xn::NodeInfoList scene_nodes; 
		xn::NodeInfoList hands_nodes; 
		//xn::NodeInfoList audio_nodes; 
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_IMAGE, NULL, image_nodes, NULL );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_IR, NULL, ir_nodes, NULL );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEPTH, NULL, depth_nodes, NULL );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_USER, NULL, user_nodes, NULL );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_SCENE, NULL, scene_nodes, NULL );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_HANDS, NULL, hands_nodes, NULL );
		//_status = _context.EnumerateProductionTrees( XN_NODE_TYPE_AUDIO, NULL, audio_nodes, NULL );


		// Temporary counter
		uint32_t count = 0;


		count = 0;
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=depth_nodes.Begin(); nodeIt!=depth_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				status = nodeInfo.GetInstance( dev->_depthGen );

				DepthGenerator gen;
				status = nodeInfo.GetInstance( gen );
				mDepthGenList.push_back( gen );


				++devIt;
				depthCount++;
				mDepthGenCount++;
				count++;
			}
		}

		count = 0;
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=ir_nodes.Begin(); nodeIt!=ir_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				status = nodeInfo.GetInstance( dev->_irGen );

				IRGenerator gen;
				status = nodeInfo.GetInstance( gen );
				mIRGenList.push_back( gen );

				++devIt;
				irCount++;
				mIRGenCount++;
				count++;
			}
		}

		count = 0;
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=image_nodes.Begin(); nodeIt!=image_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				status = nodeInfo.GetInstance( dev->_imageGen );

				ImageGenerator gen;
				status = nodeInfo.GetInstance( gen );
				mImageGenList.push_back( gen );

				++devIt;
				imageCount++;
				mImageGenCount++;
				count++;
			}
		}


		count = 0;
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=user_nodes.Begin(); nodeIt!=user_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				status = nodeInfo.GetInstance( dev->_depthGen );

				UserGenerator gen;
				status = nodeInfo.GetInstance( gen );
				mUserGenList.push_back( gen );

				++devIt;
				userCount++;
				mUserGenCount++;
				count++;
			}
		}


		count = 0;
		if( nodeTypeFlags & NODE_TYPE_SCENE )
		{
			for( xn::NodeInfoList::Iterator nodeIt=scene_nodes.Begin(); nodeIt!=scene_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				SceneAnalyzer gen;
				status = nodeInfo.GetInstance( gen );
				mSceneAnalyzerList.push_back( gen );

				mSceneAnalyzerCount++;
				count++;
			}
		}

		count = 0;
		if( nodeTypeFlags & NODE_TYPE_HANDS )
		{
			for( xn::NodeInfoList::Iterator nodeIt=hands_nodes.Begin(); nodeIt!=hands_nodes.End(); nodeIt++ ) 
			{
				if( count >= deviceCount )	
					break;

				xn::NodeInfo nodeInfo = *nodeIt;

				status = _context.CreateProductionTree( nodeInfo ); 

				HandsGenerator gen;
				status = nodeInfo.GetInstance( gen );
				mHandsGenList.push_back( gen );

				mHandsGenCount++;
				count++;
			}
		}

	}


	void OpenNIDeviceManager::Init()
	{
		_context.Init();

		XnCallbackHandle hDummy;
		_context.RegisterToErrorStateChange(onErrorStateChanged, NULL, hDummy);

		mIsContextInit = true;
	}



	void OpenNIDeviceManager::AllocateMem( uint32_t width, uint32_t height )
	{
		mDepthMap = new uint16_t[width*height];
		mColorMap = new uint8_t[width*height];
	}


	OpenNIDevice::Ref OpenNIDeviceManager::getDevice( uint32_t deviceIdx/*=0*/ )
	{
		if( mDevices.empty() || deviceIdx >= mDevices.size() ) 
		{
			std::stringstream ss;
			ss << "[OpenNIDeviceManager]  Device '" << deviceIdx << "' is not available" << std::endl;
			
			//throw std::exception( ss.str().c_str() );
			
			return OpenNIDevice::Ref();
		}

		int count = 0;
		OpenNIDeviceList::iterator currDevice = mDevices.begin();
		while( 1 )
		{
			if( count == deviceIdx )
				return *currDevice;
			else
			{
				*currDevice++;
				count++;
			}
		}
		return OpenNIDevice::Ref();
	}


	void OpenNIDeviceManager::destroyAll( void )
	{
		// Stop thread
		if( USE_THREAD && _isRunning )
		{
			_isRunning = false;
			DEBUG_MESSAGE( "Stop running thread on device manager\n" );
			assert( _thread );
			_thread->join();
			_thread.reset();
		}

		mPrimaryGen = NULL;
		mDepthGenList.clear();
		mImageGenList.clear();
		mIRGenList.clear();
		mSceneAnalyzerList.clear();
		mUserGenList.clear();
		mHandsGenList.clear();

		//mDepthMDList.clear();
		//mImageMDList.clear();
		//mIRMDList.clear();
		//mSceneMDList.clear();


		SAFE_DELETE_ARRAY( mDepthMap );
		SAFE_DELETE_ARRAY( mColorMap );


		// stop generators
		_context.StopGeneratingAll();


		mDeviceList.clear();

		// Delete device list
		mDevices.clear();

		// Delete user list
		mUserList.clear();



		//
		// Shutdown openNI context
		//
		_context.Release();
		//_context.Shutdown();
#ifdef WIN32
		if( mNetworkMsg )
		{
			mNetworkMsg->Release();
			SAFE_DELETE( mNetworkMsg );
		}
		SAFE_DELETE_ARRAY( mTempPixels );
#endif
	}


	OpenNIUserRef OpenNIDeviceManager::addUser( xn::UserGenerator* userGen, uint32_t id )
	{
		// Currently works with first device only.
		OpenNIDeviceList::iterator it = mDevices.begin();

		OpenNIUserRef newUser = OpenNIUserRef( new OpenNIUser(id, it->get()) );
		mUserList.push_back( newUser );
		return newUser;
	}

	void OpenNIDeviceManager::removeUser( uint32_t id )
	{
		if( mUserList.empty() ) return;
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			if( id == (*it)->getId() )
			{
				mUserList.remove( *it );
				return;
			}
		}
	}



	V::OpenNIUserRef OpenNIDeviceManager::getFirstUser()
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.empty() ) return OpenNIUserRef();
		return (*mUserList.begin());
	}

	V::OpenNIUserRef OpenNIDeviceManager::getSecondUser()
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.size() < 2 ) return OpenNIUserRef();

		int index = 0;
		for( OpenNIUserList::iterator it=mUserList.begin(); it!=mUserList.end(); ++it )
		{
			if( index == 1 )
				return (*it);

			index++;
		}
		return OpenNIUserRef();
	}


	V::OpenNIUserRef OpenNIDeviceManager::getLastUser()
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		return ( mUserList.empty() ) ? OpenNIUserRef() : (*mUserList.end());
	}

	OpenNIUserRef OpenNIDeviceManager::getUser( int id )
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::lock_guard<boost::mutex> lock( _mutex );
		//boost::mutex::scoped_lock lock( _mutex );

		//if( mUserList.empty() ) return OpenNIUserRef();
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			if( id == (*it)->getId() )
				return (*it);
		}

		return OpenNIUserRef();
	}


	bool OpenNIDeviceManager::hasUser( int32_t id )
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.empty() ) return false;
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			if( id == (*it)->getId() )
				return true;
		}

		return false;
	}

	bool OpenNIDeviceManager::hasUsers()
	{ 
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		return !mUserList.empty(); 
	}


	const uint32_t OpenNIDeviceManager::getNumOfUsers()
	{ 
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		///boost::mutex::scoped_lock lock( _mutex );

		return (uint32_t)mUserList.size();	
	}
	
	OpenNIUserList OpenNIDeviceManager::getUserList()
	{ 
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		return mUserList;	
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

/*		// Start all devices
		for( OpenNIDeviceList::iterator it = mDevices.begin(); it != mDevices.end(); it++ )
		{
			(*it)->start();
		}*/

		// Start generators
		_context.StartGeneratingAll();
	}



	void OpenNIDeviceManager::update()
	{
		if( USE_THREAD ) 
		{
			//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
			boost::mutex::scoped_lock lock( _mutex );
		}



		XnStatus rc = XN_STATUS_OK;
		if( mPrimaryGen != NULL )
		{
			rc = _context.WaitOneUpdateAll( *mPrimaryGen );
		}
		else
		{
			rc = _context.WaitNoneUpdateAll();	// Make sure we are not blocking the application to the refresh-rate of a camera/generator (usually 30fps)
			//rc = _context.WaitAndUpdateAll();
			//rc = _context.WaitAnyUpdateAll();
		}
		//if( rc != XN_STATUS_OK )
		//{
		//	std::stringstream ss;
		//	ss << " Error: " << xnGetStatusString(rc) << std::endl;
		//	DEBUG_MESSAGE( ss.str().c_str() );
		//	return;
		//}


		if( !USE_NEW_WRAPPER_CODE )
		{
			// Handle device update
			for( OpenNIDeviceList::iterator it = mDevices.begin(); it != mDevices.end(); ++it )
			{
				(*it)->readFrame();
			}

			// Handle user update
			for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
			{
				(*it)->update();
			}
		}



		// Sleep
		if( USE_THREAD ) 
			boost::this_thread::sleep( boost::posix_time::millisec(1) ); 
	}


	void OpenNIDeviceManager::renderJoints( float width, float height, float depth, float pointSize, bool renderDepth )
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.empty() ) return;

		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			(*it)->renderJoints( width, height, depth, pointSize, renderDepth );
		}
	}





	/************************************************************************/
	/*                                                                      */
	/************************************************************************/

	uint16_t* OpenNIDeviceManager::getDepthMap( uint32_t deviceIdx/*=0*/ )
	{
		//if( USE_THREAD ) boost::mutex::scoped_lock lock( _mutex );

		if( !mDepthGenList[deviceIdx].IsValid() ) return NULL;

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		return (uint16_t*)mDepthMD.Data();
	}


	uint16_t* OpenNIDeviceManager::getDepthMapShift( uint32_t deviceIdx/*=0*/, uint32_t shiftMul/*=3*/ )
	{
		//if( USE_THREAD ) boost::mutex::scoped_lock lock( _mutex );

		if( !mDepthGenList[deviceIdx].IsValid() ) return NULL;

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		uint32_t siz = mDepthMD.XRes()*mDepthMD.YRes();

		if( !mDepthMap )
			mDepthMap = new uint16_t[siz];

		const XnDepthPixel *map = mDepthMD.Data();
		uint16_t *dest = mDepthMap;
		for( uint32_t i=0; i<siz; i++ )
		{
			uint32_t pixel = *map;
			*dest = (pixel << shiftMul);

			map++;
			dest++;
		}
		return mDepthMap;
	}


	uint8_t* OpenNIDeviceManager::getColorMap( uint32_t deviceIdx/*=0*/ )
	{
		//if( USE_THREAD ) boost::mutex::scoped_lock lock( _mutex );

		// Case if its Image
		if( !mImageGenList.empty() && mImageGenList[deviceIdx].IsValid() )
		{
			mImageGenList[deviceIdx].GetMetaData( mImageMD );
			return (uint8_t*)mImageMD.Data();
		}

		// Case if its Infrared
		if( !mIRGenList.empty() && mIRGenList[deviceIdx].IsValid() )
		{
			mIRGenList[deviceIdx].GetMetaData( mIRMD );

			uint32_t siz = mIRMD.XRes()*mIRMD.YRes();

			if( !mColorMap ) 
				mColorMap = new uint8_t[siz*3];


			const XnDepthPixel *map = mIRMD.Data();
			uint8_t *dest = mColorMap;
			for( uint32_t i=0; i<siz; i++ )
			{
				uint32_t pixel = *map;
				uint8_t value = (uint8_t)(pixel * 0.5f); 
				*dest++ = value;
				*dest++ = value;
				*dest++ = value;

				map++;
			}

			/*
			XnUInt8 val;
			for( XnUInt y=0; y<mIRMD.YRes(); ++y )
			{
				const XnIRPixel*   pIr		= mIRMD.Data()  + (y * mIRMD.XRes());
				uint8_t*           pPixel	= mColorMap     + (y * mIRMD.XRes());

				for( XnUInt x=0; x<mIRMD.XRes(); ++x, ++pIr )
				{
					// We divide value by 2
					val = (XnUInt8)((float)*pIr * .5f);
					*pPixel++ = val;
					*pPixel++ = val;
					*pPixel++ = val;
				}
			}*/

			return mColorMap;
		}

		return NULL;
	}



	void OpenNIDeviceManager::SetPrimaryBuffer( uint32_t type )
	{
		if( (type & NODE_TYPE_IMAGE) )	
			mPrimaryGen = &mImageGenList[0];
		else if( (type & NODE_TYPE_IR) )	
			mPrimaryGen = &mIRGenList[0];
		else if( (type & NODE_TYPE_DEPTH) )	
			mPrimaryGen = &mDepthGenList[0];
		else if( (type & NODE_TYPE_USER) )	
			mPrimaryGen = &mUserGenList[0];
		else 
			mPrimaryGen = NULL;
	}

	void OpenNIDeviceManager::CalcDepthImageRealWorld( uint32_t deviceIdx, uint16_t* pixelData, XnPoint3D* worldData )
	{
		//boost::mutex::scoped_lock lock( _mutex );

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		const XnDepthPixel* pDepth = pixelData;
		XnPoint3D* map = worldData;

		uint32_t bufSize = mDepthMD.XRes() * mDepthMD.YRes();
		for( uint32_t y=0; y<mDepthMD.YRes(); y++ )
		{
			for(uint32_t x=0; x<mDepthMD.XRes(); x++ )
			{
				if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				else
				{
					map->X = 0;
					map->Y = 0;
					map->Z = -9999;
				}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		mDepthGenList[deviceIdx].ConvertProjectiveToRealWorld( bufSize, worldData, worldData );
	}


	void OpenNIDeviceManager::GetSceneLabelMap( uint32_t deviceIdx, uint32_t labelId, uint16_t* labelMap )
	{
		//boost::mutex::scoped_lock lock( _mutex );

		SceneAnalyzer& scene = mSceneAnalyzerList[deviceIdx];
		// Make sure scene is available
		if( !scene.IsValid() ) return;

		scene.GetMetaData( mSceneMD );

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		const XnLabel* labels = mSceneMD.Data();

		if( labels )
		{
			int depthWidth = mSceneMD.XRes();
			int depthHeight = mSceneMD.YRes();

			memset( labelMap, 0, depthWidth*depthHeight*sizeof(uint16_t) );

			//std::stringstream ss;
			//ss << " + " << *labels << " - " << _sceneMetaData->FrameID() << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );

			const XnDepthPixel* pDepth = mDepthMD.Data();	//getDepthMap( deviceIdx );
			uint16_t* map = labelMap;

			//int index = 0;
			for( int j=0; j<depthHeight; j++ )
			{
				for( int i=0; i<depthWidth; i++ )
				{
					XnLabel label = *labels;

					if( label == labelId )
					{
						*map = *pDepth;
					}

					pDepth++;
					map++;
					labels++;
				}
			}
		}
	}



	void OpenNIDeviceManager::GetUserPixels( uint32_t deviceIdx, uint32_t userId, uint16_t* labelMap )
	{
		XnStatus status;
		status = mUserGenList[deviceIdx].GetUserPixels( userId, mSceneMD );

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		const XnLabel* labels = mSceneMD.Data();

		if( labels )
		{
			int depthWidth = mSceneMD.XRes();
			int depthHeight = mSceneMD.YRes();

			memset( labelMap, 0, depthWidth*depthHeight*sizeof(uint16_t) );

			//std::stringstream ss;
			//ss << " + " << *labels << " - " << _sceneMetaData->FrameID() << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );

			const XnDepthPixel* pDepth = mDepthMD.Data();	//getDepthMap( deviceIdx );
			uint16_t* map = labelMap;

			//int index = 0;
			for( int j=0; j<depthHeight; j++ )
			{
				for( int i=0; i<depthWidth; i++ )
				{
					XnLabel label = *labels;

					if( label == userId )
					{
						*map = *pDepth;
					}

					pDepth++;
					map++;
					labels++;
				}
			}
		}
	}


	void OpenNIDeviceManager::SetFrameSync( uint32_t deviceIdx, uint32_t index1 )
	{
		XnStatus status;
		if( mDepthGenList[deviceIdx].IsCapabilitySupported(XN_CAPABILITY_FRAME_SYNC) )
		{
			status = mDepthGenList[deviceIdx].GetFrameSyncCap().FrameSyncWith( mDepthGenList[index1] );
			CHECK_RC( status, "FrameSync" );
		}
	}


	void OpenNIDeviceManager::AlignRGBAndDepth( uint32_t deviceIdx )
	{
		if( mImageGenList.empty() ) return;

		// Align depth and image generators
		if( mDepthGenList[deviceIdx].IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT) )
		{
			mDepthGenList[deviceIdx].GetAlternativeViewPointCap().ResetViewPoint();
			if( mImageGenList[deviceIdx].IsValid() ) mDepthGenList[deviceIdx].GetAlternativeViewPointCap().SetViewPoint( mImageGenList[deviceIdx] );
		}

	}


	void OpenNIDeviceManager::CalcFieldOfView( uint32_t deviceIdx/*=0*/ )
	{
		GetFieldOfViewH( deviceIdx );
		GetFieldOfViewV( deviceIdx );
		mRealWorldXtoZ = tan( mHFov/2 ) * 2;
		mRealWorldYtoZ = tan( mVFov/2 ) * 2;
	}


	//
	// Getters
	//

	float OpenNIDeviceManager::GetFieldOfViewH( uint32_t deviceIdx/*=0*/ )
	{
		XnFieldOfView fov;
		// Only for depth sensor
		mDepthGenList[deviceIdx].GetFieldOfView( fov );
		return (float)fov.fHFOV;
	}

	float OpenNIDeviceManager::GetFieldOfViewV( uint32_t deviceIdx/*=0*/ )
	{
		XnFieldOfView fov;
		// Only for depth sensor
		mDepthGenList[deviceIdx].GetFieldOfView( fov );
		return (float)fov.fVFOV;
	}


	void OpenNIDeviceManager::UpdateFrame( uint32_t deviceIdx/*=0 */ )
	{

	}

	void OpenNIDeviceManager::UpdateUsers( uint32_t deviceIdx/*=0 */ )
	{
	}

#ifdef _WIN32
	void OpenNIDeviceManager::EnableNetworking( bool flag, const std::string& hostName/*="127.0.0.1"*/, uint16_t port/*=8888*/ )
	{
		if( flag )
		{
			if( !mNetworkMsg )
			{
				mNetworkMsg = new OpenNINetwork(hostName, port );
				mNetworkMsg->Init();
				mTempPixels = new uint16_t[640*480];
			}

			mEnableNetwork = true;
		}
		else
		{
			mEnableNetwork = false;
		}
	}


	void OpenNIDeviceManager::SendNetworkUserData()
	{
		if( !mEnableNetwork || mUserList.empty() ) return;

		char buffer[1024];	// 1k 

		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			OpenNIUser::Ref user = *it;
			
			if( user->getUserState() != V::USER_TRACKING )
				continue;

			for( OpenNIBoneList::iterator bit = user->getBoneList().begin(); bit != user->getBoneList().end(); ++bit )
			{
				V::OpenNIBone* bone = *bit;

				// Format message "USERID JOINTID X Y Z"
				memset( buffer, 0, 1024 );
				sprintf_s( buffer, "%d %d %f %f %f\0", user->getId(), bone->id, bone->position[0], bone->position[1], bone->position[2] );
				mNetworkMsg->Send( buffer );
				//std::stringstream message;
				//message << user->getId() << " " << bone->id << " " << bone->position[0] << " " << bone->position[1] << " " << bone->position[2];
				//mNetworkMsg->Send( message.str().c_str() );
			}
		}

/*
		// Send frame ID
		unsigned char frameID[1] = { 0 };
		mNetworkMsg->Send( (char*)frameID );

		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			OpenNIUser::Ref user = *it;

			// Send user ID
			mNetworkMsg->Send( (char*)user->getId() );

			for( OpenNIBoneList::iterator bit = user->getBoneList().begin(); bit != user->getBoneList().end(); ++bit )
			{
				V::OpenNIBone* bone = *bit;

				// Send bone data "USERID X Y Z Qx Qy Qz Qw"
				float boneData[4] = { bone->id, bone->position[0], bone->position[1], bone->position[2] };
				mNetworkMsg->Send( (char*)boneData );

			}
		}*/
	}


	void OpenNIDeviceManager::SendNetworkUserData( uint32_t userId )
	{
		OpenNIUser::Ref user = getUser( userId );

		if( !mEnableNetwork || !user ) return;
		if( user->getUserState() != V::USER_TRACKING ) return;

		OpenNIBoneList boneList = user->getBoneList();

		//static const int bufferSize = 1024;
		//char buffer[bufferSize];	// 1k 

		int frameID = 9;

		for( OpenNIBoneList::iterator bit = boneList.begin(); bit != boneList.end(); ++bit )
		{
			V::OpenNIBone* bone = *bit;

			// Format message "FRAMEID USERID JOINTID X Y Z Qx Qy Qz Qw"
			//memset( buffer, 0, bufferSize*sizeof(char) );
			//sprintf_s( buffer, "%d %d %d %0.2f %0.2f %0.2f\n", frameID, user->getId(), bone->id, bone->position[0], bone->position[1], bone->position[2] );
			////sprintf_s( buffer, "%d %d %d %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f %0.2f", 0, user->getId(), bone->id, bone->position[0], bone->position[1], bone->position[2] );
			//int result = mNetworkMsg->Send( buffer );

			float buff[10] = { (float)frameID, 
								(float)user->getId(), 
								(float)bone->id, 
								(float)bone->position[0], 
								(float)bone->position[1], 
								(float)bone->position[2], 
								(float)-1, 
								(float)-1, 
								(float)-1, 
								(float)-1
							};
			int result = mNetworkMsg->Send<float>( buff, 10 );

			//std::stringstream ss;
			//ss << "Send result: " << result << std::endl;
			//OutputDebugStringA( ss.str().c_str() );
		}
	}


	void OpenNIDeviceManager::SendNetworkPixels( uint32_t deviceIdx, uint16_t* pixels, uint32_t sendBlocksSize/*=65535*/ )
	{
		if( !mEnableNetwork || !pixels ) return;

		//uint16_t header[1] = { 0xffff };
		//int result = mNetworkMsg->Send( header, 1 );

		// Send all image at once
		int result = mNetworkMsg->Send<uint16_t>( pixels, 640*480 );

/*		int count = 0;
		uint16_t *buff = pixels;
		while( count < 640*480 )
		{
			int result = mNetworkMsg->Send( buff, sendBlocksSize );
			buff += sendBlocksSize;
			count += sendBlocksSize;
		}*/
	}

	void OpenNIDeviceManager::SendNetworkUserPixels( uint32_t deviceIdx, uint32_t userId, uint32_t sendBlocksSize/*=65535*/ )
	{
		if( !mEnableNetwork ) return;
		//OpenNIUser::Ref user = getUser( userId );
		//if( !mEnableNetwork || !user ) return;

		OpenNIDeviceRef device = getDevice( deviceIdx );
		device->getLabelMap( userId, mTempPixels );
		//GetSceneLabelMap( deviceIdx, userId, pixels );

		int count = 0;
		uint16_t *buff = mTempPixels;
		while( count < 640*480 )
		{
			int result = mNetworkMsg->Send<uint16_t>( buff, sendBlocksSize );
			buff += sendBlocksSize;
			count += sendBlocksSize;
		}
	}

#endif

}	// Namespace V