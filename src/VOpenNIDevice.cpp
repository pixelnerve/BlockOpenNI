/***
	OpenNI Wrapper.

	Victor Martins
	2010-2011 Pixelnerve 
	http://www.pixelnerve.com


	Current version:
		OpenNI	1.5.2.7
		NITE	1.5.2.7
***/

//#include "VOpenNICommon.h"
//#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
//#include <direct.h>
//#endif
#include "VOpenNIBone.h"
#include "VOpenNIUser.h"
//#include "VOpenNINetwork.h"
#include "VOpenNIDevice.h"
#include "VOpenNIDeviceManager.h"

#include <iostream>
#include <fstream>
#include <ostream>


//std::ofstream log_;
//
//inline void Log( std::string msg )
//{
//	log_ << msg << std::endl;
//	log_.flush();
//}

// Make sure we link libraries
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	#pragma comment( lib, "openni.lib" )	// 32bit version
	//#pragma comment( lib, "openNI64.lib" )	// 64bit version
#endif


namespace V
{
	using namespace xn;


	static char g_strPose[128];
	static bool g_bNeedPose = false;


	/************************************************************************/
	/* Callbacks
	*/
	/************************************************************************/

	void XN_CALLBACK_TYPE OpenNIDevice::Callback_UserExit( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Send events to all listeners
		for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
		{
			//if( listener )
			{
				UserEvent event;
				event.mId = nId;
				//event.mUser = OpenNIDeviceManager::Instance().getUser( nId );

				(*it)->onUserExit( event );
			}
		}
		//std::stringstream ss2;
		//ss2 << "Send user exit event: '" << nId << std::endl;
		//DEBUG_MESSAGE( ss2.str().c_str() );
	}

	void XN_CALLBACK_TYPE OpenNIDevice::Callback_UserReEnter( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Send events to all listeners
		for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
		{
			//if( listener )
			{
				UserEvent event;
				event.mId = nId;
				//event.mUser = OpenNIDeviceManager::Instance().getUser( nId );

				(*it)->onUserReEnter( event );
			}
		}
		//std::stringstream ss2;
		//ss2 << "Send user reenter event: '" << nId << std::endl;
		//DEBUG_MESSAGE( ss2.str().c_str() );
	}

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
//		if( !OpenNIDeviceManager::Instance().hasUser(nId) )
		{
			// Add new user
			OpenNIDeviceManager::Instance().addUser( &generator, nId );

			XnUInt32 epochTime = 0;
			xnOSGetEpochTime( &epochTime );
			std::stringstream ss;
			ss << epochTime << " " << "New User '" << nId << "' / '" << OpenNIDeviceManager::Instance().getNumOfUsers() << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );


