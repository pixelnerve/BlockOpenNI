//#include "mmgr.h"
#include "VOpenNIBone.h"
#include "VOpenNIDevice.h"
#include "VOpenNIUser.h"



namespace V
{
	using namespace xn;


	XnFloat g_Colors[][3] =
	{
		{0,1,1},
		{0,1,0},
		{1,1,0},
		{1,0,0},
		{1,.5,0},
		{.5,1,0},
		{0,.5,1},
		{.5,0,1},
		{1,1,.5},
		{1,1,1},
		{0,0,1}
	};
	XnUInt32 nColors = 10;

	int g_BoneIndexArray[BONE_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };



	OpenNIUser::OpenNIUser( int32_t id, OpenNIDevice* device )
	{
		//_userGen = std::shared_ptr<xn::UserGenerator>( new xn::UserGenerator );
		_device = device;
		mId = id;

		_userPixels = NULL;

		init();
	}


	OpenNIUser::~OpenNIUser()
	{
		_device = NULL;

		SAFE_DELETE_ARRAY( _userPixels );

		for ( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++ )
		{
			SAFE_DELETE( *it );
		}
		mBoneList.clear();
	}


	void OpenNIUser::init()
	{
		// Set default color
		mColor[0] = mColor[1] = mColor[2] = 1;

		// Allocate memory for every bone/joint
		for( int i=0; i<BONE_COUNT; i++ )
		{
			// Index array. same as the joint enumeration values
			//g_BoneIndexArray[i] = i+1;

			mBoneList.push_back( new OpenNIBone() );
		}
	}

	void OpenNIUser::update()
	{
		updatePixels();
		updateBody();
	}


