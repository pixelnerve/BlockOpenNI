#include "VOpenNICommon.h"
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <direct.h>
#endif
#include "VOpenNIBone.h"
#include "VOpenNIUser.h"
#include "VOpenNINetwork.h"
#include "VOpenNIDevice.h"
#include "VOpenNIDeviceManager.h"
#include <iostream>

namespace V
{
	using namespace xn;

	/*
	   Device Manager
	*/

	bool OpenNIDeviceManager::USE_THREAD = true;
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
		mMaxNumOfUsers = 20;
		mNumOfUsers = 0;
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
		Release();
	}

	
	void OpenNIDeviceManager::Init()
	{
		XnStatus status = _context.Init();
		CHECK_RC( status, "Cant init context" );
		
		XnCallbackHandle hDummy;
		_context.RegisterToErrorStateChange( onErrorStateChanged, NULL, hDummy );
		
		mIsContextInit = true;
	}

	
/***
	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( const std::string& xmlFile, bool allocUserIfNoNode )
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
		boost::shared_ptr<OpenNIDevice> dev = boost::shared_ptr<OpenNIDevice>( new OpenNIDevice( 0, this) );
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
			return OpenNIDeviceRef();

		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();

		// Make sure we have a device connected
		xn::NodeInfoList deviceList;
		//xn::EnumerationErrors errors;
		XnStatus status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEVICE, NULL, deviceList, NULL );
		CHECK_RC( status, "No kinect device was found" );
		if( status != XN_STATUS_OK )
		{
			return OpenNIDeviceRef();            
		}

		
		OpenNIDeviceRef dev = OpenNIDeviceRef( new OpenNIDevice( 0, this) );
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
***/


	V::OpenNIDeviceRef OpenNIDeviceManager::createDevice( int nodeTypeFlags, int colorResolution, int depthResolution )
	{
		createDevices( 1, nodeTypeFlags, colorResolution, depthResolution );
		return getDevice( 0 );
	}

	void OpenNIDeviceManager::createDevices( uint32_t deviceCount, int nodeTypeFlags, int colorResolution, int depthResolution )
	{
		if( deviceCount <= 0 || !mDevices.empty() ) 
			return;

		// Make sure we initialize our context
		if( !mIsContextInit ) 
			Init();

		XnStatus status;

		// Count max devices connected to this machine
		xn::NodeInfoList device_nodes; 
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEVICE, NULL, device_nodes, NULL );
		for( xn::NodeInfoList::Iterator nodeIt=device_nodes.Begin(); nodeIt!=device_nodes.End(); ++nodeIt ) 
		{
			const xn::NodeInfo& info = *nodeIt;
			const XnProductionNodeDescription& description = info.GetDescription(); 
		
			// create a query to depend on this node
			status = mQuery.AddNeededNode( info.GetInstanceName() ); 

			std::stringstream ss;
			ss << "Device Name: " << description.strName << 
				"  Vendor: " << description.strVendor << 
				info.GetInstanceName() << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			
			mDeviceCount++;
		}
		
		
		if( mDeviceCount == 0 )
		{
			_context.Release();
			DEBUG_MESSAGE( "No device is present" );
			return;
		}
		
		
		/*
		//
		// List all devices present
		//
		for( xn::NodeInfoList::Iterator nodeIt=device_nodes.Begin(); 
			nodeIt!=device_nodes.End(); 
			nodeIt++ ) 
		{
			xn::Device device;

			xn::NodeInfo deviceNode = *nodeIt;
			status = _context.CreateProductionTree( deviceNode, device );
			CHECK_RC( status, "DeviceCreation" );
			//status = deviceNode.GetInstance( device );
			//CHECK_RC( status, "DeviceCreation" );

			mDevicesList.push_back( device );
		}*/


		// Make sure we do not allocate more than the ones needed
		if( mDeviceCount > deviceCount ) 
			mDeviceCount = deviceCount;


		// Allocate devices
		for( uint32_t i=0; i<mDeviceCount; i++ )
		{            
			OpenNIDeviceRef dev = OpenNIDeviceRef( new OpenNIDevice( i, this) );
			mDevices.push_back( dev );
//			SDevice::Ref sdev = SDevice::Ref( new SDevice() );
//			mDeviceList.push_back( sdev );
		}


		// Allocate all generators we ask for
		xn::NodeInfoList image_nodes; 
		xn::NodeInfoList ir_nodes; 
		xn::NodeInfoList depth_nodes;
		xn::NodeInfoList user_nodes; 
		xn::NodeInfoList scene_nodes; 
		xn::NodeInfoList hands_nodes; 
		//xn::NodeInfoList audio_nodes; 
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_IMAGE, NULL, image_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_IR, NULL, ir_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_DEPTH, NULL, depth_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_USER, NULL, user_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_SCENE, NULL, scene_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		status = _context.EnumerateProductionTrees( XN_NODE_TYPE_HANDS, NULL, hands_nodes, NULL );
		CHECK_RC( status, "Enumeration Tree" );
		//_status = _context.EnumerateProductionTrees( XN_NODE_TYPE_AUDIO, NULL, audio_nodes, NULL );


		
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=depth_nodes.Begin(); nodeIt!=depth_nodes.End(); nodeIt++ ) 
			{
				if( mDepthGenCount >= mDeviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;
				
				xn::DepthGenerator gen;
				//status = _context.CreateProductionTree( nodeInfo, gen ); 
				status = _context.CreateAnyProductionTree( XN_NODE_TYPE_DEPTH, &mQuery, gen ); 
				CHECK_RC( status, "(createDevices)  DepthGenerator" );
				/*status = _context.CreateProductionTree( nodeInfo ); 
				CHECK_RC( status, "(createDevices)  DepthGenerator" );
				status = nodeInfo.GetInstance( gen );
				CHECK_RC( status, "DepthGenerator" );*/
				mDepthGenList.push_back( gen );


				++devIt;
				mDepthGenCount++;
			}
		}
		std::cout << "+++ Depth nodes: \t" << mDepthGenCount << std::endl;


		
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=ir_nodes.Begin(); nodeIt!=ir_nodes.End(); ++nodeIt )
			{
				if( mIRGenCount >= mDeviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				xn::IRGenerator gen;
				status = _context.CreateAnyProductionTree( XN_NODE_TYPE_IR, &mQuery, gen ); 
				//status = _context.CreateProductionTree( nodeInfo, gen ); 
				CHECK_RC( status, "(createDevices) IRGenerator" );
				/*status = _context.CreateProductionTree( nodeInfo ); 
				CHECK_RC( status, "(createDevices)  IRGenerator" );
				status = nodeInfo.GetInstance( gen );
				CHECK_RC( status, "IRGenerator" );*/
				mIRGenList.push_back( gen );

				++devIt;
				mIRGenCount++;
			}
		}
		std::cout << "+++ IR nodes: \t" << mIRGenCount << std::endl;

		
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=image_nodes.Begin(); nodeIt!=image_nodes.End(); ++nodeIt ) 
			{
				if( mImageGenCount >= mDeviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				xn::ImageGenerator gen;
				status = _context.CreateAnyProductionTree( XN_NODE_TYPE_IMAGE, &mQuery, gen ); 
				//status = _context.CreateProductionTree( nodeInfo, gen ); 
				CHECK_RC( status, "(createDevices)  ImageGenerator" );
				/*status = _context.CreateProductionTree( nodeInfo );
				CHECK_RC( status, "(createDevices)  ImageGenerator" );
				status = nodeInfo.GetInstance( gen );
				CHECK_RC( status, "ImageGenerator" );*/
				mImageGenList.push_back( gen );

				++devIt;
				mImageGenCount++;
			}
		}
		std::cout << "+++ Image nodes: \t" << mImageGenCount << std::endl;


		if( nodeTypeFlags & NODE_TYPE_USER )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=user_nodes.Begin(); nodeIt!=user_nodes.End(); ++nodeIt ) 
			{
				if( mUserGenCount >= mDeviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				xn::UserGenerator gen;
				status = _context.CreateAnyProductionTree( XN_NODE_TYPE_USER, &mQuery, gen ); 
				CHECK_RC( status, "(createDevices)  UserGenerator" );
				//status = _context.CreateProductionTree( nodeInfo, gen ); 
				//CHECK_RC( status, "(createDevices)  UserGenerator" );
				/*status = _context.CreateProductionTree( nodeInfo ); 
				CHECK_RC( status, "(createDevices)  UserGenerator" );
				status = nodeInfo.GetInstance( gen );
				CHECK_RC( status, "UserGenerator" );*/
				mUserGenList.push_back( gen );

				++devIt;
				mUserGenCount++;
			}
		}
		std::cout << "+++ User nodes: \t" << mUserGenCount << std::endl;


		if( nodeTypeFlags & NODE_TYPE_SCENE )
		{
			OpenNIDeviceList::iterator devIt = mDevices.begin();
			for( xn::NodeInfoList::Iterator nodeIt=scene_nodes.Begin(); nodeIt!=scene_nodes.End(); ++nodeIt ) 
			{
				if( mSceneAnalyzerCount >= mDeviceCount )	
					break;

				OpenNIDevice::Ref dev = *devIt;
				xn::NodeInfo nodeInfo = *nodeIt;

				xn::SceneAnalyzer gen;
				status = _context.CreateAnyProductionTree( XN_NODE_TYPE_SCENE, &mQuery, gen ); 
				//status = _context.CreateProductionTree( nodeInfo, gen ); 
				CHECK_RC( status, "(createDevices)  SceneAnalyzer" );
				/*status = _context.CreateProductionTree( nodeInfo ); 
				CHECK_RC( status, "(createDevices)  SceneAnalyzer" );
				status = nodeInfo.GetInstance( gen );
				CHECK_RC( status, "SceneAnalyzer" );*/
				mSceneAnalyzerList.push_back( gen );

				++devIt;
				mSceneAnalyzerCount++;
			}
		}
		std::cout << "+++ Scene nodes: \t" << mSceneAnalyzerCount << std::endl;

		
		//if( nodeTypeFlags & NODE_TYPE_HANDS )
		//{
		//	OpenNIDeviceList::iterator devIt = mDevices.begin();
		//	for( xn::NodeInfoList::Iterator nodeIt=hands_nodes.Begin(); nodeIt!=hands_nodes.End(); ++nodeIt ) 
		//	{
		//		if( mHandsGenCount >= mDeviceCount )	
		//			break;

		//		OpenNIDevice::Ref dev = *devIt;
		//		xn::NodeInfo nodeInfo = *nodeIt;

		//		xn::HandsGenerator gen;
		//		status = _context.CreateProductionTree( nodeInfo, gen ); 
		//		CHECK_RC( status, "(createDevices)  HandsGenerator" );
		//		/*status = _context.CreateProductionTree( nodeInfo ); 
		//		CHECK_RC( status, "(createDevices)  HandsGenerator" );
		//		status = nodeInfo.GetInstance( gen );
		//		CHECK_RC( status, "HandsGenerator" );
		//		mHandsGenList.push_back( gen );*/

		//		++devIt;
		//		mHandsGenCount++;
		//	}
		//}
		//std::cout << "+++ Hands nodes: \t" << mHandsGenCount << std::endl;


		
		// Initialize devices
		for( OpenNIDeviceList::iterator it=mDevices.begin();
			it!=mDevices.end();
			++it )
		{
			bool result = (*it)->init( nodeTypeFlags, colorResolution, depthResolution );
			if( !result )
			{
				DEBUG_MESSAGE( "(OpenNIDeviceManager)  Failed to initialize device\n" );
				break;
			}
		}
	}



	void OpenNIDeviceManager::AllocateMem( uint32_t width, uint32_t height )
	{
		mDepthMap = new uint16_t[width*height];
		mColorMap = new uint8_t[width*height];
		std::stringstream ss;
		ss << width << "x" << height << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );
	}


	OpenNIDevice::Ref OpenNIDeviceManager::getDevice( uint32_t deviceIdx )
	{
		if( mDevices.empty() || deviceIdx >= mDevices.size() ) 
		{
			std::stringstream ss;
			ss << "[OpenNIDeviceManager]  Device '" << deviceIdx << "' is not available" << std::endl;
			DEBUG_MESSAGE( ss.str().c_str() );
			//throw std::exception( ss.str().c_str() );
			return OpenNIDevice::Ref();
		}

		//OpenNIDeviceList::iterator currDevice;
		//currDevice = std::find( mDevices.begin(), mDevices.end(), deviceIdx );
		//return *currDevice;        
		int count = 0;
		OpenNIDeviceList::iterator devIt = mDevices.begin();
		while( 1 )
		{
			if( count == deviceIdx )
				return *devIt;
			
			devIt++;
			count++;
		}
		return OpenNIDevice::Ref();
	}



	void OpenNIDeviceManager::Release( void )
	{
		this->stop();
		
		// Stop thread
		if( USE_THREAD )
		{
			if( _thread )
			{
				DEBUG_MESSAGE( "Stop running thread on device manager\n" );
				assert( _thread );
				DEBUG_MESSAGE( "Wait for thread to be done\n" );
				_thread->join();
				DEBUG_MESSAGE( "Delete thread's memory\n" );
				_thread.reset();                
			}
		}

		// stop generators		
		_context.StopGeneratingAll();

		//mPrimaryGen->StopGenerating();
		//mPrimaryGen->Release();

		for ( size_t i = 0; i < mDepthGenList.size(); i++ )
			mDepthGenList[ i ].Release();
		mDepthGenList.clear();

		for ( size_t i = 0; i < mImageGenList.size(); i++ )
			mImageGenList[ i ].Release();
		mImageGenList.clear();

		for ( size_t i = 0; i < mIRGenList.size(); i++ )
			mIRGenList[ i ].Release();
		mIRGenList.clear();

		for ( size_t i = 0; i < mSceneAnalyzerList.size(); i++ )
			mSceneAnalyzerList[ i ].Release();
		mSceneAnalyzerList.clear();

		for ( size_t i = 0; i < mUserGenList.size(); i++ )
			mUserGenList[ i ].Release();
		mUserGenList.clear();

		for ( size_t i = 0; i < mHandsGenList.size(); i++ )
			mHandsGenList[ i ].Release();
		mHandsGenList.clear();

//		mDepthMDList.clear();
//		mImageMDList.clear();
//		mIRMDList.clear();
//		mSceneMDList.clear();


		SAFE_DELETE_ARRAY( mDepthMap );
		SAFE_DELETE_ARRAY( mColorMap );		


		//mDeviceList.clear();

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
		
		if( USE_THREAD ) 
		{
			_mutex.lock();
			//boost::mutex::scoped_lock lock( _mutex );
		}

		// Currently works with first device only.
		OpenNIDeviceList::iterator it = mDevices.begin();
		OpenNIUserRef newUser = OpenNIUserRef( new OpenNIUser(id, it->get()) );
		mUserList.push_back( newUser );
		mNumOfUsers ++;
		
		
		if( USE_THREAD ) 
		{
			_mutex.unlock();
		}
		 
		
		return newUser;
	}

	void OpenNIDeviceManager::removeUser( uint32_t id )
	{
		if( USE_THREAD ) 
		{
			_mutex.lock();
			//boost::mutex::scoped_lock lock( _mutex );
		}

		// bail out is no users left
		if( mUserList.empty() ) return;
		
		// Remove user with id
		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			if( id == (*it)->getId() )
			{
				mUserList.remove( *it );
				mNumOfUsers --;
				return;
			}
		}
		
		
		if( USE_THREAD ) 
		{
			_mutex.unlock();
		}
		 

	}



	V::OpenNIUserRef OpenNIDeviceManager::getFirstUser()
	{
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.empty() ) return OpenNIUserRef();
		return (*mUserList.begin());
	}

	V::OpenNIUserRef OpenNIDeviceManager::getSecondUser()
	{
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
		//boost::mutex::scoped_lock lock( _mutex );

		return ( mUserList.empty() ) ? OpenNIUserRef() : (*mUserList.end());
	}

	OpenNIUserRef OpenNIDeviceManager::getUser( int id )
	{
		//boost::mutex::scoped_lock lock( _mutex );

		for( OpenNIUserList::iterator it = mUserList.begin(); it != mUserList.end(); ++it )
		{
			if( id == (*it)->getId() )
				return (*it);
		}

		return OpenNIUserRef();
	}


	bool OpenNIDeviceManager::hasUser( int32_t id )
	{
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
		//boost::mutex::scoped_lock lock( _mutex );
		return !mUserList.empty(); 
	}


	const uint32_t OpenNIDeviceManager::getNumOfUsers()
	{ 
		//boost::mutex::scoped_lock lock( _mutex );
		return (uint32_t)mUserList.size();	
	}
	
	OpenNIUserList OpenNIDeviceManager::getUserList()
	{ 
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


		// Start generators
		_context.StartGeneratingAll();
	}



	void OpenNIDeviceManager::update()
	{
		XnStatus rc = XN_STATUS_OK;
		if( mPrimaryGen )
		{
			rc = _context.WaitOneUpdateAll( *mPrimaryGen );
		}
		else
		{
			//rc = _context.WaitNoneUpdateAll();
			//rc = _context.WaitAndUpdateAll();
			rc = _context.WaitAnyUpdateAll();
		}
		CHECK_RC( rc, "WaitAndUpdateAll" );


		
		if( USE_THREAD ) 
		{
			_mutex.lock();
			//boost::mutex::scoped_lock lock( _mutex );
		}

		
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

		
		if( USE_THREAD ) 
		{
			_mutex.unlock();
		}

		
		// Sleep
//		if( USE_THREAD ) 
//          boost::this_thread::sleep( boost::posix_time::millisec(1) ); 
	}


	
	void OpenNIDeviceManager::renderJoints( float width, float height, float depth, float pointSize, bool renderDepth )
	{
		//boost::lock_guard<boost::recursive_mutex> lock( _rmutex );
		//boost::mutex::scoped_lock lock( _mutex );

		if( mUserList.empty() ) 
			return;
		
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

		if( !mDepthGenList[deviceIdx].IsValid() ) 
			return NULL;

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		return (uint16_t*)mDepthMD.Data();
	}


	uint16_t* OpenNIDeviceManager::getDepthMapShift( uint32_t deviceIdx, uint32_t shiftMul )
	{
		//if( USE_THREAD ) boost::mutex::scoped_lock lock( _mutex );

		if( !mDepthGenList[deviceIdx].IsValid() ) 
			return NULL;

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


	uint8_t* OpenNIDeviceManager::getColorMap( uint32_t deviceIdx )
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

			uint32_t arraySize = mIRMD.XRes()*mIRMD.YRes();

			if( !mColorMap ) 
				mColorMap = new uint8_t[arraySize*3];

			const XnDepthPixel* map = mIRMD.Data();
			uint8_t* dest = mColorMap;
			for( uint32_t i=0; i<arraySize; i++ )
			{
				uint32_t pixel = *map;
				uint8_t value = (pixel >> 1) & 0xff; 
				*dest++ = value;
				*dest++ = value;
				*dest++ = value;
				map++;
			}

			return mColorMap;
		}

		return NULL;
	}

	
	
	void OpenNIDeviceManager::GetUserMap( uint32_t deviceIdx, uint32_t labelId, uint16_t* labelMap )
	{
		//boost::mutex::scoped_lock lock( _mutex );
		
		SceneAnalyzer& scene = mSceneAnalyzerList[deviceIdx];
		// Make sure scene is available
		if( !scene.IsValid() ) 
			return;
		
		scene.GetMetaData( mSceneMD );
		const XnLabel* labels = mSceneMD.Data();
		
		if( labels )
		{
			int depthWidth = mSceneMD.XRes();
			int depthHeight = mSceneMD.YRes();
			
			memset( labelMap, 0, depthWidth*depthHeight*sizeof(uint16_t) );
			
			mDepthGenList[deviceIdx].GetMetaData( mDepthMD );
			const XnDepthPixel* pDepth = mDepthMD.Data();
			uint16_t* map = labelMap;
			
			if( labelId > 0 )
			{
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
			else
			{
				// Common user map
				for( int i=0; i<depthWidth*depthHeight; i++ )
				{
					XnLabel label = *labels;

					if( label == labelId )
						*map = *pDepth;

					map++;
					pDepth++;
					labels++;
				}
			}
		}
	}



	void OpenNIDeviceManager::SetPrimaryBuffer( uint32_t deviceIdx, uint32_t type )
	{
		if( (type & NODE_TYPE_IMAGE) )	
			mPrimaryGen = &mImageGenList[deviceIdx];
		else if( (type & NODE_TYPE_IR) )	
			mPrimaryGen = &mIRGenList[deviceIdx];
		else if( (type & NODE_TYPE_DEPTH) )	
			mPrimaryGen = &mDepthGenList[deviceIdx];
		else if( (type & NODE_TYPE_USER) )	
			mPrimaryGen = &mUserGenList[deviceIdx];
		else if( (type & NODE_TYPE_HANDS) )	
			mPrimaryGen = &mHandsGenList[deviceIdx];
		else 
			mPrimaryGen = NULL;
	}


	void OpenNIDeviceManager::CalcDepthImageRealWorld( uint32_t deviceIdx, uint16_t* pixelData, XnPoint3D* worldData )
	{
		//boost::mutex::scoped_lock lock( _mutex );

		mDepthGenList[deviceIdx].GetMetaData( mDepthMD );

		const XnDepthPixel* src = pixelData;
		XnPoint3D* destMap = worldData;

		/*for( uint32_t y=0; y<mDepthMD.YRes(); y++ )
		{
			for(uint32_t x=0; x<mDepthMD.XRes(); x++ )
			{*/
		
			uint32_t bufSize = mDepthMD.XRes() * mDepthMD.YRes();
			for(uint32_t i=0; i<bufSize; i++ )
			{
				int x = i % mDepthMD.XRes();
				int y = i / mDepthMD.XRes();
				if( *src > 0 )
				{
					destMap->X = (float)x;
					destMap->Y = (float)y;
					destMap->Z = (float)(*src);
				}
				else
				{
					destMap->X = 0;
					destMap->Y = 0;
					destMap->Z = -9999;
				}

				src++;
				destMap++;
			}
		//}

		// Convert all point into real world coordinates
		mDepthGenList[deviceIdx].ConvertProjectiveToRealWorld( bufSize, worldData, worldData );
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
								(float)-1.0f, 
								(float)-1.0f, 
								(float)-1.0f, 
								(float)-1.0f
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

}  // namespace