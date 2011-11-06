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

	static const int KINECT_COLOR_WIDTH = 640;	//1280;
	static const int KINECT_COLOR_HEIGHT = 480;	//1024;
	static const int KINECT_COLOR_FPS = 30;	//15;
	static const int KINECT_DEPTH_WIDTH = 640;
	static const int KINECT_DEPTH_HEIGHT = 480;
	static const int KINECT_DEPTH_FPS = 30;


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

	ImageSourceRef getDepthImage24()
	{
		// register a reference to the active buffer
		uint8_t *activeDepth = _device0->getDepthMap24();
		return ImageSourceRef( new ImageSourceKinectColor( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}

public:	// Members
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;

	gl::Texture				mColorTex;
	gl::Texture				mDepthTex;
	 
};


void BlockOpenNISampleAppApp::setup()
{
	V::OpenNIDeviceManager::USE_THREAD = false;
	_manager = V::OpenNIDeviceManager::InstancePtr();
    _manager->createDevices( 1, V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH );
	_device0 = _manager->getDevice( 0 );
	if( !_device0 ) 
	{
		shutdown();
		quit();
		return;
	}
    _device0->setDepthInvert( false );
    _device0->setDepthShiftMul( 4 );
	if( !_device0 )
	{
		DEBUG_MESSAGE( "(App)  Can't find a kinect device\n" );
        this->shutdown();
        this->quit();
	}

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


	float sx = 320;
	float sy = 240;
	float xoff = 10;
	float yoff = 10;
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