			// Only use calibration is asked for it. 
			// Usually it is set to TRUE by default
			if( device->_enableUserCalibration )
			{
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
					//std::stringstream ss2;
					//ss2 << "Start Pose Detection For User: '" << nId << "'  Slot: " << slot << std::endl;
					//DEBUG_MESSAGE( ss2.str().c_str() );

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


			// Send events to all listeners
			for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
			{
				//if( listener )
				{
					UserEvent event;
					event.mId = nId;
					//event.mUser = OpenNIDeviceManager::Instance().getUser( nId );

					(*it)->onNewUser( event );

					//std::stringstream ss2;
					//ss2 << "Send new user event: '" << nId << std::endl;
					//DEBUG_MESSAGE( ss2.str().c_str() );
				}
			}
		}
	}


	// Callback: An existing user was lost
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_LostUser( xn::UserGenerator& generator, XnUserID nId, void* pCookie )
	{
		OpenNIDevice* device = static_cast<OpenNIDevice*>(pCookie);

		// Remove user
//		if( OpenNIDeviceManager::Instance().hasUser(nId) )
		{
			OpenNIDeviceManager::Instance().removeUser( nId );

			for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
			{
				UserEvent event;
				event.mId = nId;
				//event.mUser = OpenNIUserRef();

				//std::stringstream ss2;
				//ss2 << "Send lost user event: '" << nId << std::endl;
				//DEBUG_MESSAGE( ss2.str().c_str() );

				(*it)->onLostUser( event );
			}
			
			XnUInt32 epochTime = 0;
			xnOSGetEpochTime( &epochTime );
			std::stringstream ss;
			ss << epochTime << " " << "Lost User '" << nId << "'.     Total: '" << OpenNIDeviceManager::Instance().getNumOfUsers() << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
		}
	}

	// Callback: Detected a pose
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseDetected( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
		//std::stringstream ss;
		//ss << "Pose detection start: '" << strPose << "' on User: " << nId << std::endl;
		//DEBUG_MESSAGE( ss.str().c_str() );

		OpenNIDevice* device = static_cast<OpenNIDevice*>( pCookie );
		
		//if( OpenNIDeviceManager::Instance().hasUser(nId) ) 
		{
			//OpenNIDeviceManager::Instance().setText( ss.str() );

			// Pose found! Now we need skeleton calibration.
			device->getUserGenerator()->GetPoseDetectionCap().StopPoseDetection( nId );
			device->getUserGenerator()->GetSkeletonCap().RequestCalibration( nId, TRUE );
		}
	}
	
	
	// Callback: pose detection in progress
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseInProgress( xn::PoseDetectionCapability& pose, const XnChar* strPose, XnUserID nId, XnPoseDetectionStatus poseError, void* pCookie )
	{
 		//std::stringstream ss;
 		//ss << "Pose detection in progress: '" << strPose << "' on User: " << nId << std::endl;
 		//DEBUG_MESSAGE( ss.str().c_str() );
	}
	
	
	// Callback: When the user is out of pose
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_PoseDetectionEnd( xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie )
	{
 		//std::stringstream ss;
 		//ss << "Pose detection end. User: '" << strPose << "'. Id: " << nId << std::endl;
 		//DEBUG_MESSAGE( ss.str().c_str() );
 		//OpenNIDeviceManager::Instance().setText( ss.str() );
	}
	

	// Callback: Started calibration
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_CalibrationStart( xn::SkeletonCapability& capability, XnUserID nId, void* pCookie )
	{
 		//std::stringstream ss;
 		//ss << "Calibration started for user " << nId << std::endl;
 		//DEBUG_MESSAGE( ss.str().c_str() );
 		//OpenNIDeviceManager::Instance().setText( ss.str() );
	}

	
	// Callback: Started calibration
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_CalibrationInProgress( xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus calibrationError, void* pCookie )
	{
 		//std::stringstream ss;
 		//ss << "Calibration in progress for user " << nId << std::endl;
 		//DEBUG_MESSAGE( ss.str().c_str() );
 		//OpenNIDeviceManager::Instance().setText( ss.str() );
	}

	
	// Callback: Finished calibration
	void XN_CALLBACK_TYPE OpenNIDevice::Callback_CalibrationComplete( xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus calibrationError, void* pCookie )
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

		if( calibrationError == XN_CALIBRATION_STATUS_OK )
		{
			// Calibration succeeded
			//std::stringstream ss;
			//ss << "Calibration complete, start tracking user " << nId << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );
			//OpenNIDeviceManager::Instance().setText( ss.str() );

			// Save calibration data on slot 0
			// TODO! We should be able to choose a slot
			int slot = 0;
			if( !device->getUserGenerator()->GetSkeletonCap().IsCalibrationData(slot) &&
				device->isOneTimeCalibration() )
			{
				device->getUserGenerator()->GetSkeletonCap().SaveCalibrationData( nId, slot );
				//std::stringstream ss2;
				//ss2 << "Saving Calibration Data From User: '" << nId << "'  Slot: " << slot << std::endl;
				//DEBUG_MESSAGE( ss2.str().c_str() );
				device->_isFirstCalibrationComplete = true;
			}
			
			for( UserListenerList::iterator it=device->mListeners.begin(); it!=device->mListeners.end(); ++it )
			{
				UserListener* listener = *it;
				//UserListener* listener = listeners[i];
				//if( listener )
				{
					UserEvent event;
					event.mId = nId;
					//event.mUser = OpenNIDeviceManager::Instance().getUser( nId );
					
					//std::stringstream ss2;
					//ss2 << "Send calibration-end event: '" << nId << std::endl;
					//DEBUG_MESSAGE( ss2.str().c_str() );
					
					listener->onCalibrationComplete( event );
				}
			}
			

			// Start tracking
			device->getUserGenerator()->GetSkeletonCap().StartTracking( nId );
		}
		else
		{
			//// Calibration failed
			//std::stringstream ss;
			//ss << "Calibration failed for user " << nId << std::endl;
			//DEBUG_MESSAGE( ss.str().c_str() );

			//OpenNIDeviceManager::Instance().setText( ss.str() );

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
	OpenNIDevice::OpenNIDevice( int index, OpenNIDeviceManager* mgr ) 
		: mBitsPerPixel( 3 )
	{
		_index = index;
		_mgr = mgr;
		_context = mgr->getContext();

		privateInit();
	}

	OpenNIDevice::OpenNIDevice( int index, OpenNIDeviceManager* mgr, xn::Device* device ) 
		: mBitsPerPixel( 3 )
	{
		_index = index;
		_mgr = mgr;
		_context = mgr->getContext();
		//_device = *device;

//		log_.open( "kinect_log.txt" );

		privateInit();
	}


	OpenNIDevice::~OpenNIDevice()
	{
//		log_.close();
		release();
	}


	void OpenNIDevice::privateInit()
	{
		//_configFile = "";
		//mDebugInfo = "No debug information\n";

		_primaryGen	= NULL;
		_imageGen	= NULL;
		//_irGen		= NULL;
		//_depthGen	= NULL;
		//_audioGen	= NULL;
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

//		_imageMetaData = NULL;
//		_irMetaData = NULL;
//		_depthMetaData = NULL;
//		_sceneMetaData = NULL;
//		_audioMetaData = NULL;

		_isImageOn = false;
		_isIROn = false;
		_isDepthOn = false;
		_isSceneOn = false;
		_isUserOn = false;
		_isAudioOn = false;
		_isHandsOn = false;

		_isDepthInverted = false;
		_enableHistogram = false;
		_enableUserCalibration = true;
		_isOneTimeCalibration = false;
		_isFirstCalibrationComplete = false;
		
		_confidenceThreshold = 0.5f;

		mNearClipPlane = 0;
		mFarClipPlane = 6000;

		mDepthShiftValue = 3;

		mSkeletonProfile = XN_SKEL_PROFILE_ALL;

		//_callback = NULL;
		//_callback = new OpenNIDeviceCallback( this, &OpenNIDevice::CallbackFunc );

		mListeners.clear();
	}


	bool OpenNIDevice::init( uint64_t nodeTypeFlags, int colorResolution, int depthResolution )
	{      
		// Pick image or IR map
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			_imageGen = &_mgr->mImageGenList[_index];
			if( !_imageGen->IsValid() )
			{
				//Log( "*** (OpenNIDevice)  Image generator is not valid\n" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  Image generator is not valid\n" );
				return false;
			}
			_isImageOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			_irGen = &_mgr->mIRGenList[_index];
			if( !_irGen->IsValid() ) 
			{
				//Log( "*** (OpenNIDevice)  IR generator is not valid" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  IR generator is not valid" );
				return false;
			}
			_isIROn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			_depthGen = &_mgr->mDepthGenList[_index];
			if( !_depthGen->IsValid() ) 
			{
				//Log( "*** (OpenNIDevice)  Depth generator is not valid" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  Depth generator is not valid" );
				return false;
			}
			_isDepthOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_SCENE )
		{
			_sceneAnalyzer = &_mgr->mSceneAnalyzerList[_index];
			if( !_sceneAnalyzer->IsValid() ) 
			{
				//Log( "*** (OpenNIDevice)  Scene generator is not valid" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  Scene generator is not valid" );
				return false;
			}
			_isSceneOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			_userGen = &_mgr->mUserGenList[_index];
			if( !_userGen->IsValid() ) 
			{
				//Log( "*** (OpenNIDevice)  User generator is not valid" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  User generator is not valid" );
				return false;
			}
			_isUserOn = true;
		}

		/*if( nodeTypeFlags & NODE_TYPE_HANDS )
		{
			// Hand Gestures
			_handsGen = &_mgr->mHandsGenList[_index];
			if( !_handsGen->IsValid() ) 
			{
				Log( "*** (OpenNIDevice)  Hands generator is not valid" );
				DEBUG_MESSAGE( "*** (OpenNIDevice)  Hands generator is not valid" );
				return false;
			}
			_isHandsOn = true;
		}*/
		//if( nodeTypeFlags & NODE_TYPE_AUDIO )
		//{
		//	// Audio
		//	//_audioGen = new xn::AudioGenerator();
		//	_audioMetaData = new AudioMetaData();
		//	_status = _audioGen.Create( *_context );	//, NULL, &_errors );
		//	CHECK_RC( _status, "Create audio generator" );
		//	_isAudioOn = true;
		//}


		if( _isImageOn ) 
		{
			int fps = 30;
			switch( colorResolution )
			{
			case RES_1280x1024:
				fps = 15;
				break;
			}
			setResolution( NODE_TYPE_IMAGE, colorResolution, fps );
			//_imageGen->GetMetaData( _imageMetaData );
		}
		if( _isIROn )	
		{
			int fps = 30;
			switch( colorResolution )
			{
			case RES_1280x1024:
				fps = 15;
				break;
			}
			setResolution( NODE_TYPE_IR, colorResolution, fps );
			//_irGen->GetMetaData( _irMetaData );
		}
		if( _isDepthOn )
		{
			int fps = 30;
			switch( colorResolution )
			{
			case RES_1280x1024:
				fps = 15;
				break;
			}
			setResolution( NODE_TYPE_DEPTH, depthResolution, fps );
			//_depthGen->GetMetaData( _depthMetaData );
		}
		//if( _isSceneOn )
		//{
		//	int fps = 30;
		//	switch( colorResolution )
		//	{
		//	case RES_1280x1024:
		//		fps = 15;
		//		break;
		//	}
		//	//setResolution( NODE_TYPE_SCENE, depthResolution, fps );
		//	//_sceneAnalyzer->GetMetaData( _sceneMetaData );
		//}

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

		
		allocate( static_cast<int>(nodeTypeFlags) ); //, xRes, yRes );


		return true;
	}


