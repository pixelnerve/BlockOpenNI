#include "VOpenNIRecorder.h"



namespace V
{
	using namespace xn;

	#define CHECK_RC_ERR(rc, what, errors)			\
	{												\
		if (rc == XN_STATUS_NO_NODE_PRESENT)		\
		{											\
			XnChar strError[1024];					\
			errors.ToString(strError, 1024);		\
			printf("%s\n", strError);				\
		}											\
		CHECK_RC(rc, what)							\
	}



	OpenNIRecorder::OpenNIRecorder( OpenNIDeviceRef device )
	{
		mIsRecording = false;
		mIsPaused = false;
		mNodeFlags = NODE_TYPE_NONE;
		mDevice = device;
	}


	OpenNIRecorder::~OpenNIRecorder()
	{
		stop();
	}


	void OpenNIRecorder::addNodes( uint64_t nodeTypeFlags, const std::string& filename )
	{
		XnStatus result;

		// Create recorder
		result = mRecorder.Create( *mDevice->getContext() );	//->CreateAnyProductionTree( XN_NODE_TYPE_RECORDER, NULL, mRecorder, &mErrors );
		CHECK_RC( result, "[Recorder]  Create recorder" );
		//CHECK_RC_ERR( result, "Create recorder", mErrors );

		result = mRecorder.SetDestination(XN_RECORD_MEDIUM_FILE, filename.c_str() );
		CHECK_RC( result, "[Recorder]  Set destination" );


		ImageGenerator* image = NULL;
		IRGenerator* ir = NULL;
		DepthGenerator* depth = NULL;
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			image = mDevice->getImageGenerator();
			mRecorder.AddNodeToRecording( *image, XN_CODEC_JPEG );
			configuration.mIsImageOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			ir = mDevice->getIRGenerator();
			mRecorder.AddNodeToRecording( *ir, XN_CODEC_JPEG );
			configuration.mIsIROn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			depth = mDevice->getDepthGenerator();
			mRecorder.AddNodeToRecording( *depth, XN_CODEC_16Z_EMB_TABLES );
			configuration.mIsDepthOn = true;
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
		}

		// Check for frame synchronization
		if( depth->IsCapabilitySupported( XN_CAPABILITY_FRAME_SYNC ) ) 
		{
			if( (mNodeFlags&NODE_TYPE_DEPTH) && (mNodeFlags&NODE_TYPE_IMAGE) )
			{
				if( depth->GetFrameSyncCap().CanFrameSyncWith( *image ) ) 
				{
					result = depth->GetFrameSyncCap().FrameSyncWith( *image );
					CHECK_RC( result, "Enable frame sync");
					configuration.mHasFrameSync = true;
				}
			}
			else if( (mNodeFlags&NODE_TYPE_DEPTH) && (mNodeFlags&NODE_TYPE_IR) )
			{
				if( depth->GetFrameSyncCap().CanFrameSyncWith( *ir ) ) 
				{
					result = depth->GetFrameSyncCap().FrameSyncWith( *ir );
					CHECK_RC( result, "Enable frame sync");
					configuration.mHasFrameSync = true;
				}
			}
		}
	}

	void OpenNIRecorder::start()
	{
		if( mRecorder.IsValid() )
		{
			// If everything is ok, set to record.
			mIsRecording = true;
			mIsPaused = false;

			mRecorder.Record();
		}
	}

	void OpenNIRecorder::removeNode( uint64_t nodeTypeFlags )
	{
		ImageGenerator* image = NULL;
		IRGenerator* ir = NULL;
		DepthGenerator* depth = NULL;
		if( nodeTypeFlags & NODE_TYPE_IMAGE )
		{
			image = mDevice->getImageGenerator();
			mRecorder.RemoveNodeFromRecording( *image );
		}
		if( nodeTypeFlags & NODE_TYPE_IR )
		{
			ir = mDevice->getIRGenerator();
			mRecorder.RemoveNodeFromRecording( *ir );
		}
		if( nodeTypeFlags & NODE_TYPE_DEPTH )
		{
			depth = mDevice->getDepthGenerator();
			mRecorder.RemoveNodeFromRecording( *depth );
		}
		if( nodeTypeFlags & NODE_TYPE_USER )
		{
		}
	}
	void OpenNIRecorder::stop()
	{
		if( mIsRecording )
		{
			mRecorder.Release();
			mIsPaused = false;
			mIsRecording = false;
		}
	}


	void OpenNIRecorder::update()
	{
		if( mIsRecording )
			mRecorder.Record();
	}


	void OpenNIRecorder::pause()
	{
		mIsPaused = true;
		mIsRecording = false;
	}


}