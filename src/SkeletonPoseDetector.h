/*****************************************************************************
*                                                                            *
*  Sinbad Sample Application                                                 *
*  Copyright (C) 2010 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  OpenNI is free software: you can redistribute it and/or modify            *
*  it under the terms of the GNU Lesser General Public License as published  *
*  by the Free Software Foundation, either version 3 of the License, or      *
*  (at your option) any later version.                                       *
*                                                                            *
*  OpenNI is distributed in the hope that it will be useful,                 *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
*  GNU Lesser General Public License for more details.                       *
*                                                                            *
*  You should have received a copy of the GNU Lesser General Public License  *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.            *
*                                                                            *
*****************************************************************************/

// This sample is based on the Character sample from the OgreSDK.

#pragma once

#include <math.h>
#include "VOpenNICommon.h"
#include <time.h>


enum PoseDetectionResult
{
	NOT_IN_POSE,
	IN_POSE_FOR_LITTLE_TIME,
	IN_POSE_FOR_LONG_TIME,
	NOT_ENOUGH_CONFIDENCE,
};


static double GetCurrentTimeInSeconds()
{
	struct timespec t1;

	clock_gettime(CLOCK_REALTIME, &t1);

	double d_time = (double)t1.tv_nsec;
	return d_time;
}

class PoseDetectorBase
{
protected:
	double m_beginTimeOfPose;
	double m_durationOfPoseForDetection;
	double m_detectionPercent;

public:

	double GetDetectionPercent() const
	{
		return m_detectionPercent;
	}
	PoseDetectorBase(double durationForDetection = 1)
	{
		m_durationOfPoseForDetection = durationForDetection;
		Reset();
	}
	virtual void SetDurationForDetection(double time)
	{
		m_durationOfPoseForDetection = time;
		Reset();
	}
	virtual PoseDetectionResult checkPose() = 0;
	virtual void Reset()
	{
		m_beginTimeOfPose = -1;
		m_detectionPercent = 0;
	}
	virtual PoseDetectionResult checkPoseDuration()
	{
		double curTime = GetCurrentTimeInSeconds();
		switch(checkPose())
		{
		case IN_POSE_FOR_LITTLE_TIME: //falling through
		case IN_POSE_FOR_LONG_TIME:
			if(m_beginTimeOfPose != -1)
			{
				if(m_durationOfPoseForDetection!=0)
				{
					m_detectionPercent = (curTime - m_beginTimeOfPose) / m_durationOfPoseForDetection;
				} else
				{
					m_detectionPercent = 1;
				}

				if( m_detectionPercent >= 1){
					m_detectionPercent = 1;
					return IN_POSE_FOR_LONG_TIME;
				} 
			} else
			{
				m_beginTimeOfPose = curTime;
			}
			return IN_POSE_FOR_LITTLE_TIME;
		case NOT_ENOUGH_CONFIDENCE:
			if(m_beginTimeOfPose != -1)
			{
				if((curTime - m_beginTimeOfPose) > m_durationOfPoseForDetection){
					//restart waiting
					Reset();
					return IN_POSE_FOR_LITTLE_TIME;
				}
				return IN_POSE_FOR_LITTLE_TIME;
			}
			break;
		case NOT_IN_POSE:
			Reset();
			break;
		}
		return NOT_IN_POSE;
	}
};
class EndPoseDetector: public PoseDetectorBase
{
public:
	XnSkeletonJointTransformation m_prevLeftHand;
	XnSkeletonJointTransformation m_prevRightHand;
	xn::UserGenerator m_userGenerator;
	XnUserID m_nUserId;

	EndPoseDetector(xn::UserGenerator ug, double duration):PoseDetectorBase(duration)
	{
		m_userGenerator = ug;
	}

	void Reset()
	{
		PoseDetectorBase::Reset();
		m_prevLeftHand.position.fConfidence = 0;
		m_prevRightHand.position.fConfidence = 0;
	}

	void SetUserId(XnUserID nUserId)
	{
		m_nUserId = nUserId;
	}

	PoseDetectionResult checkPose()
	{	
		XnSkeletonJointTransformation leftHand;
		XnSkeletonJointTransformation rightHand;
		xn::SkeletonCapability skeletonCap = m_userGenerator.GetSkeletonCap();
		
		if (!skeletonCap.IsTracking(m_nUserId))
		{
			return NOT_IN_POSE;
		}

		skeletonCap.GetSkeletonJoint(m_nUserId, XN_SKEL_LEFT_HAND, leftHand);
		skeletonCap.GetSkeletonJoint(m_nUserId, XN_SKEL_RIGHT_HAND, rightHand);


		bool bHaveLeftHand = leftHand.position.fConfidence  >= 0.5;
		bool bHaveRightHand = rightHand.position.fConfidence >= 0.5;
		if(!bHaveLeftHand && !bHaveRightHand )
		{
			return NOT_IN_POSE;
		}
		if(bHaveLeftHand) m_prevLeftHand  = leftHand;
		if(bHaveRightHand) m_prevRightHand = rightHand;

		//check for X (left hand is "righter" than right (more than 10 cm)
		float xDist = leftHand.position.position.X - rightHand.position.position.X ;

		if(xDist < 60 ) return NOT_IN_POSE;

		//check hands to be at same height
		float yDist = fabs(leftHand.position.position.Y - rightHand.position.position.Y);

		if(yDist > 300 ) return NOT_IN_POSE;


//		printf("in end pose!!!");
		return IN_POSE_FOR_LITTLE_TIME;
	}
};

class StartPoseDetector: public PoseDetectorBase
{
public:
	
	xn::UserGenerator* m_pUserGenerator;

	StartPoseDetector(double duration):PoseDetectorBase(duration)
	{
		_isInStartPose = false;
	}
	void Reset()
	{
		PoseDetectorBase::Reset();
		_isInStartPose = false;
	}

	PoseDetectionResult checkPose()
	{	
	
		if(IsInStartPose())
		{
			return IN_POSE_FOR_LITTLE_TIME;
		}
		return NOT_IN_POSE;
	}
	bool _isInStartPose;
	bool IsInStartPose() 
	{
		return _isInStartPose;
	}
	void SetStartPoseState(bool isInStartPose)
	{
		_isInStartPose = isInStartPose;
	}
};