/***
	bool OpenNIDevice::initFromXmlFile( const std::string& xmlFile, bool allocUserIfNoNode )
	{
		//_configFile = xmlFile;

		_status = _context->InitFromXmlFile( xmlFile.c_str(), mScriptNode, &_errors );
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
					(*it).GetInstance( *_imageGen );
					ss << "Image: " << _imageGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isImageOn = true;
					break;
				case XN_NODE_TYPE_IR:
					//_irGen = new IRGenerator();
					_irMetaData = new IRMetaData();
					(*it).GetInstance( *_irGen );
					ss << "IR: " << _irGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isIROn = true;
					break;
				case XN_NODE_TYPE_DEPTH:
					//_depthGen = new DepthGenerator();
					_depthMetaData = new DepthMetaData();
					(*it).GetInstance( *_depthGen );
					ss << "Depth: " << _depthGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isDepthOn = true;
					break;
				case XN_NODE_TYPE_USER:
					//_userGen = new UserGenerator();
					_sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( *_userGen );
					ss << "User: " << _userGen->GetName() << std::endl;
					DEBUG_MESSAGE( ss.str().c_str() );
					_isUserOn = true;
					break;
				case XN_NODE_TYPE_SCENE:
					if( !_sceneMetaData ) _sceneMetaData = new xn::SceneMetaData();
					(*it).GetInstance( *_sceneAnalyzer );
					ss << "Scene: " << _sceneAnalyzer->GetName() << std::endl;
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
					(*it).GetInstance( *_handsGen );
					ss << "Hands: " << _handsGen->GetName() << std::endl;
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
		if( _isSceneOn ) flags |= NODE_TYPE_SCENE;
		if( _isAudioOn ) flags |= NODE_TYPE_AUDIO;
		if( _isHandsOn ) flags |= NODE_TYPE_HANDS;


		XnMapOutputMode mode;
		if( flags & NODE_TYPE_IMAGE )
		{
			_imageGen->GetMapOutputMode( mode );
			//_colorData = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
		}
		if( flags & NODE_TYPE_IR )
		{
			_irGen->GetMapOutputMode( mode );
			//_colorData = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			_irData = new uint16_t[mode.nXRes*mode.nYRes];
			_irData8 = new uint8_t[mode.nXRes*mode.nYRes];
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			_depthGen->GetMapOutputMode( mode );
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
		if( _isImageOn ) _imageGen->GetMetaData( *_imageMetaData );
		if( _isIROn )	_irGen->GetMetaData( *_irMetaData );
		if( _isDepthOn ) _depthGen->GetMetaData( *_depthMetaData );
		if( _isSceneOn ) _sceneAnalyzer->GetMetaData( *_sceneMetaData );
//		if( _isAudioOn) _audioGen->GetMetaData( *_audioMetaData );


		//
		// Handle things for hands generator (NITE stuff)
		//


		return true;
	}
***/
	
	

	void OpenNIDevice::setAlignWithDepthGenerator()
	{
		//if( _depthGen->IsCapabilitySupported(XN_CAPABILITY_FRAME_SYNC) )
		//{
		//	_status = _depthGen->GetFrameSyncCap().FrameSyncWith( _imageGen );
		//	CHECK_RC( _status, "FrameSync" );
		//}

		// Align depth and image generators
		if( _depthGen->IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT) )
		{
			_depthGen->GetAlternativeViewPointCap().ResetViewPoint();
			if( _imageGen ) _depthGen->GetAlternativeViewPointCap().SetViewPoint( *_imageGen );
		}

	}
	
	
	void OpenNIDevice::resetUser( int userId )
	{
		if( _userGen )
		{
			_status = _userGen->GetSkeletonCap().Reset( userId );
			CHECK_RC( _status, "Resetting user" );
		}
	}


	void OpenNIDevice::release()
	{
		mListeners.clear();

		// Delete buffers

		SAFE_DELETE( mColorSurface );
		SAFE_DELETE( mIRSurface );
		SAFE_DELETE( mDepthSurface );

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

		//_primaryGen = NULL;	// just null the pointer 
		
		//_device.Release();

		//_context = NULL;	// just null the pointer 

//		SAFE_DELETE( _imageMetaData ); 
//		SAFE_DELETE( _irMetaData );
//		SAFE_DELETE( _depthMetaData );
//		SAFE_DELETE( _sceneMetaData );
//		SAFE_DELETE( _audioMetaData );

		//SAFE_DELETE( _callback );
	}


	void OpenNIDevice::allocate( uint64_t flags ) //, uint32_t width, uint32_t height )
	{
		if( flags & NODE_TYPE_IMAGE )
		{
			//_colorData = new boost::uint8_t[width*height*mBitsPerPixel];
			if( !mColorSurface ) 
			{
				_imageGen->GetMetaData( _imageMetaData );
				int w = _imageMetaData.XRes();
				int h = _imageMetaData.YRes();
				mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, w, h );
			}
		}
		if( flags & NODE_TYPE_IR )
		{
			_irGen->GetMetaData( _irMetaData );
			int w = _irMetaData.XRes();
			int h = _irMetaData.YRes();
			if( !_irData ) 
				_irData = new uint16_t[w*h];
			if( !_irData8 ) 
				_irData8 = new uint8_t[w*h];
			if( !mIRSurface ) 
				mIRSurface = new OpenNISurface8( NODE_TYPE_IR, w, h);
			if( !mColorSurface ) 
				mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, w, h );
		}
		if( flags & NODE_TYPE_DEPTH )
		{
			_depthGen->GetMetaData( _depthMetaData );
			int w = _depthMetaData.XRes();
			int h = _depthMetaData.YRes();

			g_MaxDepth = MAX_DEPTH;
			if( !_backDepthData ) 
				_backDepthData = new uint16_t[w*h];
			if( !_depthData ) 
				_depthData = new uint16_t[w*h];
			if( !_depthDataRGB ) 
				_depthDataRGB = new uint8_t[w*h*mBitsPerPixel];
			if( !_depthMapRealWorld ) 
				_depthMapRealWorld = new XnPoint3D[w*h];
			if( !_backDepthMapRealWorld ) 
				_backDepthMapRealWorld = new XnPoint3D[w*h];
			if( !mDepthSurface ) 
				mDepthSurface = new OpenNISurface16( NODE_TYPE_DEPTH, w, h );
			if( !g_pTexMap ) 
				g_pTexMap = new XnRGB24Pixel[ w*h*sizeof(XnRGB24Pixel) ];
			if( !g_pDepthHist ) 
				g_pDepthHist = new float[g_MaxDepth];
		}
	}




	bool OpenNIDevice::requestUserCalibration()
	{
		XnCallbackHandle hUserCallbacks = 0;
		XnCallbackHandle hUserExitCallback = 0;
		XnCallbackHandle hUserReEnterCallback = 0;
		XnCallbackHandle hCalibrationStartCallback = 0;
		XnCallbackHandle hCalibrationInProgressCallback = 0;
		//XnCallbackHandle hPoseInProgressCallback = 0;
		XnCallbackHandle hCalibrationCompleteCallback = 0;
		XnCallbackHandle hPoseDetectedCallback = 0;

		if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
		{
			DEBUG_MESSAGE( "Supplied user generator doesn't support skeleton\n" );
			return false;
		}
		_status = _userGen->RegisterUserCallbacks( &V::OpenNIDevice::Callback_NewUser, &V::OpenNIDevice::Callback_LostUser, this, hUserCallbacks );
		CHECK_RC( _status, "Register User Callbacks" );

		_status = _userGen->GetSkeletonCap().RegisterToCalibrationStart( &V::OpenNIDevice::Callback_CalibrationStart, this, hCalibrationStartCallback );
		CHECK_RC( _status, "Register Calibration Start" );

		_status = _userGen->GetSkeletonCap().RegisterToCalibrationComplete( &V::OpenNIDevice::Callback_CalibrationComplete, this, hCalibrationCompleteCallback );
		CHECK_RC( _status, "Register Calibration Complete" );

		_status = _userGen->RegisterToUserExit( &V::OpenNIDevice::Callback_UserExit, this, hUserExitCallback );
		CHECK_RC( _status, "Register User Exit Callback" );

		_status = _userGen->RegisterToUserReEnter( &V::OpenNIDevice::Callback_UserReEnter, this, hUserReEnterCallback );
		CHECK_RC( _status, "Register User ReEnter Callback" );

		if( _userGen->GetSkeletonCap().NeedPoseForCalibration() )
		{
			g_bNeedPose = true;
			if( !_userGen->IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION) )
			{
				DEBUG_MESSAGE( "Pose required, but not supported\n" );
				return false;
			}
			
			_status = _userGen->GetPoseDetectionCap().RegisterToPoseDetected( &V::OpenNIDevice::Callback_PoseDetected, this, hPoseDetectedCallback );
			CHECK_RC( _status, "Register Pose Detected" );
			_userGen->GetSkeletonCap().GetCalibrationPose( g_strPose );
			std::stringstream ss;
			ss << "--> User pose name: '" << g_strPose << "'" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
		}

        setSkeletonProfile( mSkeletonProfile );
