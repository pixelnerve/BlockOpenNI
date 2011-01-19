#pragma once

#include "VOpenNICommon.h"


namespace V
{

	template<typename T>
	class OpenNISurfaceT
	{
	public:
		OpenNISurfaceT( ProductionNodeType type, uint32_t width, uint32_t height ) 
		{
			switch( type )
			{
			case NODE_TYPE_IMAGE:
			case NODE_TYPE_IR:
				mBPP = 3;
				break;
			case NODE_TYPE_DEPTH:
				mBPP = 1;
				break;
			default:
				mBPP = 3;
			}
			mType = type;
			mWidth = width;
			mHeight = height;
			mData = NULL;
			mData = new T[mWidth*mHeight*mBPP];
		}

		~OpenNISurfaceT() 
		{
			SAFE_DELETE_ARRAY( mData );
		}

		void remap( uint32_t width, uint32_t height )
		{
			if( mData ) 
			{
				mWidth = width;
				mHeight = height;
				SAFE_DELETE_ARRAY( mData );
				mData = new T[width*height*mBPP];
			}
		}
		void update( T* data )			{ memcpy( mData, data, mWidth*mHeight*mBPP );		}

		T* getData()					{ return mData;		}
		uint32_t getWidth()				{ return mWidth;	}
		uint32_t getHeight()			{ return mHeight;	}
		uint32_t getType()				{ return mType;		}

	protected:
		T*			mData;
		uint32_t	mWidth, mHeight;
		uint32_t	mBPP;
		uint32_t	mType;
	};


	//typedef OpenNISurfaceT<uint8_t> OpenNISurfaceColorLuminance;
	typedef OpenNISurfaceT<uint8_t> OpenNISurfaceColorRGB;
	typedef OpenNISurfaceT<uint16_t> OpenNISurfaceIR;
	typedef OpenNISurfaceT<uint16_t> OpenNISurfaceDepth;
	typedef OpenNISurfaceT<uint8_t> OpenNISurface8;
	typedef OpenNISurfaceT<uint16_t> OpenNISurface16;
}