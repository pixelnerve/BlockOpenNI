#pragma once

#include "VOpenNICommon.h"


namespace V
{
	struct UserEvent
	{
		UserEvent()
		{
			mId = 0;
			mDevice = OpenNIDeviceRef();
			mUser = OpenNIUserRef();
		}

		uint32_t				mId;
		OpenNIDeviceRef			mDevice;
		OpenNIUserRef			mUser;
	};

	class UserListener
	{
	public:
		virtual void onNewUser( UserEvent event ) {};
		virtual void onLostUser( UserEvent event ) {};
		virtual void onCalibrationStart( UserEvent event ) {};
		virtual void onCalibrationComplete( UserEvent event ) {};
	};

}  // namespace