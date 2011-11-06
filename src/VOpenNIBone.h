#pragma once



namespace V
{
	// Defines a body joint in space
	struct OpenNIBone
	{
		OpenNIBone()
		{
			active = false;
			id = -1;
			position[0] = position[1] = position[2] = 0.0f;
			positionProjective[0] = positionProjective[1] = positionProjective[2] = 0.0f;

			for( int i=0; i<9; i++)
				orientation[i] = 0.0f;

			positionConfidence = 0.0f;
			orientationConfidence = 0.0f;
		}

		int id;

		// is this bone active?
		bool active;

		// Position of the joint
		float position[3];
		float positionProjective[3];

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