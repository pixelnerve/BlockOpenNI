#pragma once

#include "VOpenNICommon.h"
#include "VOpenNIDevice.h"


namespace V
{

	class OpenNIRecorder
	{
	public:
#ifdef WIN32
		typedef		std::shared_ptr<OpenNIRecorder>		Ref;
#else
		typedef		boost::shared_ptr<OpenNIRecorder>		Ref;
#endif
		struct Configuration
		{
			bool	mIsMirrored;
			bool	mHasRegister;
			bool	mHasFrameSync;

			bool	mIsUserOn;
			bool	mIsDepthOn;
			bool	mIsImageOn;
			bool	mIsIROn;
		};

	public:

		OpenNIRecorder( OpenNIDeviceRef device );
		~OpenNIRecorder();


		void addNodes( uint64_t nodeTypeFlags, const std::string& filename );
		void removeNode( uint64_t nodeTypeFlags );
		void start();
		void stop();
		void pause();
		void update();
		bool isRecording()		{ return mIsRecording;	}

	private:
		OpenNIDeviceRef			mDevice;

		xn::EnumerationErrors	mErrors;
		xn::Recorder			mRecorder;
		xn::MockDepthGenerator	mMockDepth;
		xn::MockImageGenerator	mMockImage;
		xn::MockIRGenerator		mMockIR;

		uint64_t				mNodeFlags;

		Configuration			configuration;

		bool					mIsRecording;
		bool					mIsPaused;
	};
}