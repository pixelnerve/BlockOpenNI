#pragma once

#include "VOpenNICommon.h"
#include "VOpenNIDevice.h"


namespace V
{

	class OpenNIRecorder
	{
	public:
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


		void start( uint64_t nodeTypeFlags, std::string& filename );
		void start( Configuration& config );
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

		bool					mIsRecording;
		bool					mIsPaused;
	};
}