#include "VKinect.h"
#include "cinder/app/AppBasic.h"
#include <boost/foreach.hpp>

using namespace ci;
using namespace std;

namespace V{

Kinect::Kinect() :
pixels(NULL)
{
}

Kinect::~Kinect()
{
    delete [] pixels;
    pixels = NULL;
}

void Kinect::setup()
{
    setup(Vec2i(640, 480));
}

void Kinect::setup(const Vec2i & size)
{
    setup(size, size, NODE_TYPE_IMAGE | NODE_TYPE_DEPTH | NODE_TYPE_SCENE | NODE_TYPE_USER);
}

void Kinect::setup(const Vec2i & size, int nodeTypeFlags)
{
    setup(size, size, nodeTypeFlags);
}

    
void Kinect::setup(const Vec2i & _depthSize, const Vec2i & _colorSize, int nodeTypeFlags)
{
    depthSize = _depthSize;
    colorSize = _colorSize;
    
    OpenNIDeviceManager::USE_THREAD = false;
    
    manager = OpenNIDeviceManager::InstancePtr();
    manager->createDevices(1, nodeTypeFlags);
    
    device = manager->getDevice(0);
    if ( !device ) {
        app::console() << "Can't find a kinect device" << endl;
        throw 1;
    }
    device->setDepthShiftMul(3);
    
    device->addListener(this);
    
    pixels = new uint16_t[depthSize.x * depthSize.y];
    tex_Color = gl::Texture(colorSize.x, colorSize.y);
    tex_Depth = gl::Texture(depthSize.x, depthSize.y);
    
    manager->start();
}

void Kinect::update()
{
    if ( !OpenNIDeviceManager::USE_THREAD ) {
        manager->update();
    }
    
    if ( device->_isImageOn && device->getImageGenerator()->IsValid() && device->isImageDataNew() )
        tex_Color.update(Surface(getColorImage()));
    if ( device->_isDepthOn && device->getDepthGenerator()->IsValid() && device->isDepthDataNew() )
        isDepthMapRealWorldUpdated = false;
        tex_Depth.update(Surface(getDepthImage()));
    
    if ( device->_isUserOn && device->getUserGenerator()->IsValid() && device->isUserDataNew() ) {
        BOOST_FOREACH(users_map::value_type &user, users)
        {
            user.second.texture.update(Surface(getUserImage(user.first)));
        }
    }
}

void Kinect::drawColor( const Rectf & rect )
{
    gl::draw(tex_Color, rect);
}

void Kinect::drawDepth( const Rectf & rect )
{
    gl::draw(tex_Depth, rect);
}

void Kinect::drawUser( const int id, const Rectf & rect )
{
    users_map::iterator it = users.find(id);
    if ( it != users.end() ) {
        gl::draw(it->second.texture, rect);
    } else {
        app::console() << "Could not draw user with id: " << id << endl;
    }
}

void Kinect::drawSkeletons( const Rectf & rect, float depth, float pointRadius, bool renderDepth )
{
    if( !manager->hasUsers() ) return;
    
    gl::disable( GL_TEXTURE_2D );
    
    gl::pushMatrices();
    gl::translate(Vec2f(rect.x1, rect.y1));
    manager->renderJoints(rect.getWidth(),
                          rect.getHeight(),
                          depth,
                          pointRadius,
                          renderDepth);
    gl::popMatrices();
}

void Kinect::onNewUser( UserEvent ev )
{
    users.insert(make_pair(ev.mId, User(*this, ev.mId)));
}


void Kinect::onLostUser( UserEvent ev )
{
    users.erase(ev.mId);
}

void Kinect::onUserExit( V::UserEvent ev )
{
}

void Kinect::onUserReEnter( V::UserEvent ev )
{
}


ImageSourceRef Kinect::getColorImage()
{
    // register a reference to the active buffer
    uint8_t *activeColor = device->getColorMap();
    return ImageSourceRef(new ImageSourceColor(activeColor, colorSize.x, colorSize.y));
}

ImageSourceRef Kinect::getUserImage( const int id )
{
    device->getLabelMap(id, pixels);
    return ImageSourceRef(new ImageSourceDepth(pixels, depthSize.x, depthSize.y));
}

ImageSourceRef Kinect::getDepthImage()
{
    // register a reference to the active buffer
    uint16_t *activeDepth = device->getDepthMap();
    return ImageSourceRef(new ImageSourceDepth(activeDepth, depthSize.x, depthSize.y));
}
    
XnPoint3D * Kinect::getDepthMapRealWorld()
{
    if ( isDepthMapRealWorldUpdated ) {
        return device->getDepthMapRealWorld();
    } else {
        device->calcDepthImageRealWorld();
        isDepthMapRealWorldUpdated = true;
        return device->getDepthMapRealWorld();
    }
}
    
ColorA8u Kinect::getColorPixel(Vec2i pixel)
{
    uint8_t *r = device->getColorMap() + (pixel.x + pixel.y * colorSize.y);
    return ColorA8u(*r, *(r + 1), *(r + 2));
}


/*******************************************************************************
 * Kinect::User
 */

ImageSourceRef Kinect::User::getImage() const
{
    return kinect.getUserImage(id);
}

OpenNIUserRef Kinect::User::getOpenNIUser() const
{
    return kinect.manager->getUser(id);
}

Vec3f Kinect::User::getCenterOfMass( bool doProjectiveCoords ) const
{
    float * center = getOpenNIUser()->getCenterOfMass(doProjectiveCoords);
    return ci::Vec3f(center[0], center[1], center[2]);
};

} // end namespace V