//		_userGen->GetSkeletonCap().SetSkeletonProfile( mSkeletonProfile );

		_status = _userGen->GetSkeletonCap().RegisterToCalibrationInProgress( &V::OpenNIDevice::Callback_CalibrationInProgress, this, hCalibrationInProgressCallback );
		CHECK_RC( _status, "Register Calibration InProgress" );

		//_status = _userGen->GetPoseDetectionCap().RegisterToPoseInProgress( &V::OpenNIDevice::Callback_PoseInProgress, this, hPoseInProgressCallback );
		//CHECK_RC( _status, "Register Pose InProgress" );

		return true;
	}



	void OpenNIDevice::setResolution( ProductionNodeType nodeType, int resolution, int fps )
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
		//case NODE_TYPE_SCENE:
			//gen = _sceneAnalyzer;
			//break;
		default:
			DEBUG_MESSAGE( "(setResolution)  Can't change resolution. Not a valid generator\n" );
			return;
		}
		
		if( !gen->IsValid() )
		{
			DEBUG_MESSAGE( "(setResolution)  Generator is not valid\n" );
			return;
		}
		
		
		// convert from our enumeration to ONI's
		XnResolution res = (XnResolution)resolution;
		
		XnMapOutputMode mode;
		gen->GetMapOutputMode( mode );
		mode.nXRes = Resolution(res).GetXResolution();
		mode.nYRes = Resolution(res).GetYResolution();
		mode.nFPS = fps;

		std::stringstream ss;
		ss << "(setResolution)  " << mode.nXRes << "x" << mode.nYRes << "x" << mode.nFPS;

		XnStatus nRetVal = gen->SetMapOutputMode( mode );
		CHECK_RC( nRetVal, "setResolution" );
		if( nRetVal != XN_STATUS_OK )
		{
			std::stringstream ss;
			ss << "(setResolution)   Failed to set resolution: " << xnGetStatusString(nRetVal) << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			return;
		}

		// Re-alloc bitmap buffers
		switch( nodeType )
		{
		case NODE_TYPE_IMAGE:
			//SAFE_DELETE_ARRAY( _colorData );
			//_colorData = new boost::uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			if( !mColorSurface ) 
				mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			else 
				mColorSurface->remap( mode.nXRes, mode.nYRes );
			break;
		case NODE_TYPE_IR:
			if( !mColorSurface ) 
				mColorSurface = new OpenNISurface8( NODE_TYPE_IMAGE, mode.nXRes, mode.nYRes );
			else 
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
			SAFE_DELETE_ARRAY( _depthDataRGB );
			SAFE_DELETE_ARRAY( g_pTexMap );
			SAFE_DELETE_ARRAY( _depthMapRealWorld );
			SAFE_DELETE_ARRAY( _backDepthMapRealWorld );
			_depthData = new uint16_t[mode.nXRes*mode.nYRes];
			_depthDataRGB = new uint8_t[mode.nXRes*mode.nYRes*mBitsPerPixel];
			_depthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			_backDepthMapRealWorld = new XnPoint3D[mode.nXRes*mode.nYRes];
			g_pTexMap = new XnRGB24Pixel[mode.nXRes*mode.nYRes*mBitsPerPixel];
			break;
		default:
			DEBUG_MESSAGE( "(setResolution)  Can't change bitmap size.\n" );
			return;
		}
	}


