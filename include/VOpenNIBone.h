#pragma once



namespace V
{
	enum SkeletonBoneType
	{
		SKEL_HEAD = 1, 
		SKEL_NECK = 2, 
		SKEL_TORSO = 3, 
		SKEL_WAIST = 4, 
		SKEL_LEFT_COLLAR = 5, 
		SKEL_LEFT_SHOULDER = 6, 
		SKEL_LEFT_ELBOW = 7, 
		SKEL_LEFT_WRIST = 8, 
		SKEL_LEFT_HAND = 9, 
		SKEL_LEFT_FINGERTIP = 10, 
		SKEL_RIGHT_COLLAR = 11, 
		SKEL_RIGHT_SHOULDER = 12, 
		SKEL_RIGHT_ELBOW = 13, 
		SKEL_RIGHT_WRIST = 14, 
		SKEL_RIGHT_HAND = 15, 
		SKEL_RIGHT_FINGERTIP = 16, 
		SKEL_LEFT_HIP = 17, 
		SKEL_LEFT_KNEE = 18, 
		SKEL_LEFT_ANKLE = 19, 
		SKEL_LEFT_FOOT = 20, 
		SKEL_RIGHT_HIP = 21, 
		SKEL_RIGHT_KNEE = 22, 
		SKEL_RIGHT_ANKLE = 23, 
		SKEL_RIGHT_FOOT = 24 
	};
	static const int BONE_COUNT = 24;



	// Defines a body joint in space
	struct OpenNIBone
	{
		// is this bone active?
		bool active;

		// Position of the joint
		float position[3];

		// Orientation of a specific joint. 
		// A joint orientation is described by its actual rotation and the confidence we have in that rotation 
		// The first column is the X orientation, where the value increases from left to right. 
		// The second column is the Y orientation, where the value increases from bottom to top. 
		// The third column is the Z orientation, where the value increases from near to far. 
		float orientation[9];

		// The confidence in the orientation 
		float positionConfidence;
		// The confidence in the orientation 
		float orientationConfidence;
	};
}