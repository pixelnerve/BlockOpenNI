#pragma once

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>
#include "VOpenNICommon.h"
#include "VOpenNISurface.h"


namespace V
{






	/************************************************************************/
	/* Callbacks
	*/
	/************************************************************************/

	typedef void (*JoinCallback)( int deviceId );
	typedef void (*LeaveCallback)( int deviceId );


	int CreateDevice( JoinCallback join, LeaveCallback leave );
	void DestroyDevice( int device_id );
	void PollDevices();



	// Callback class
	class OpenNIDevice;
	class OpenNIDeviceCallback
	{
	public:
		typedef void (OpenNIDevice::*CallbackFunction)(int arg);
		OpenNIDeviceCallback( OpenNIDevice* obj, CallbackFunction function )
		{
			mCallbackObj = obj;
			mCallbackFunc = function;
		}
		virtual ~OpenNIDeviceCallback() {}

	public:
		OpenNIDevice* mCallbackObj;
		CallbackFunction mCallbackFunc;
	};






	/************************************************************************/
	/*                                                                      */
	/************************************************************************/

	class OpenNIUser;

	class OpenNIDevice
	{
	public:
		OpenNIDevice( xn::Context* context );
		~OpenNIDevice();
		bool init( boost::uint64_t nodeTypeFlags );
		bool initFromXmlFile( const std::string& xmlFile, bool allocUserIfNoNode=false );



		void CallbackFunc( int arg )
		{
			std::stringstream ss;
			ss << "--> Value is: " << arg << std::endl;
			OutputDebugStringA( ss.str().c_str() );
		}

		virtual void OnJoin() {}
		virtual void OnLeave() {}
		virtual void OnCalibrationStart() {}
		virtual void OnCalibrationEnd() {}
		virtual void OnPoseStart() {}
		virtual void OnPoseEnd() {}

		void OpenNIDevice::release();

		void OpenNIDevice::start();
		void OpenNIDevice::run();
		void OpenNIDevice::update();

		void OpenNIDevice::allocate( int flags, int width, int height );

		bool OpenNIDevice::requestUserCalibration();
		void OpenNIDevice::setResolution( ProductionNodeType nodeType, int res, int fps );
		void OpenNIDevice::setFPS( ProductionNodeType nodeType, int fps );
		void OpenNIDevice::setMapOutputMode( ProductionNodeType nodeType, int width, int height, int fps );
		void OpenNIDevice::readFrame();
		void OpenNIDevice::setPrimaryBuffer( int type );

		void OpenNIDevice::setDepthInvert( bool flag );
		bool OpenNIDevice::getDepthInvert()			{ return _isDepthInverted; }

		boost::uint8_t* getColorMap();
		boost::uint16_t* getIRMap();
		boost::uint8_t* getIRMap8i();
		boost::uint16_t* getDepthMap();
		boost::uint8_t* getDepthMap8i();
		boost::uint8_t* getDepthMap24();
		boost::uint16_t* getRawDepthMap();

		xn::ImageGenerator*	getImageGenerator()		{ return _imageGen;	}
		xn::IRGenerator* getIRGenerator()			{ return _irGen;	}
		xn::DepthGenerator*	getDepthGenerator()		{ return _depthGen;	}
		xn::UserGenerator* getUserGenerator()		{ return _userGen;	}
		xn::Context*	getContext()				{ return _context;	}

		const std::string& getDebugInfo()			{ return mDebugInfo;}

	public:
		static const bool		USE_THREAD;
		boost::shared_ptr<boost::thread> _thread;

		std::string				mDeviceName;
		std::string				mDebugInfo;

		bool					_isRunning;
		bool					_isDepthInverted;

		std::string				_configFile;

		xn::EnumerationErrors	_errors;
		XnStatus				_status;

		xn::Context*			_context;
		xn::Device				_device;

		XnFPSData				_fpsData;

		XnMapOutputMode			_mapMode; 


		// Buffers
		V::OpenNISurface8*		mColorSurface;
		V::OpenNISurface8*		mIRSurface;
		V::OpenNISurface16*		mDepthSurface;

		int						_bpp;
		//boost::uint8_t*			_colorData;
		boost::uint16_t*		_irData;
		boost::uint8_t*			_irData8;
		boost::uint16_t*		_depthData;
		boost::uint16_t*		_backDepthData;
		boost::uint8_t*			_depthData8;
		boost::uint8_t*			_depthDataRGB;

		XnRGB24Pixel*			g_pTexMap;
		int						g_MaxDepth;
		float*					g_pDepthHist;

		// Temp buffers
		const XnUInt8*			pImage;
		const XnDepthPixel*		pDepth;
		const XnIRPixel*		pIR;