/*
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
			DEBUG_MESSAGE( "Can't change resolution. Not a valid generator\n" );
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
			DEBUG_MESSAGE( "Can't change resolution. Not a valid generator\n" );
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
			DEBUG_MESSAGE( "Can't change bitmap size.\n" );
			return;
		}
	}
*/


	void OpenNIDevice::readFrame()
	{
		//
		// Handle depth map
		//
		if( _isDepthOn && _depthGen->IsValid() && _depthGen->IsDataNew() )
		{
			_depthGen->GetMetaData( _depthMetaData );
			//pDepth = _depthGen->GetDepthMap();	//_depthMetaData.Data();
			//if( !pDepth )
			//{
			//	DEBUG_MESSAGE( "Depth bitmap is not valid\n" );
			//	return;
			//}

			// Compute bitmap buffers
			uint32_t w = _depthMetaData.XRes();
			uint32_t h = _depthMetaData.YRes();


			//
			// Calculate the accumulative histogram (the yellow/cyan display...)
			//
			if( _enableHistogram )
			{
				calculateHistogram();
				xnOSMemSet( g_pTexMap, 0, w*h*sizeof(XnRGB24Pixel) );
			}


			// First, clear bitmap data
			xnOSMemSet( _depthData, 0, w*h*sizeof(uint16_t) );
			xnOSMemSet( _backDepthData, 0, w*h*sizeof(uint16_t) );

			/*/
			pDepth = _depthMetaData.Data();
			for( uint32_t i=0; i<w*h; i++ )
			{
				_depthData[i] = pDepth[i] << mDepthShiftValue;
			}                
			/*/
			uint16_t* backDepthPtr = _depthData;	//_backDepthData;
			pDepth = _depthMetaData.Data();
			if( _isDepthInverted )
			{
				for( uint32_t i=0; i<w*h; i++ )
				{
					//if( *pDepth > 0 )
						*backDepthPtr = ( 65536 - (*pDepth) ) << mDepthShiftValue;
					backDepthPtr++;
					pDepth++;
				}
			}
			else
			{
				for( uint32_t i=0; i<w*h; i++ )
				{
					//if( *pDepth > 0 )
						*backDepthPtr = *pDepth << mDepthShiftValue;
					backDepthPtr++;
					pDepth++;
				}                
			}
			
			if( _enableHistogram )	memcpy( _depthDataRGB, g_pTexMap, w*h*sizeof(XnRGB24Pixel) );
			//memcpy( _depthData, _backDepthData, w*h*sizeof(uint16_t) );
			/***/
		}


		if( _isImageOn && _imageGen->IsValid() && _imageGen->IsDataNew() )
		{
			_imageGen->GetMetaData( _imageMetaData );
			pImage = _imageMetaData.Data(); //_imageGen->GetImageMap();
			if( !pImage )
			{
				DEBUG_MESSAGE( "Image bitmap is not valid\n" );
				return;
			}
			// Compute bitmap buffers
			mColorSurface->update( (uint8_t*)(pImage) );
		}


		if( _isIROn && _irGen->IsValid()  && _irGen->IsDataNew() )
		{
			_irGen->GetMetaData( _irMetaData );
			pIR = _irMetaData.Data();	//_irGen->GetIRMap();

			// Save original data
			XnUInt32 nDataSize = _irGen->GetDataSize();
			memcpy( _irData, pIR, nDataSize );

			// Compute 8bit and RGB color bitmap
			//int index = 0;
			uint8_t* pArray = mColorSurface->getData();
			uint32_t dim = nDataSize / sizeof(XnIRPixel);
			for( uint32_t i=0; i<dim; i++ )
			{
				uint32_t v = _irData[i];
				//uint16_t d = 65535 - ((v*v) >> 4);
				int32_t d = v << 5;	// move to 16bit
				_irData[i] = d;

				v = (v>>2)&0xff; //(v<255)?v:255;

				// Convert to 8bit precision (luminance)
				_irData8[i] = v;

				// rgb grayscale image (8bit)
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
		if( !_sceneAnalyzer->IsValid() ) 
			return;

		_sceneAnalyzer->GetMetaData( _sceneMetaData );
		//CHECK_RC( _status, "calcLabelMap()" );

		const XnLabel* labels = _sceneMetaData.Data();
		
		if( labels )
		{
			//std::cout << "depth width:" << depthWidth << ", height:" << depthHeight << std::endl;
			int depthWidth = _sceneMetaData.XRes();
			int depthHeight = _sceneMetaData.YRes();			
			memset( labelMap, 0, depthWidth*depthHeight*sizeof(uint16_t) );
	
			uint16_t* pDepth = _depthData;
			uint16_t* map = labelMap;

			
			if( userId > 0 )
			{                
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
						
						pDepth++;
						map++;
						labels++;
					}
				}                
			}
			else
			{
				for( int i=0; i<depthWidth*depthHeight; i++ )
				{
					XnLabel label = *labels;
					
					if( label > 0 )
						*map = *pDepth;
					
					pDepth++;
					map++;
					labels++;
				}
			}
		}
	}



	void OpenNIDevice::calcDepthImageRealWorld( XnPoint3D* buffer )
	{
		if( !_depthMapRealWorld ) return;
		const XnDepthPixel* pDepth = _depthMetaData.Data();
		uint32_t bufSize = _depthMetaData.XRes() * _depthMetaData.YRes();
		XnPoint3D* map = _backDepthMapRealWorld;
		for( uint32_t y=0; y<_depthMetaData.YRes(); y++ )
		{
			for(uint32_t x=0; x<_depthMetaData.XRes(); x++ )
			{
				//if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				//else
				//{
				//	map->X = 0;
				//	map->Y = 0;
				//	map->Z = -9999;
				//}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		_status = _depthGen->ConvertProjectiveToRealWorld( bufSize, _backDepthMapRealWorld, buffer );
		//CHECK_RC( _status, "calcDepthImageRealWorld()" );
	}

	void OpenNIDevice::calcDepthImageRealWorld()
	{
		if( !_depthMapRealWorld ) return;
		const XnDepthPixel* pDepth = _depthMetaData.Data();
		uint32_t bufSize = _depthMetaData.XRes() * _depthMetaData.YRes();
		XnPoint3D* map = _backDepthMapRealWorld;
		for( uint32_t y=0; y<_depthMetaData.YRes(); y++ )
		{
			for(uint32_t x=0; x<_depthMetaData.XRes(); x++ )
			{
				//if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				//else
				//{
				//	map->X = 0;
				//	map->Y = 0;
				//	map->Z = -9999;
				//}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		_status = _depthGen->ConvertProjectiveToRealWorld( bufSize, _backDepthMapRealWorld, _depthMapRealWorld );
		//CHECK_RC( _status, "calcDepthImageRealWorld()" );
	}


	void OpenNIDevice::calcDepthImageRealWorld( uint16_t* pixelData, XnPoint3D* worldData )
	{
		const XnDepthPixel* pDepth = pixelData;
		const uint32_t bufSize = _depthMetaData.XRes() * _depthMetaData.YRes();
		XnPoint3D* map = _backDepthMapRealWorld;
		for( uint32_t y=0; y<_depthMetaData.YRes(); y++ )
		{
			for(uint32_t x=0; x<_depthMetaData.XRes(); x++ )
			{
				//if( *pDepth > 0 )
				{
					map->X = (float)x;
					map->Y = (float)y;
					map->Z = (float)(*pDepth);
				}
				//else
				//{
				//	map->X = 0;
				//	map->Y = 0;
				//	map->Z = -9999;
				//}

				pDepth++;
				map++;
			}
		}

		// Convert all point into real world coordinates
		_status = _depthGen->ConvertProjectiveToRealWorld( bufSize, _backDepthMapRealWorld, worldData );
	}


    
    // Map our depth map values to be between Near and Far Cut Planes
    void OpenNIDevice::remapDepthMap( uint16_t* newDepthMap, uint16_t depthExtraScale, bool invertDepth )
    {
		const XnDepthPixel* pDepth = _depthMetaData.Data();
		//const uint32_t bufSize = _depthMetaData.XRes() * _depthMetaData.YRes();
        
        uint16_t* map = newDepthMap;

		double range = static_cast<double>( mFarClipPlane-mNearClipPlane );
        
		//float nearZ = 99999, farZ = 0;
		for(uint32_t y=0; y<_depthMetaData.YRes(); y++ )
		for(uint32_t x=0; x<_depthMetaData.XRes(); x++ )
		{
			double newDepth = 0.0;

            double depth = static_cast<double>( *pDepth );

			if( depth < mNearClipPlane )
				depth = mNearClipPlane;
			if( depth > mFarClipPlane )
				depth = mFarClipPlane;

			//XnPoint3D depthPoint;
			//depthPoint.X = (float)x;
			//depthPoint.Y = (float)y;
			//depthPoint.Z = (float)depth;
			//_depthGen->ConvertProjectiveToRealWorld( 1, &depthPoint, &depthPoint );

			//if( depthPoint.Z < nearZ ) 
			//	nearZ = depthPoint.Z;
			//if( depthPoint.Z > farZ ) 
			//	farZ = depthPoint.Z;

			//newDepth = ( depthPoint.Z - mNearClipPlane ) / range;
			newDepth = ( depth - mNearClipPlane ) / range;

			//newDepth = sqrt( newDepth * 4 );
			newDepth = newDepth * newDepth;
			newDepth *= depthExtraScale;

			if( newDepth > 1.0 )
				newDepth = 1.0;

			if( invertDepth )
				newDepth = 1.0 - newDepth;

			newDepth *= 65536.0;

			if( newDepth > 65536.0 )
				newDepth = 65536.0;

            *map = static_cast<uint16_t>( newDepth );

            map ++;
			pDepth ++;
		}

		//char buf[256];
		//sprintf( buf, "%f   %f\n", nearZ, farZ );
		//OutputDebugStringA( buf );
    }



	void OpenNIDevice::calculateHistogram()
	{
		if( !_isDepthOn )	
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

		float invNOP = 1.0f / (float)(nNumberOfPoints);
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
			_primaryGen = _imageGen;
		else if( _irGen && (type & NODE_TYPE_IR) )	
			_primaryGen = _irGen;
		else if( _isDepthOn && (type & NODE_TYPE_DEPTH) )	
			_primaryGen = _depthGen;
		else if( _userGen && (type & NODE_TYPE_USER) )	
			_primaryGen = _userGen;
		//else if( _audioGen && (type & NODE_TYPE_AUDIO) )	
			//_primaryGen = _audioGen;
		else 
			_primaryGen = NULL;
	}


	void OpenNIDevice::setMirrorMode( int type, bool flag )
	{ 
		if( _isImageOn && (type & NODE_TYPE_IMAGE) )	
			_imageGen->GetMirrorCap().SetMirror( flag ); 
		else if( _irGen && (type & NODE_TYPE_IR) )	
			_irGen->GetMirrorCap().SetMirror( flag ); 
		else if( _isDepthOn && (type & NODE_TYPE_DEPTH) )	
			_depthGen->GetMirrorCap().SetMirror( flag ); 
	}



	void OpenNIDevice::setDepthInvert( bool flag )
	{
		_isDepthInverted = flag;
	}

	uint8_t* OpenNIDevice::getColorMap()
	{
		return mColorSurface->getData();
	}

	//boost::uint16_t* OpenNIDevice::getIRMap()
	//{
	//	return _irData;
	//}

	//boost::uint8_t* OpenNIDevice::getIRMap8i()
	//{
	//	return _irData8;
	//}


	uint16_t* OpenNIDevice::getRawDepthMap()
	{
		return _backDepthData;
	}


	uint16_t* OpenNIDevice::getDepthMap()
	{
		return _depthData;
	}

	uint8_t* OpenNIDevice::getDepthMap24()
	{
		return _depthDataRGB;
	}


	XnPoint3D* OpenNIDevice::getDepthMapRealWorld()
	{
		return _depthMapRealWorld;
	}

	void OpenNIDevice::setSkeletonSmoothing( float value )
	{
		if( _userGen && _userGen->IsValid() )
			_userGen->GetSkeletonCap().SetSmoothing( value );
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

}	// Namespace V