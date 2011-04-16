//#include "mmgr.h"
#include "VOpenNIBone.h"
#include "VOpenNIDevice.h"
#include "VOpenNIUser.h"

//#include "Vitamin/base/LogManager.h"



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
		: mUserState(USER_NONE)
	{
		//_userGen = boost::shared_ptr<xn::UserGenerator>( new xn::UserGenerator );
		_device = device;
		mId = id;

		_userPixels = NULL;
		_backUserPixels = NULL;
		_userDepthPixels = NULL;
		_backUserDepthPixels = NULL;

		_enablePixels = true;

		init();
	}

	OpenNIUser::OpenNIUser( int32_t id, OpenNIDeviceRef device )
		: mUserState(USER_NONE)
	{
		//_userGen = boost::shared_ptr<xn::UserGenerator>( new xn::UserGenerator );
		_deviceRef = device;
		mId = id;

		_userPixels = NULL;
		_backUserPixels = NULL;
		_userDepthPixels = NULL;
		_backUserDepthPixels = NULL;

		_enablePixels = true;

		init();
	}

	OpenNIUser::~OpenNIUser()
	{
		std::stringstream ss;
		ss << "DESTRUCTOR USER ID: " << mId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		SAFE_DELETE_ARRAY( _userPixels );
		SAFE_DELETE_ARRAY( _backUserPixels );
		SAFE_DELETE_ARRAY( _userDepthPixels );
		SAFE_DELETE_ARRAY( _backUserDepthPixels );

		if( !mBoneList.empty() )
		{
			for ( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++ )
			{
				SAFE_DELETE( *it );
			}
		}
		mBoneList.clear();
		mId = -1;
	}


	void OpenNIUser::init()
	{
		// Set default color
		std::stringstream ss;
		ss << "INIT USER ID: " << mId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );
		mColor[0] = g_Colors[(mId-1)%nColors][0];
		mColor[1] = g_Colors[(mId-1)%nColors][1];
		mColor[2] = g_Colors[(mId-1)%nColors][2];

		// Allocate memory for every bone/joint
		for( int i=0; i<BONE_COUNT; i++ )
		{
			mBoneList.push_back( new OpenNIBone() );
		}


		mWidth = 640;
		mHeight = 480;
		allocate( mWidth, mHeight );


		mSkeletonSmoothing = 0.2f;
		if( _device->getUserGenerator()->IsCapabilitySupported(XN_CAPABILITY_SKELETON) )
		{
			_device->getUserGenerator()->GetSkeletonCap().SetSmoothing( mSkeletonSmoothing );
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


		//// Get Center Of Mass
		//XnPoint3D com;
		//user->GetCoM( mId, com );
		//// Convert to screen coordinates
		//depth->ConvertRealWorldToProjective( 1, &com, &com );
		//mCenter[0] = com.X;
		//mCenter[1] = com.Y;
		//mCenter[2] = com.Z;


		if( _enablePixels )
		{
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
				int depthWidth = depthMetaData->XRes();
				int depthHeight = depthMetaData->YRes();
				if( !_userPixels || (mWidth != depthWidth) || (mHeight != depthHeight) )
				{
					mWidth = depthWidth;
					mHeight = depthHeight;
					allocate( depthWidth, depthHeight );
				}

				// Clear memory
				//xnOSMemSet( _userDepthPixels, 0, depthWidth*depthHeight*sizeof(uint16_t) );	// 16bit
				xnOSMemSet( _backUserPixels, 0, depthWidth*depthHeight*3 );
				xnOSMemSet( _backUserDepthPixels, 0, depthWidth*depthHeight*sizeof(uint16_t) );	// 16bit

				uint16_t* pDepth = _device->getDepthMap();
				uint8_t* pixels = _backUserPixels;
				uint16_t* depthPixels = _backUserDepthPixels;


				mUserMinZDistance = 65535;
				mUserMaxZDistance = 0;


				int index = 0;
				for( int j=0; j<depthHeight; j++ )
				{
					for( int i=0; i<depthWidth; i++ )
					{
						// Only fill bitmap with current user's data
						/*if( *labels != 0 && *labels == mId )
						{
							// Pixel is not empty, deal with it.
							uint32_t nValue = *pDepth;

							mColor[0] = g_Colors[mId][0];
							mColor[1] = g_Colors[mId][1];
							mColor[2] = g_Colors[mId][2];

							if( nValue != 0 )
							{
								int nHistValue = _device->getDepthMap24()[nValue];
								pixels[index+0] = 0xff & static_cast<uint8_t>(nHistValue * mColor[0]);//g_Colors[nColorID][0]); 
								pixels[index+1] = 0xff & static_cast<uint8_t>(nHistValue * mColor[1]);//g_Colors[nColorID][1]);
								pixels[index+2] = 0xff & static_cast<uint8_t>(nHistValue * mColor[2]);//g_Colors[nColorID][2]);
							}
						}****/


						// Check out for every user visible and fill bitmap with correct coloring
						// Only save pixels from current user, no other.
						if( *labels != 0 )	//&& *labels == mId )
						{
							// Pixel is not empty, deal with it.
							uint32_t nValue = *pDepth;
							XnLabel label = *labels;
							XnUInt32 nColorID = label % nColors;
							if( label == 0 )
							{
								nColorID = nColors;
							}
							mColor[0] = g_Colors[mId][0];
							mColor[1] = g_Colors[mId][1];
							mColor[2] = g_Colors[mId][2];
							if( nValue != 0 )
							{
								uint32_t nHistValue = _device->getDepthMap24()[nValue];
								pixels[index+0] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][0]); 
								pixels[index+1] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][1]);
								pixels[index+2] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][2]);
							}

							// Keep depth value
							*depthPixels = nValue;

							// Get max and min z values for current user
							//mUserMinZDistance = (nValue < mUserMinZDistance) ? nValue : mUserMinZDistance;
							//mUserMaxZDistance = (nValue > mUserMaxZDistance) ? nValue : mUserMaxZDistance;
							if( nValue < mUserMinZDistance ) 
							{
								mUserMinZDistance = nValue;
								mUserMinPixelIdx = i + j * depthWidth;
							}
							if( nValue > mUserMaxZDistance )
							{
								mUserMaxZDistance = nValue;
								mUserMaxPixelIdx = i + j * depthWidth;
							}
						}


						depthPixels++;

						pDepth++;
						labels++;
						index += 3;
					}
				}

				memcpy( _userPixels, _backUserPixels, mWidth*mHeight*3 );
				memcpy( _userDepthPixels, _backUserDepthPixels, mWidth*mHeight*sizeof(uint16_t) );  // 16bit
			}
		}
	}


	void OpenNIUser::updateBody()
	{
		UserGenerator* user = _device->getUserGenerator();
		if( !user ) return;

		// If not tracking, bail out!
		if( !user->GetSkeletonCap().IsTracking(mId) )
		{
			_debugInfo = "User is not being tracked";
			mUserState = USER_NONE;
			//DEBUG_MESSAGE( "User is not being tracked!\n" );
			return;
		}
		//if( user->GetSkeletonCap().IsCalibrating(mId) )
		//{
		//	_debugInfo = "User is calibrating";
		//	mUserState = USER_CALIBRATING;
		//}
		//if( user->GetSkeletonCap().IsCalibrated(mId) )
		//{
		//	_debugInfo = "User is calibrated";
		//	mUserState = USER_CALIBRATED;
		//}
		// If tracking is on...
		if( user->GetSkeletonCap().IsTracking(mId) )
		{
			_debugInfo = "User is being tracked";

			 
			//XnSkeletonJoint activeJoints[BONE_COUNT];
			//XnUInt16 numOfJoints = BONE_COUNT;
			//user->GetSkeletonCap().EnumerateActiveJoints( activeJoints, numOfJoints );

			//if( mBoneList.empty() )
			//{
			//	for( int i=0; i<BONE_COUNT; i++ )
			//	{
			//		mBoneList.push_back( new OpenNIBone() );
			//	}
			//}

			// Save joint's transformation to our list
			XnSkeletonJointPosition jointPos;
			XnSkeletonJointOrientation jointOri;
			int index = 0;
			//for( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++, index++ )
			for( int i=0; i<V::BONE_COUNT; i++ )
			{
				user->GetSkeletonCap().GetSkeletonJointPosition( mId, static_cast<XnSkeletonJoint>(g_BoneIndexArray[index]), jointPos );
				user->GetSkeletonCap().GetSkeletonJointOrientation( mId, static_cast<XnSkeletonJoint>(g_BoneIndexArray[index]), jointOri );

				if( jointOri.fConfidence >= 0.25f || jointPos.fConfidence >= 0.25f )
				{
					//if( jointPos.position.X == 0.0f ||
					//	jointPos.position.Y == 0.0f ||
					//	jointPos.position.Z == 0.0f )
					//{
					//	mUserState = USER_INVALID;
					//	Vitamin::LogManager::getSingleton().logMessage( "OpenNIUser. USER INVALID" );
					//	continue;
					//}

					//OpenNIBone* bone = *it;
					OpenNIBone* bone = mBoneList[i];

					bone->id = g_BoneIndexArray[i];

					// Is active?
					bone->active = true;

					// Position (actual position in world coordinates)
					bone->position[0] = jointPos.position.X;
					bone->position[1] = jointPos.position.Y;
					bone->position[2] = jointPos.position.Z;
					//memcpy( bone->position, (float*)&jointPos.position.X, 3*sizeof(float) );

					// Compute Projective position coordinates
					XnPoint3D projectivePos;
					DepthGenerator* depth = _device->getDepthGenerator();
					depth->ConvertRealWorldToProjective( 1, &jointPos.position, &projectivePos );
					bone->positionProjective[0] = (projectivePos.X > 0) ? projectivePos.X : 0;
					bone->positionProjective[1] = (projectivePos.Y > 0) ? projectivePos.Y : 0;
					bone->positionProjective[2] = (projectivePos.Z > 0) ? projectivePos.Z : 0;

					// Orientation
					memcpy( bone->orientation, jointOri.orientation.elements, 9*sizeof(float) );

					// Confidence
					bone->positionConfidence = jointPos.fConfidence;
					// Confidence
					bone->orientationConfidence = jointOri.fConfidence;

					//// Log first time this runs
					//if( mUserState != USER_TRACKING )
					//{
					//	std::stringstream ss;
					//	ss << bone->id << " - " << bone->position[0] << ", " << bone->position[1] << ", " << bone->position[2] << ", " << std::endl;
					//	Vitamin::LogManager::getSingleton().logMessage( ss.str() );
					//}
				}

				index++;
			}

			mUserState = USER_TRACKING;
		}
	}


	float* OpenNIUser::getCenterOfMass( bool doProjectiveCoords/*=false*/ )
	{
		//// Get Center Of Mass
		XnPoint3D com;
		_device->getUserGenerator()->GetCoM( mId, com );
		// Convert to screen coordinates
		if( doProjectiveCoords ) _device->getDepthGenerator()->ConvertRealWorldToProjective( 1, &com, &com );
		mCenter[0] = com.X;
		mCenter[1] = com.Y;
		mCenter[2] = com.Z;
		return mCenter; 
	}


	void OpenNIUser::renderJoints( float width, float height, float depth, float pointSize, bool renderDepth )
	{
		if( mUserState == USER_TRACKING )
		{
			DepthGenerator* depthGen = _device->getDepthGenerator();
			if( !depthGen )
			{
				std::stringstream ss;
				ss << "(renderJoints)  Failed to get depth generator on user: '" << mId << "'" << std::endl;
				DEBUG_MESSAGE( ss.str().c_str() );
				return;
			}

			// Old OpenGL rendering method. it's fine for now.
			glBegin( GL_QUADS );
			int index = 1;
			for( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); it++, index++ )
			{
				OpenNIBone* bone = *it;

				XnPoint3D point;
				point.X = bone->positionProjective[0];
				point.Y = bone->positionProjective[1];
				point.Z = bone->positionProjective[2];
				// Convert a point from world coordinates to screen coordinates
				//XnPoint3D point;
				//XnPoint3D realJointPos;
				//realJointPos.X = bone->position[0];
				//realJointPos.Y = bone->position[1];
				//realJointPos.Z = bone->position[2];
				//depth->ConvertRealWorldToProjective( 1, &realJointPos, &point );
				point.X *= 0.0015625f;			// div by 640
				point.Y *= 0.00208333333333333333333333333333f;			// div by 480
				float sx = pointSize;
				float sy = pointSize;
				if( bone->id == SKEL_TORSO )
				{
					sx *= 2;
					sy *= 2;
				}

				if( bone->id == SKEL_TORSO )
					glColor4f( 1, 0, 0, 1 );
				else
					glColor4f( 1, 1, 1, 1 );
				if( renderDepth )
				{
					glVertex3f( -sx+(point.X*width), -sy+(point.Y*height), point.Z*depth );
					glVertex3f(  sx+(point.X*width), -sy+(point.Y*height), point.Z*depth );
					glVertex3f(  sx+(point.X*width),  sy+(point.Y*height), point.Z*depth );
					glVertex3f( -sx+(point.X*width),  sy+(point.Y*height), point.Z*depth );

				}
				else
				{
					glVertex3f( -sx+(point.X*width), -sy+(point.Y*height), 0 );
					glVertex3f(  sx+(point.X*width), -sy+(point.Y*height), 0 );
					glVertex3f(  sx+(point.X*width),  sy+(point.Y*height), 0 );
					glVertex3f( -sx+(point.X*width),  sy+(point.Y*height), 0 );
				}
			}
			glEnd();


			//
			// Render body connecting lines
			//
			renderBone( SKEL_HEAD, SKEL_NECK, width, height, depth, true, renderDepth );

			renderBone( SKEL_NECK, SKEL_LEFT_SHOULDER, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_ELBOW, SKEL_LEFT_HAND, width, height, depth, true, renderDepth );

			renderBone( SKEL_NECK, SKEL_RIGHT_SHOULDER, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND, width, height, depth, true, renderDepth );

			renderBone( SKEL_LEFT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_HIP, SKEL_LEFT_KNEE, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_KNEE, SKEL_LEFT_FOOT, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT, width, height, depth, true, renderDepth );

			renderBone( SKEL_LEFT_HIP, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );

			renderBone( SKEL_WAIST, SKEL_TORSO, width, height, depth, true, renderDepth );
			renderBone( SKEL_WAIST, SKEL_NECK, width, height, depth, true, renderDepth );

			// Restore texture
			//glEnable( GL_TEXTURE_2D );
		}
	}

	void OpenNIUser::renderJointsRealWorld( float pointSize )
	{
		if( mUserState == USER_TRACKING )
		//if( !mBoneList.empty() && _device->getUserGenerator()->GetSkeletonCap().IsTracking(mId) )
		{
			DepthGenerator* depth = _device->getDepthGenerator();
			if( !depth ) 
			{
				std::stringstream ss;
				ss << "(renderJointsRealWorld)  Failed to get depth generator on user: '" << mId << "'" << std::endl;
				DEBUG_MESSAGE( ss.str().c_str() );
				return;
			}

			// Old OpenGL rendering method. it's fine for now.
			glBegin( GL_QUADS );
			int index = 1;
			for( OpenNIBoneList::iterator it = mBoneList.begin(); it != mBoneList.end(); it++, index++ )
			{
				OpenNIBone* bone = *it;

				// Convert a point from world coordinates to screen coordinates
				XnPoint3D point;
				point.X = bone->position[0];
				point.Y = bone->position[1];
				point.Z = bone->position[2];
				float sx = pointSize;
				float sy = pointSize;

				//std::stringstream ss;
				//ss << "BONE ID: " << bone->id << "  --   " << point.X << ", " << point.Y << ", " << point.Z << std::endl;
				//DEBUG_MESSAGE( ss.str().c_str() );

				glColor4f( 1, 1, 1, 1 );
				glVertex3f( -sx+point.X, -sy+point.Y, point.Z );
				glVertex3f(  sx+point.X, -sy+point.Y, point.Z );
				glVertex3f(  sx+point.X,  sy+point.Y, point.Z );
				glVertex3f( -sx+point.X,  sy+point.Y, point.Z );
			}
			glEnd();


			//
			// Render body connecting lines
			//
			renderBone( SKEL_HEAD, SKEL_NECK, false );

			renderBone( SKEL_NECK, SKEL_LEFT_SHOULDER, false );
			renderBone( SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW, false );
			renderBone( SKEL_LEFT_ELBOW, SKEL_LEFT_HAND, false );

			renderBone( SKEL_NECK, SKEL_RIGHT_SHOULDER, false );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW, false );
			renderBone( SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND, false );

			renderBone( SKEL_LEFT_SHOULDER, SKEL_TORSO, false );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_TORSO, false );

			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, false );
			renderBone( SKEL_LEFT_HIP, SKEL_LEFT_KNEE, false );
			renderBone( SKEL_LEFT_KNEE, SKEL_LEFT_FOOT, false );

			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, false );
			renderBone( SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE, false );
			renderBone( SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT, false );

			renderBone( SKEL_LEFT_HIP, SKEL_RIGHT_HIP, false );

			renderBone( SKEL_WAIST, SKEL_TORSO, false );
			renderBone( SKEL_WAIST, SKEL_NECK, false );

			// Restore texture
			//glEnable( GL_TEXTURE_2D );
		}
	}


	void OpenNIUser::renderBone( int joint1, int joint2, float width/*=640*/, float height/*=480*/, float depth/*=1*/, bool doProjective /*= true*/, bool renderDepthInProjective/*=false */ )
	{
		if( mUserState != USER_TRACKING ) return;

		if( doProjective )
		{
			DepthGenerator* depthGen = _device->getDepthGenerator();
			if( !depthGen ) 
			{
				std::stringstream ss;
				ss << "(renderBone)  Failed to get depth generator on user: '" << mId << "'" << std::endl;
				DEBUG_MESSAGE( ss.str().c_str() );
				return;
			}
			if( mBoneList.size() == 0 ) 
			{
				std::stringstream ss;
				ss << "(renderBone)  Bone count is zero on user: '" << mId << "'" << std::endl;
				DEBUG_MESSAGE( ss.str().c_str() );
				return;
			}

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

			depthGen->ConvertRealWorldToProjective( 1, &realJointPos1, &point1 );
			depthGen->ConvertRealWorldToProjective( 1, &realJointPos2, &point2 );

			point1.X *= 0.0015625f;			// div by 640
			point1.Y *= 0.00208333333333333333333333333333f;			// div by 480
			point2.X *= 0.0015625f;			// div by 640
			point2.Y *= 0.00208333333333333333333333333333f;			// div by 480
			point1.Z *= depth;
			point2.Z *= depth;

			glBegin( GL_LINES );
			glColor4f( mColor[0], mColor[1], mColor[2], 1.0f );
			glVertex3f( point1.X*width, point1.Y*height, (renderDepthInProjective)?point1.Z:0 );
			glVertex3f( point2.X*width, point2.Y*height, (renderDepthInProjective)?point2.Z:0 );
			glEnd();
		}
		else
		{
			OpenNIBone* bone1 = mBoneList[joint1-1];
			OpenNIBone* bone2 = mBoneList[joint2-1];

			// Convert a point from world coordinates to screen coordinates
			XnPoint3D point1, point2;
			point1.X = bone1->position[0];
			point1.Y = bone1->position[1];
			point1.Z = bone1->position[2];
			point2.X = bone2->position[0];
			point2.Y = bone2->position[1];
			point2.Z = bone2->position[2];

			glBegin( GL_LINES );
			glColor4f( mColor[0], mColor[1], mColor[2], 1.0f );
			glVertex3f( point1.X, point1.Y, point1.Z );
			glVertex3f( point2.X, point2.Y, point2.Z );
			glEnd();
		}
	}


	OpenNIBone* OpenNIUser::getBone( int id )
	{
		//if( mUserState != USER_TRACKING || mBoneList.empty() ) return NULL;
		return mBoneList[id-1];
	}

	OpenNIBoneList OpenNIUser::getBoneList()
	{ 
		return mBoneList; 
	}


	void OpenNIUser::setSkeletonSmoothing( float value )
	{
		mSkeletonSmoothing = value;
		if( _device->getUserGenerator() && _device->getUserGenerator()->GetSkeletonCap().IsTracking( mId ) )
		{
			_device->getUserGenerator()->GetSkeletonCap().SetSmoothing( value );
		}
	}



	void OpenNIUser::allocate( int width, int height )
	{
		if( _backUserPixels ) SAFE_DELETE_ARRAY( _backUserPixels );
		if( _userPixels ) SAFE_DELETE_ARRAY( _userPixels );
		_userPixels = new uint8_t[width*height*3];
		_backUserPixels = new uint8_t[width*height*3];

		if( _userDepthPixels ) SAFE_DELETE_ARRAY( _userDepthPixels );
		if( _backUserDepthPixels ) SAFE_DELETE_ARRAY( _backUserDepthPixels );
		_userDepthPixels = new uint16_t[width*height];
		_backUserDepthPixels = new uint16_t[width*height];
	}
}