/***
	SkeletonApp

	A sample app showing skeleton rendering with the kinect and openni.
	This sample renders only the user with id=1. If user has another id he won't be displayed.
	You may change that in the code.

	V.
***/


#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
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
		uint8_t *activeColor = _manager->getColorMap();
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
		uint16_t *activeDepth = _manager->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	} 

	void prepareSettings( Settings *settings );


public:	
    // Members
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;

	gl::Texture				mColorTex;
	gl::Texture				mDepthTex;
    std::map< int, gl::Texture> mUsersTexMap;
	
	uint16_t*				pixels;
    
    float                   startTime, endTime, frameTime;
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
	settings->setFrameRate( 60 );
	settings->setWindowSize( WIDTH, HEIGHT );
	settings->setFullScreenSize( WIDTH, HEIGHT );
	settings->setTitle( "BlockOpenNI Skeleton Sample" );
	//settings->setShouldQuit( true );
	//settings->setFullScreen( true );
}


void BlockOpenNISampleAppApp::setup()
{
    V::OpenNIDeviceManager::USE_THREAD = false;
	_manager = V::OpenNIDeviceManager::InstancePtr();
    _manager->createDevices( 1, V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH | V::NODE_TYPE_SCENE | V::NODE_TYPE_USER, V::RES_640x480 );	// Create manually.
	_device0 = _manager->getDevice( 0 );
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App)  Can't find a kinect device\n" );
        quit();
        shutdown();
	}
    _device0->addListener( this );

	pixels = NULL;
	pixels = new uint16_t[ KINECT_DEPTH_WIDTH*KINECT_DEPTH_HEIGHT ];

	mColorTex = gl::Texture( KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT );
	mDepthTex = gl::Texture( KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT );


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
    

	// Uses manager to handle users.
    for( std::map<int, gl::Texture>::iterator it=mUsersTexMap.begin();
        it != mUsersTexMap.end();
        ++it )
    {
        it->second.update( getUserImage( it->first ) );
    }
}


void BlockOpenNISampleAppApp::draw()
{
    endTime = startTime;
    startTime = getElapsedSeconds();
    frameTime = startTime - endTime;
    app::console() << frameTime << std::endl;
    
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ), true ); 


	gl::setMatricesWindow( WIDTH, HEIGHT );

	gl::disableDepthWrite();
	gl::disableDepthRead();

	float sx = 320/2;
	float sy = 240/2;
	float xoff = 10;
	float yoff = 10;
	//glEnable( GL_TEXTURE_2D );
	gl::color( cinder::ColorA(1, 1, 1, 1) );
	gl::draw( mDepthTex, Rectf( xoff, yoff, xoff+sx, yoff+sy) );
	gl::draw( mColorTex, Rectf( xoff+sx*1, yoff, xoff+sx*2, yoff+sy) );

    /*
    // Render all user textures
    int xpos = 5;
    int ypos = sy+10;
    for( std::map<int, gl::Texture>::iterator it=mUsersTexMap.begin();
            it != mUsersTexMap.end();
            ++it )
    {
        //int id = it->first;
        gl::Texture tex = it->second;
    
        gl::draw( tex, Rectf(xpos, ypos, xpos+sx, ypos+sy) );
        xpos += sx;
        if( xpos > (sx+10) )
        {
            xpos = 5;
            ypos += sy+10;
        }
    }
    

	if( _manager->hasUsers() )
	{
		gl::disable( GL_TEXTURE_2D );
		// Render skeleton if available
		_manager->renderJoints( WIDTH, HEIGHT, 0, 3, false );
	}*/
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
    mUsersTexMap.insert( std::make_pair( event.mId, gl::Texture(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT) ) );
}


void BlockOpenNISampleAppApp::onLostUser( V::UserEvent event )
{
	app::console() << "User Lost With ID: " << event.mId << std::endl;
    mUsersTexMap.erase( event.mId );
}


CINDER_APP_BASIC( BlockOpenNISampleAppApp, RendererGl )