		// Flags for nodes
		bool					_isImageOn;
		bool					_isIROn;
		bool					_isDepthOn;
		bool					_isUserOn;
		bool					_isAudioOn;

		xn::Generator*			_primaryGen;
		xn::DepthGenerator*		_depthGen;
		xn::ImageGenerator*		_imageGen;
		xn::IRGenerator*		_irGen;
		xn::UserGenerator*		_userGen;
		xn::AudioGenerator*		_audioGen;

		xn::ImageMetaData*		_imageMetaData;
		xn::IRMetaData*			_irMetaData;
		xn::DepthMetaData*		_depthMetaData;
		xn::SceneMetaData*		_sceneMetaData;
		xn::AudioMetaData*		_audioMetaData;

		// Users
		//std::vector<std::shared_ptr<OpenNIUser*>> mUserList;


		OpenNIDeviceCallback*		_callback;
	};





	/************************************************************************/
	/* Device Manager
	*/
	/************************************************************************/
	//typedef std::shared_ptr<std::vector<OpenNIDevice>> OpenNIDeviceList;
	typedef std::list<OpenNIUser*> OpenNIUserList;
	//std::vector<std::shared_ptr<OpenNIUser*>> mUserList;

	// A singleton
	class OpenNIDeviceManager : private boost::noncopyable
	{
	public:
		OpenNIDeviceManager();
		~OpenNIDeviceManager();

		boost::uint32_t OpenNIDeviceManager::enumDevices();
		OpenNIDevice* createDevice( const std::string& xmlFile="", bool allocUserIfNoNode=false );
		OpenNIDevice* createDevice( int nodeTypeFlags );
		void destroyDevice( OpenNIDevice* device );

		OpenNIUser* OpenNIDeviceManager::addUser( xn::UserGenerator* userGen, uint32_t id );
		void OpenNIDeviceManager::removeUser( OpenNIUser* user );
		void OpenNIDeviceManager::removeUser( uint32_t id );
		void OpenNIDeviceManager::destroyAll( void );
		void OpenNIDeviceManager::start();

		void OpenNIDeviceManager::update();
		void OpenNIDeviceManager::renderJoints( float pointSize );
		bool hasUsers()						{ return (mUserList.size()>0)?true:false; }
		OpenNIUser* OpenNIDeviceManager::getUser( int id );

		const std::string& getDebugInfo()			{ return mDebugInfo;}
		void setText( const std::string& info )	{ mDebugInfo = info; }


		//static void OpenNIDeviceManager::DeviceJoin( int deviceId );
		//static void OpenNIDeviceManager::DeviceLeave( int deviceId );



		//
		// Instance
		//
		static OpenNIDeviceManager& Instance() 
		{
			//if( !_singletonPointerRef )
				//_singletonPointerRef = std::shared_ptr<OpenNIDeviceManager>( new OpenNIDeviceManager() );
			//return *_singletonPointerRef.get();
			//if( !_singletonPointer ) 
			//	_singletonPointer = new OpenNIDeviceManager();
			return _singletonPointer;
		}
		static OpenNIDeviceManager* InstancePtr() 
		{
			//if( !_singletonPointerRef )
			//	_singletonPointerRef = std::shared_ptr<OpenNIDeviceManager>( new OpenNIDeviceManager() );
			//return _singletonPointerRef.get();
			//if( !_singletonPointer ) 
			//	_singletonPointer = new OpenNIDeviceManager();
			return &_singletonPointer;
		}
		/*static std::shared_ptr<OpenNIDeviceManager> InstanceRef() 
		{
			//if( !_singletonPointerRef )
			//	_singletonPointerRef = std::shared_ptr<OpenNIDeviceManager>( new OpenNIDeviceManager() );
			//return _singletonPointerRef;
			if( !_singletonPointer ) 
				_singletonPointer = new OpenNIDeviceManager();
			return &_singletonPointer;
		}*/

	private:
		// Copy constructor
		OpenNIDeviceManager( const OpenNIDeviceManager& ) {};
		// Operators
		OpenNIDeviceManager& operator = ( const OpenNIDeviceManager& ) {};
	protected:
		struct DeviceInfo
		{
			int				id;
			OpenNIDevice*	dev;
		};

		//static std::shared_ptr<OpenNIDeviceManager>		_singletonPointerRef;
		static OpenNIDeviceManager		_singletonPointer;
		//static OpenNIDeviceManager*		_singletonPointer;

		xn::Context						_context;

		std::string						mDebugInfo;

		int								_idCount;

		// Device list
		std::list<DeviceInfo*>			mDeviceList;
		//static std::list<DeviceInfo*>	mDeviceList;

		// Generic user list. This users have no knowledge of which device they come from
		OpenNIUserList					mUserList;
	};


	//typedef std::shared_ptr<OpenNIDeviceManager> OpenNIDeviceManagerRef;
}