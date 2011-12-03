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


class BlockOpenNISampleAppApp : public AppBasic 
{
public:
	static const int WIDTH = 1280;
	static const int HEIGHT = 720;

	static const int KINECT_COLOR_WIDTH = 1280;
	static const int KINECT_COLOR_HEIGHT = 1024;
	static const int KINECT_DEPTH_WIDTH = 1280;
	static const int KINECT_DEPTH_HEIGHT = 1024;


	void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void keyDown( KeyEvent event );

	ImageSourceRef getColorImage()
	{
		// register a reference to the active buffer
		uint8_t *activeColor = _device0->getColorMap();
		return ImageSourceRef( new ImageSourceKinectColor( activeColor, KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT ) );
	}

	ImageSourceRef getDepthImage()
	{
		// register a reference to the active buffer
		uint16_t *activeDepth = _device0->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	} 

public:	// Members
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;

	gl::Texture				mColorTex;
	gl::Texture				mDepthTex;
	 
};



void BlockOpenNISampleAppApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( WIDTH, HEIGHT );
}


void BlockOpenNISampleAppApp::setup()
{
	V::OpenNIDeviceManager::USE_THREAD = false;
	_manager = V::OpenNIDeviceManager::InstancePtr();
	_manager->createDevices( 1, V::NODE_TYPE_DEPTH | V::NODE_TYPE_IMAGE, V::RES_1280x1024 );
	_device0 = _manager->getDevice( 0 );
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App)  Can't find a kinect device\n" );
		shutdown();
		quit();
		return;
	}
	_device0->setDepthInvert( false );
	_device0->setDepthShiftMul( 4 );

	mColorTex = gl::Texture( KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT );
	mDepthTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT );


	//_manager->SetPrimaryBuffer( 0, V::NODE_TYPE_DEPTH );
	_manager->start();
}


void BlockOpenNISampleAppApp::mouseDown( MouseEvent event )
{
}


void BlockOpenNISampleAppApp::update()
{
	if( !V::OpenNIDeviceManager::USE_THREAD )
	{
		_manager->update();
	}


	// Update textures
	mColorTex.update( getColorImage() );
	mDepthTex.update( getDepthImage() );
}


void BlockOpenNISampleAppApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 


	float sx = 640;
	float sy = 480;
	float xoff = 0;
	float yoff = 0;
	glEnable( GL_TEXTURE_2D );
	gl::color( cinder::ColorA(1, 1, 1, 1) );
	gl::draw( mDepthTex, Rectf( xoff, yoff, xoff+sx, yoff+sy) );
	gl::draw( mColorTex, Rectf( xoff+sx*1, yoff, xoff+sx*2, yoff+sy) );
}


void BlockOpenNISampleAppApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE )
	{
		this->quit();
		this->shutdown();
	}
}


CINDER_APP_BASIC( BlockOpenNISampleAppApp, RendererGl )
