#pragma once

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Vector.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"

#include "VOpenNIHeaders.h"

namespace V {
    
class Kinect : public UserListener {
public:
    Kinect();
    ~Kinect();
    
    virtual void setup();
    virtual void setup(const ci::Vec2i & size);
    virtual void setup(const ci::Vec2i & size, int nodeTypeFlags);
    virtual void setup(const ci::Vec2i & _depthSize, const ci::Vec2i & _colorSize, int nodeTypeFlags);
    void update();
    
    void drawColor(const ci::Rectf &rect);
    void drawDepth(const ci::Rectf &rect);
    void drawUser(const int id, const ci::Rectf &rect);
    void drawSkeletons(const ci::Rectf &rect, float depth=0.f, float pointRadius=3.f, bool renderDepth=false);
    
    void onNewUser( UserEvent ev );
    void onLostUser( UserEvent ev );
    void onUserExit( UserEvent ev );
    void onUserReEnter( UserEvent ev );

    
    ci::ImageSourceRef getColorImage();
    ci::ColorA8u getColorPixel(ci::Vec2i pixel);
    ci::ImageSourceRef getUserImage(const int id);
    ci::ImageSourceRef getDepthImage();
    XnPoint3D * getDepthMapRealWorld();
    
    ci::gl::Texture * getColorTexture() { return &tex_Color; };
    ci::gl::Texture * getDepthTexture() { return &tex_Depth; };
    
    ci::Vec2i getDepthSize() { return depthSize; };
    ci::Vec2i getColorSize() { return colorSize; };
    
    OpenNIDevice::Ref getDevice() { return device; };
    
    class User {
    public:
        User(Kinect & kinect, const int id) :
        kinect(kinect),
        id(id),
        texture(ci::gl::Texture(kinect.depthSize.x, kinect.depthSize.y))
        {};

        ci::ImageSourceRef getImage() const;
        OpenNIUserRef getOpenNIUser() const;
        ci::Vec3f getCenterOfMass( bool doProjectiveCoords=false ) const;
        
        int id;
        Kinect & kinect;
        ci::gl::Texture texture;
    };
    friend class User;
    
    typedef std::map< int, User > users_map;
    users_map users;
    
protected:
    OpenNIDeviceManager * manager;
    OpenNIDevice::Ref device;
    ci::gl::Texture tex_Color, tex_Depth;
    uint16_t * pixels;
    ci::Vec2i depthSize, colorSize;
    bool isDepthMapRealWorldUpdated;

public:
    class ImageSourceColor : public ci::ImageSource
    {
    public:
        ImageSourceColor( uint8_t *buffer, int width, int height )
        : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ImageIo::CM_RGB );
            setChannelOrder( ImageIo::RGB );
            setDataType( ImageIo::UINT8 );
        }
        
        ~ImageSourceColor()
        {
            // mData is actually a ref. It's released from the device.
            /*if( mData ) {
             delete[] mData;
             mData = NULL;
             }*/
        }
        
        virtual void load( ci::ImageTargetRef target )
        {
            ImageSource::RowFunc func = setupRowFunc( target );
            
            for( uint32_t row	 = 0; row < _height; ++row )
                ((*this).*func)( target, row, mData + row * _width * 3 );
        }
        
    protected:
        uint32_t					_width, _height;
        uint8_t						*mData;
    };
    
    
    class ImageSourceDepth : public ci::ImageSource
    {
    public:
        ImageSourceDepth( uint16_t *buffer, int width, int height )
        : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
        {
            setSize( _width, _height );
            setColorModel( ci::ImageIo::CM_GRAY );
            setChannelOrder( ci::ImageIo::Y );
            setDataType( ci::ImageIo::UINT16 );
        }
        
        ~ImageSourceDepth()
        {
            // mData is actually a ref. It's released from the device.
            /*if( mData ) {
             delete[] mData;
             mData = NULL;
             }*/
        }
        
        virtual void load( ci::ImageTargetRef target )
        {
            ci::ImageSource::RowFunc func = setupRowFunc( target );
            
            for( uint32_t row = 0; row < _height; ++row )
                ((*this).*func)( target, row, mData + row * _width );
        }
        
    protected:
        uint32_t					_width, _height;
        uint16_t					*mData;
    };
};
} // end namespace V