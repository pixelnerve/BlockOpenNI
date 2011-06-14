#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "KinectImageSource.h"
#include "VOpenNIHeaders.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MultipleKinectsApp : public AppBasic 
{
public:

	static const int	KINECT_WIDTH = 640;
	static const int	KINECT_HEIGHT = 480;

public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();


	ImageSourceRef getColorImage( V::OpenNIDeviceRef device )
	{
		// register a reference to the active buffer
		uint8_t *activeColor = device->getColorMap();
		return ImageSourceRef( new ImageSourceKinectColor( activeColor, KINECT_WIDTH, KINECT_HEIGHT ) );
	}

	ImageSourceRef getDepthImage( V::OpenNIDeviceRef device )
	{
		// register a reference to the active buffer
		uint16_t *activeDepth = device->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_WIDTH, KINECT_HEIGHT ) );
	} 



	//
	// Members
	//

	gl::Texture				mColorTex0, mDepthTex0;
	gl::Texture				mColorTex1, mDepthTex1;

	V::OpenNIDeviceManager*	mManager;
	V::OpenNIDeviceRef		mDevice0, mDevice1;


	double					mPrevTime, mCurrTime, mFrameTime;
};

void MultipleKinectsApp::setup()
{
	// Init openni and devices
	mManager = V::OpenNIDeviceManager::InstancePtr();
	mManager->Init();	// Init context
	// Init 2 devices with image/depth generators. NOTE: User generator not working properly. TODO: Fix
	mManager->createDevices( 2, V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH );

	// Create device 0 is available
	try 
	{
		mDevice0 = mManager->getDevice( 0 );
		mDevice0->setDepthShiftMul( 4 );	// scale depth values 
		mDevice0->setMirrorMode( V::NODE_TYPE_IMAGE, true );
		mDevice0->setMirrorMode( V::NODE_TYPE_DEPTH, true );
	} catch( std::exception e )
	{
		app::console() << e.what() << std::endl;
	}

	// Create device 1 is available
	try 
	{
		mDevice1 = mManager->getDevice( 1 );
		mDevice1->setDepthShiftMul( 4 );
		mDevice1->setMirrorMode( V::NODE_TYPE_IMAGE, true );
		mDevice1->setMirrorMode( V::NODE_TYPE_DEPTH, true );
	} catch( std::exception e )
	{
		app::console() << e.what() << std::endl;
	}

	// Create textures
	mColorTex0 = gl::Texture( KINECT_WIDTH, KINECT_HEIGHT );
	mDepthTex0 = gl::Texture( KINECT_WIDTH, KINECT_HEIGHT );
	mColorTex1 = gl::Texture( KINECT_WIDTH, KINECT_HEIGHT );
	mDepthTex1 = gl::Texture( KINECT_WIDTH, KINECT_HEIGHT );

	// Start device capture
	mManager->start();

	mPrevTime = mCurrTime = mFrameTime = 0;
}

void MultipleKinectsApp::mouseDown( MouseEvent event )
{
}

void MultipleKinectsApp::update()
{
	// Update textures
	if( mDevice0 )
	{
		mColorTex0.update( getColorImage(mDevice0) );
		mDepthTex0.update( getDepthImage(mDevice0) );
	}
	if( mDevice1 )
	{
		mColorTex1.update( getColorImage(mDevice1) );
		mDepthTex1.update( getDepthImage(mDevice1) );
	}
}

void MultipleKinectsApp::draw()
{
	// Compute frametime
	mPrevTime = mCurrTime;
	mCurrTime = app::getElapsedSeconds();
	mFrameTime = mCurrTime - mPrevTime;
	app::console() << mFrameTime << std::endl;

	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 


	// Show textures
	gl::setMatricesWindow( app::getWindowWidth(), app::getWindowHeight() );
	if( mDevice0 )
	{
		gl::draw( mColorTex0, Rectf(0, 0, KINECT_WIDTH/2, KINECT_HEIGHT/2) );
		gl::draw( mDepthTex0, Rectf(KINECT_WIDTH/2, 0, KINECT_WIDTH, KINECT_HEIGHT/2) );
	}
	if( mDevice1 )
	{
		gl::draw( mColorTex1, Rectf(0, KINECT_HEIGHT/2, KINECT_WIDTH/2, KINECT_HEIGHT) );
		gl::draw( mDepthTex1, Rectf(KINECT_WIDTH/2, KINECT_HEIGHT/2, KINECT_WIDTH, KINECT_HEIGHT) );
	}
}


CINDER_APP_BASIC( MultipleKinectsApp, RendererGl )
