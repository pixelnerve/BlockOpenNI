/***
	SkeletonApp

	A sample app showing skeleton rendering with the kinect and openni.
	This sample renders only the user with id=1. If user has another id he won't be displayed.
	You may change that in the code.

	V.
***/


#include "cinder/app/AppBasic.h"
#include "cinder/imageio.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "VOpenNIHeaders.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class ImageSourceKinectColor : public ImageSource 
{
public:
	ImageSourceKinectColor( uint8_t *buffer, int width, int height )
		: ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_RGB );
		setChannelOrder( ImageIo::RGB );
		setDataType( ImageIo::UINT8 );
	}

	~ImageSourceKinectColor()
	{
		// mData is actually a ref. It's released from the device. 
		/*if( mData ) {
			delete[] mData;
			mData = NULL;
		}*/
	}

	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );

		for( uint32_t row	 = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width * 3 );
	}

protected:
	uint32_t					_width, _height;
	uint8_t						*mData;
};


class ImageSourceKinectDepth : public ImageSource 
{
public:
	ImageSourceKinectDepth( uint16_t *buffer, int width, int height )
		: ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_GRAY );
		setChannelOrder( ImageIo::Y );
		setDataType( ImageIo::UINT16 );
	}

	~ImageSourceKinectDepth()
	{
		// mData is actually a ref. It's released from the device. 
		/*if( mData ) {
			delete[] mData;
			mData = NULL;
		}*/
	}

	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );

		for( uint32_t row = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width );
	}

protected:
	uint32_t					_width, _height;
	uint16_t					*mData;
};


class BlockOpenNISampleAppApp : public AppBasic, V::UserListener
{
public:
	static const int WIDTH = 800;
	static const int HEIGHT = 600;

	static const int KINECT_COLOR_WIDTH = 640;	//1280;
	static const int KINECT_COLOR_HEIGHT = 480;	//1024;
	static const int KINECT_COLOR_FPS = 30;	//15;
	static const int KINECT_DEPTH_WIDTH = 640;
	static const int KINECT_DEPTH_HEIGHT = 480;
	static const int KINECT_DEPTH_FPS = 30;


	BlockOpenNISampleAppApp();
	~BlockOpenNISampleAppApp();
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void keyDown( KeyEvent event );

	void onNewUser( V::UserEvent event );
	void onLostUser( V::UserEvent event );


	ImageSourceRef getColorImage()
	{
		// register a reference to the active buffer
		uint8_t *activeColor = _device0->getColorMap();
		return ImageSourceRef( new ImageSourceKinectColor( activeColor, KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT ) );
	}

	ImageSourceRef getUserImage( int id )
	{
		_device0->getLabelMap( id, pixels );
		return ImageSourceRef( new ImageSourceKinectDepth( pixels, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}

	ImageSourceRef getDepthImage()
	{
		// register a reference to the active buffer
		uint16_t *activeDepth = _device0->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	} 

	ImageSourceRef getDepthImage24()
	{
		// register a reference to the active buffer
		uint8_t *activeDepth = _device0->getDepthMap24();
		return ImageSourceRef( new ImageSourceKinectColor( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}
	void prepareSettings( Settings *settings );
public:	// Members
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;

	gl::Texture				mColorTex;
	gl::Texture				mDepthTex;
	gl::Texture				mOneUserTex;	 
	
	uint16_t*				pixels;
};



BlockOpenNISampleAppApp::BlockOpenNISampleAppApp() {}
BlockOpenNISampleAppApp::~BlockOpenNISampleAppApp()
{
	if( pixels )
	{
		delete[] pixels;
		pixels = NULL;
	}
}

void BlockOpenNISampleAppApp::prepareSettings( Settings *settings )
{
	settings->setFrameRate( 120 );
	settings->setWindowSize( WIDTH, HEIGHT );
	settings->setFullScreenSize( WIDTH, HEIGHT );
	settings->setTitle( "Heineken Streaks" );
	//settings->setShouldQuit( true );
	//settings->setFullScreen( true );
}


void BlockOpenNISampleAppApp::setup()
{
	_manager = V::OpenNIDeviceManager::InstancePtr();

#if defined(CINDER_MSW) || defined(CINDER_LINUX)
	string xmlpath = getAppPath() + ("resources/".append("configIR.xml"));
#elif defined(CINDER_MAC) || defined(CINDER_COCOA) || defined(CINDER_COCOA_TOUCH)				
	string xmlpath = getAppPath() + "/Contents/Resources/configIR.xml";
#endif
	
	// console() << "Loading config xml:" << xmlpath << std::endl;
	_device0 = _manager->createDevice( xmlpath, true );
	//_device0 = _manager->createDevice( V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH | V::NODE_TYPE_USER );	// Create manually.
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App)  Couldn't init device0\n" );
		exit( 0 );
	}
	_device0->setHistogram( true );	// Enable histogram depth map (RGB8bit bitmap)
	_manager->start();


	pixels = NULL;
	pixels = new uint16_t[ KINECT_DEPTH_WIDTH*KINECT_DEPTH_HEIGHT ];

	gl::Texture::Format format;
	gl::Texture::Format depthFormat;
	mColorTex = gl::Texture( KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT, format );
	mDepthTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
	mOneUserTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT, format );
}

void BlockOpenNISampleAppApp::mouseDown( MouseEvent event )
{
}

void BlockOpenNISampleAppApp::update()
{	
	// Update textures
	mColorTex.update( getColorImage() );
	mDepthTex.update( getDepthImage24() );	// Histogram

	// Uses manager to handle users.
	if( _manager->hasUser(1) ) 
		mOneUserTex.update( getUserImage(1) );
}

void BlockOpenNISampleAppApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ), true ); 


	gl::setMatricesWindow( WIDTH, HEIGHT );

	gl::disableDepthWrite();
	gl::disableDepthRead();

	float sx = 320/2;
	float sy = 240/2;
	float xoff = 10;
	float yoff = 10;
	glEnable( GL_TEXTURE_2D );
	gl::color( cinder::ColorA(1, 1, 1, 1) );
	if( _manager->hasUsers() && _manager->hasUser(1) ) gl::draw( mOneUserTex, Rectf( 0, 0, WIDTH, HEIGHT) );
	gl::draw( mDepthTex, Rectf( xoff, yoff, xoff+sx, yoff+sy) );
	gl::draw( mColorTex, Rectf( xoff+sx*1, yoff, xoff+sx*2, yoff+sy) );


	if( _manager->hasUser(1) )
	{
		gl::disable( GL_TEXTURE_2D );
		// Render skeleton if available
		_manager->renderJoints( WIDTH, HEIGHT, 0, 3, false );

		// Get list of available bones/joints
		// Do whatever with it
		//V::UserBoneList boneList = _manager->getUser(1)->getBoneList();
	}
}




void BlockOpenNISampleAppApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE )
	{
		this->quit();
		this->shutdown();
	}
}

void BlockOpenNISampleAppApp::onNewUser( V::UserEvent event )
{
	app::console() << "New User Added With ID: " << event.mId << std::endl;
}

void BlockOpenNISampleAppApp::onLostUser( V::UserEvent event )
{
	app::console() << "User Lost With ID: " << event.mId << std::endl;
}

CINDER_APP_BASIC( BlockOpenNISampleAppApp, RendererGl )