	void OpenNIUser::updatePixels()
	{
		// Get device generators
		DepthGenerator* depth = _device->getDepthGenerator();
		UserGenerator* user = _device->getUserGenerator();

		// Get metadata
		//xn::DepthMetaData depthMetaData;
		//depth->GetMetaData( depthMetaData );
		xn::DepthMetaData* depthMetaData = _device->getDepthMetaData();
		xn::SceneMetaData* sceneMetaData = _device->getUserMetaData();


		// Get Center Of Mass
		XnPoint3D com;
		user->GetCoM( mId, com );
		// Convert to screen coordinates
		depth->ConvertRealWorldToProjective( 1, &com, &com );
		mCenter[0] = com.X;
		mCenter[1] = com.Y;
		mCenter[2] = com.Z;

		// Get user pixels
		user->GetUserPixels( mId, *sceneMetaData );
		//xn::SceneMetaData sceneMetaData;
		//user->GetUserPixels( mId, sceneMetaData );

		// Get labels
		const XnLabel* labels = sceneMetaData->Data();
		if( labels )
		{
			//
			// Generate a bitmap with the user pixels colored
			//
			uint16_t* pDepth = _device->getDepthMap();
			int depthWidth = depthMetaData->XRes();
			int depthHeight = depthMetaData->YRes();
			if( !_userPixels || (mWidth != depthWidth) || (mHeight != depthHeight) )
			{
				mWidth = depthWidth;
				mHeight = depthHeight;
				SAFE_DELETE_ARRAY( _userPixels );
				_userPixels = new uint8_t[depthWidth*depthHeight*3];
			}
			//memcpy( _userPixels, _device->getDepthMap24(), depthWidth*depthHeight*3 );
			xnOSMemSet( _userPixels, 0, depthWidth*depthHeight*3 );
			int index = 0;
			for( int j=0; j<depthHeight; j++ )
			{
				for( int x=0; x<depthWidth; x++ )
				{
					if( *labels != 0 )
					{
						int nValue = *pDepth;
						XnLabel label = *labels;
						XnUInt32 nColorID = label % nColors;
						if( label == 0 )
						{
							nColorID = nColors;
						}
						mColor[0] = g_Colors[nColorID][0];
						mColor[1] = g_Colors[nColorID][1];
						mColor[2] = g_Colors[nColorID][2];

						if( nValue != 0 )
						{
							int nHistValue = _device->getDepthMap24()[nValue];
							//_userPixels[0] = 0xff & static_cast<uint8_t>(nHistValue * Colors[nColorID][0]); 
							//_userPixels[1] = 0xff & static_cast<uint8_t>(nHistValue * Colors[nColorID][1]);
							//_userPixels[2] = 0xff & static_cast<uint8_t>(nHistValue * Colors[nColorID][2]);
							_userPixels[index+0] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][0]); 
							_userPixels[index+1] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][1]);
							_userPixels[index+2] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][2]);
						}
					}

					pDepth++;
					labels++;
					index += 3;
				}
			}
		}
	}


	void OpenNIUser::updateBody()
	{
		UserGenerator* user = _device->getUserGenerator();
		//assert( !user );

		// If not tracking, bail out!
		if( !user->GetSkeletonCap().IsTracking(mId) )
		{
			_debugInfo = "User is not being tracked";

			//DEBUG_MESSAGE( "User is not being tracked!\n" );
			return;
		}


		// Is tracking user ?
		if( user->GetSkeletonCap().IsTracking(mId) )
		{
			_debugInfo = "User is being tracked";

			 
			//XnSkeletonJoint activeJoints[BONE_COUNT];
			//XnUInt16 numOfJoints = BONE_COUNT;
			//user->GetSkeletonCap().EnumerateActiveJoints( activeJoints, numOfJoints );

			// Save joint's transformation to our list
			XnSkeletonJointPosition jointPos;
			XnSkeletonJointOrientation jointOri;
			int index = 0;
			for( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++, index++ )
			{
				user->GetSkeletonCap().GetSkeletonJointPosition( mId, (XnSkeletonJoint)g_BoneIndexArray[index], jointPos );
				user->GetSkeletonCap().GetSkeletonJointOrientation( mId, (XnSkeletonJoint)g_BoneIndexArray[index], jointOri );

				//if( jointOri.fConfidence >= 0.25f && jointPos.fConfidence >= 0.25f )
				{
					OpenNIBone* bone = *it;

					// Is active?
					bone->active = true;

					// Position (actual position in world coordinates)
					bone->position[0] = jointPos.position.X;
					bone->position[1] = jointPos.position.Y;
					bone->position[2] = jointPos.position.Z;
					// Confidence
					bone->positionConfidence = jointPos.fConfidence;

					// Orientation
					memcpy( bone->orientation, jointOri.orientation.elements, 9*sizeof(float) );
					// Confidence
					bone->orientationConfidence = jointOri.fConfidence;
				}
			}
		}

		/*if( user->GetSkeletonCap().IsCalibrating(mId) )
		{
			_debugInfo = "User is calibrating";
		}

		if( user->GetSkeletonCap().IsCalibrated(mId) )
		{
			_debugInfo = "User is calibrated";
		}*/
	}


	void OpenNIUser::renderJoints( float pointSize )
	{
		if( _device->getUserGenerator()->GetSkeletonCap().IsTracking(mId) )
		{
			DepthGenerator* depth = _device->getDepthGenerator();
			if( !depth ) return;

			// Old OpenGL rendering method. it works for what we want here.
			//glDisable( GL_TEXTURE_2D );
			glBegin( GL_QUADS );
			int index = 1;
			for( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++, index++ )
			{
				OpenNIBone* bone = *it;

				// Convert a point from world coordinates to screen coordinates
				XnPoint3D point;
				XnPoint3D realJointPos;
				realJointPos.X = bone->position[0];
				realJointPos.Y = bone->position[1];
				realJointPos.Z = bone->position[2];
				depth->ConvertRealWorldToProjective( 1, &realJointPos, &point );
				float sx = pointSize;
				float sy = pointSize;

				glColor4f( 1, 1, 1, 1 );
				glVertex3f( -sx+point.X, -sy+point.Y, 0 );//point.Z );
				glVertex3f(  sx+point.X, -sy+point.Y, 0 );//point.Z );
				glVertex3f(  sx+point.X,  sy+point.Y, 0 );//point.Z );
				glVertex3f( -sx+point.X,  sy+point.Y, 0 );//point.Z );
			}
			glEnd();


			//
			// Render body connecting lines
			//
			renderBone( SKEL_HEAD, SKEL_NECK );

			renderBone( SKEL_NECK, SKEL_LEFT_SHOULDER );
			renderBone( SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW );
			renderBone( SKEL_LEFT_ELBOW, SKEL_LEFT_HAND );

			renderBone( SKEL_NECK, SKEL_RIGHT_SHOULDER );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW );
			renderBone( SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND );

			renderBone( SKEL_LEFT_SHOULDER, SKEL_TORSO );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_TORSO );

			renderBone( SKEL_TORSO, SKEL_LEFT_HIP );
			renderBone( SKEL_LEFT_HIP, SKEL_LEFT_KNEE );
			renderBone( SKEL_LEFT_KNEE, SKEL_LEFT_FOOT );

			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP );
			renderBone( SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE );
			renderBone( SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT );

			renderBone( SKEL_LEFT_HIP, SKEL_RIGHT_HIP );

			renderBone( SKEL_WAIST, SKEL_TORSO );
			renderBone( SKEL_WAIST, SKEL_NECK );

			// Restore texture
			//glEnable( GL_TEXTURE_2D );
		}
	}
	void OpenNIUser::renderBone( int joint1, int joint2 )
	{
		DepthGenerator* depth = _device->getDepthGenerator();
		if( !depth ) return;

		OpenNIBone* bone1 = mBoneList[joint1-1];
		OpenNIBone* bone2 = mBoneList[joint2-1];

		// Convert a point from world coordinates to screen coordinates
		XnPoint3D point1, point2;
		XnPoint3D realJointPos1, realJointPos2;
		realJointPos1.X = bone1->position[0];
		realJointPos1.Y = bone1->position[1];
		realJointPos1.Z = bone1->position[2];
		realJointPos2.X = bone2->position[0];
		realJointPos2.Y = bone2->position[1];
		realJointPos2.Z = bone2->position[2];

		depth->ConvertRealWorldToProjective( 1, &realJointPos1, &point1 );
		depth->ConvertRealWorldToProjective( 1, &realJointPos2, &point2 );

		glLineWidth( 2 );
		glBegin( GL_LINES );
		glColor4f( mColor[0], mColor[1], mColor[2], 1 );
		glVertex3f( point1.X, point1.Y, 0 );	//point.Z );
		glVertex3f( point2.X, point2.Y, 0 );	//point.Z );
		glEnd();
	}


	OpenNIBone* OpenNIUser::getBone( int id )
	{
		assert( id < 1 || id >= BONE_COUNT );
		return mBoneList[id-1];
	}

	UserBoneList OpenNIUser::getBoneList()
	{ 
		return mBoneList; 
	}


	void OpenNIUser::setSkeletonSmoothing( float t )
	{
		//if( _device->getUserGenerator()->GetSkeletonCap().IsTracking( mId ) )
			_device->getUserGenerator()->GetSkeletonCap().SetSmoothing( t );
	}
}