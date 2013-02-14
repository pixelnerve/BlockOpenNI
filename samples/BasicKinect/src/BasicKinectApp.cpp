#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "VKinect.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace V;

class BasicKinectApp : public AppBasic {
public:
    void prepareSettings( Settings *settings );
	void setup();
	void mouseDown( MouseEvent event );
	void update();
	void draw();
    
private:
    Kinect kinect;
};

void BasicKinectApp::prepareSettings( Settings *settings )
{
}

void BasicKinectApp::setup()
{
    
    try {
        kinect.setup();
    } catch ( int e ) {
        console() << "No kinect. Exit sadface." << endl;
        quit();
    }
    
}

void BasicKinectApp::mouseDown( MouseEvent event )
{
}

void BasicKinectApp::update()
{
    kinect.update();
}

void BasicKinectApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
    kinect.drawSkeletons(Rectf(0, 0, getWindowWidth(), getWindowHeight()));
    
}


CINDER_APP_BASIC( BasicKinectApp, RendererGl )
