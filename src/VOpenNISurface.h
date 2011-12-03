#pragma once

//#include "VOpenNICommon.h"


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

			// malloc/free
			mData = (T*)malloc(mWidth*mHeight*mBPP);
			memset( mData, 0, mWidth*mHeight*mBPP );
			// new/delete
			//mData = new T[mWidth*mHeight*mBPP];
		}

		~OpenNISurfaceT() 
		{
			free( mData );
			//SAFE_DELETE_ARRAY( mData );
		}

		void remap( uint32_t width, uint32_t height )
		{
			if( mData ) 
			{
				mWidth = width;
				mHeight = height;
				
				// realloc/free
				T* tempBuffer = (T*)realloc( mData, width*height*mBPP);
				// Make sure the new alloc is valid
				if( tempBuffer )
				{
					mData = tempBuffer;
					memset( mData, 0, mWidth*mHeight*mBPP );
				}
				else // There was an error
				{
					free( mData );
				}
				// new/delete
				//SAFE_DELETE_ARRAY( mData );
				//mData = new T[width*height*mBPP];
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


	typedef OpenNISurfaceT<uint8_t> OpenNISurfaceColorRGB;
	typedef OpenNISurfaceT<uint16_t> OpenNISurfaceIR;
	typedef OpenNISurfaceT<uint16_t> OpenNISurfaceDepth;
	typedef OpenNISurfaceT<uint8_t> OpenNISurface8;
	typedef OpenNISurfaceT<uint16_t> OpenNISurface16;
}