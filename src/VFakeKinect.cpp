#include "VFakeKinect.h"

using namespace ci;
using namespace std;

namespace V {

    void FakeKinect::setup(const ci::Vec2i & _depthSize, const ci::Vec2i & _colorSize, int nodeTypeFlags)
    {
        depthSize = _depthSize;
        colorSize = _colorSize;
        depthMapRealWorld = new XnPoint3D[depthSize.x * depthSize.y];
        
        Surface8u colorSurface = Surface8u(colorSize.x, colorSize.y, false);
        Surface::Iter it = colorSurface.getIter();
        while ( it.line() ) {
            while ( it.pixel() ) {
                Vec2i xy = it.getPos();
                Colorf color = hsvToRGB(Vec3f((float)xy.x / (float)colorSize.x, 1.f, 1.f));
                it.r() = color.r * 255.f;
                it.g() = color.g * 255.f;
                it.b() = color.b * 255.f;
            }
        }
        
        Surface8u depthSurface = Surface8u(depthSize.x, depthSize.y, false);
        it = depthSurface.getIter();
        while ( it.line() ) {
            while ( it.pixel() ) {
                float depth = ((Vec2f)it.getPos()).distance((Vec2f)depthSize / 2.f) / ((Vec2f)depthSize).distance(Vec2f(0, 0)) * 255.f;
                it.r() = depth;
                it.g() = depth;
                it.b() = depth;
                
                XnPoint3D *point = depthMapRealWorld + (it.x() + it.y() * depthSize.x);
                point->X = it.x() + depthSize.x * -0.5;
                point->Y = it.y() + depthSize.y * -0.5;
                point->Z = (depth / 255.f) * 1000.f; // real kinect data seems to range from about 0 - 6000
            }
        }
                
        tex_Color = gl::Texture(colorSurface);
        tex_Depth = gl::Texture(depthSurface);
    }
}
