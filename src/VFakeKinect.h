#pragma once

#include "VKinect.h"

namespace V {
    class FakeKinect : public Kinect
    {
    public:
        FakeKinect(){};
        ~FakeKinect(){};
        
        using Kinect::setup;
        void setup(const ci::Vec2i & _depthSize, const ci::Vec2i & _colorSize, int nodeTypeFlags);
        void update(){};
        
        ci::ImageSourceRef getColorImage(){ return ci::ImageSourceRef(tex_Color); }
        ci::ImageSourceRef getDepthImage(){ return ci::ImageSourceRef(tex_Depth); }
        
        class FakeDevice
        {
        public:
            void setAlignWithDepthGenerator(){};
            bool isDepthDataNew(){ return true; };
            bool isColorDataNew(){ return true; };
        };
        
        XnPoint3D * getDepthMapRealWorld() { return depthMapRealWorld; };
        FakeDevice * getDevice() { return &device; };
        
    private:
        FakeDevice device;
        XnPoint3D *depthMapRealWorld;
    };
}