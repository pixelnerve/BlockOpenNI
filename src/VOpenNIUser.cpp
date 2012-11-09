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
	int g_UsedBoneIndexArray[15] = { 1, 2, 3, 6, 7, 9, 12, 13, 15, 17, 18, 20, 21, 22, 24 };



	inline XnVector3D sub( const XnVector3D& v0, const XnVector3D& v1 )
	{
		XnVector3D res;
		res.X = v0.X - v1.X;
		res.Y = v0.Y - v1.Y;
		res.Z = v0.Z - v1.Z;
		return res;
	}
	inline XnVector3D cross( const XnVector3D& v0, const XnVector3D& v1 )
	{
		XnVector3D res;
		res.X = v0.Y * v1.Z - v1.Y * v0.Z;
		res.Y = v0.Z * v1.X - v1.Z * v0.X;
		res.Z = v0.X * v1.Y - v1.X * v0.Y;
		return res;
	}
	inline XnVector3D normalize( const XnVector3D& v0 )
	{
		float len = sqrtf( v0.X*v0.X + v0.Y*v0.Y + v0.Z*v0.Z );
		if( len < 0.00001f ) return XnVector3D();
		XnVector3D res;
		float invS = 1.0f / len;
		res.X = v0.X * invS;
		res.Y = v0.Y * invS;
		res.Z = v0.Z * invS;
		return res;
	}


	OpenNIUser::OpenNIUser( int32_t id, OpenNIDevice* device ) 
		: mUserState(USER_NONE)
	{
		//_userGen = boost::shared_ptr<xn::UserGenerator>( new xn::UserGenerator );
		_device = device;
		mId = id;

		//mUserGen = NULL;
		mDepthGen = NULL;

		//_userPixels = NULL;
		//_backUserPixels = NULL;
		//_userDepthPixels = NULL;
		//_backUserDepthPixels = NULL;
		//_depthMapRealWorld = NULL;
		//_backDepthMapRealWorld = NULL;

		_enablePixels = true;

		init();
	}

	/*OpenNIUser::OpenNIUser( int32_t id, OpenNIDeviceRef device )
		: mUserState(USER_NONE)
	{
		//_userGen = boost::shared_ptr<xn::UserGenerator>( new xn::UserGenerator );
		_deviceRef = device;
		mId = id;

		_userPixels = NULL;
		_backUserPixels = NULL;
		_userDepthPixels = NULL;
		_backUserDepthPixels = NULL;
		_depthMapRealWorld = NULL;
		_backDepthMapRealWorld = NULL;

		_enablePixels = true;

		init();
	}*/

	OpenNIUser::~OpenNIUser()
	{
		std::stringstream ss;
		ss << "DESTRUCTOR USER ID: " << mId << std::endl;
		DEBUG_MESSAGE( ss.str().c_str() );

		//SAFE_DELETE_ARRAY( _userPixels );
		//SAFE_DELETE_ARRAY( _backUserPixels );
		//SAFE_DELETE_ARRAY( _userDepthPixels );
		//SAFE_DELETE_ARRAY( _backUserDepthPixels );
		//SAFE_DELETE_ARRAY( _depthMapRealWorld );
		//SAFE_DELETE_ARRAY( _backDepthMapRealWorld );

		if( !mBoneList.empty() )
		{
			for ( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); ++it )
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
		//std::stringstream ss;
		//ss << "INIT USER ID: " << mId << std::endl;
		//DEBUG_MESSAGE( ss.str().c_str() );

		mColor[0] = g_Colors[(mId-1)%nColors][0];
		mColor[1] = g_Colors[(mId-1)%nColors][1];
		mColor[2] = g_Colors[(mId-1)%nColors][2];

		// Allocate memory for every bone/joint
		for( int i=0; i<BONE_COUNT; i++ )
		{
			OpenNIBone* bone = new OpenNIBone();
			bone->id = g_BoneIndexArray[i];
			// Is active?
			bone->active = true;

			mBoneList.push_back( bone );
		}
		
		mAvgPosConfidence = 0.0f;


		//// TODO: Fix this. Remove hardcoded values
		//mWidth = 640;
		//mHeight = 480;
		//allocate( mWidth, mHeight );


		//mUserGen = _device->getUserGenerator();
		mDepthGen = _device->getDepthGenerator();
	}

	//void OpenNIUser::allocate( int width, int height )
	//{
	//	//SAFE_DELETE_ARRAY( _backUserPixels );
	//	//SAFE_DELETE_ARRAY( _userPixels );
	//	//_userPixels = new uint8_t[width*height*3];
	//	//_backUserPixels = new uint8_t[width*height*3];

	//	SAFE_DELETE_ARRAY( _userDepthPixels );
	//	SAFE_DELETE_ARRAY( _backUserDepthPixels );
	//	_userDepthPixels = new uint16_t[width*height];
	//	_backUserDepthPixels = new uint16_t[width*height];

	//	//SAFE_DELETE_ARRAY( _depthMapRealWorld );
	//	//SAFE_DELETE_ARRAY( _backDepthMapRealWorld );
	//	//_depthMapRealWorld = new XnPoint3D[width*height];
	//	//_backDepthMapRealWorld = new XnPoint3D[width*height];
	//}


	//
	// Load a calibration data file and apply to current user
	//
	void OpenNIUser::loadCalibrationDataToFile( const std::string& filename )
	{
#ifdef OPENNI_POSE_SAVE_CAPABILITY
		xn::SkeletonCapability skelCap = _device->getUserGenerator()->GetSkeletonCap();

		if( skelCap.IsCalibrated( mId ) )
			//if( _device->getUserGenerator() && _device->getUserGenerator()->GetSkeletonCap().IsCalibrated( mId ) )
		{
			// Save user's calibration to file
			XnStatus rc = skelCap.LoadCalibrationDataFromFile( mId, filename.c_str() );
			//XnStatus rc = _device->getUserGenerator()->GetSkeletonCap().LoadCalibrationDataFromFile( mId, filename.c_str() );
			CHECK_RC( rc, "OpenNIUser::loadCalibrationDataToFile()" );
			if( rc == XN_STATUS_OK )
			{
				// Make sure state 
				mUserState = USER_TRACKING;
				_device->getUserGenerator()->GetPoseDetectionCap().StopPoseDetection( mId );
				skelCap.StartTracking( mId );
			}
		}
#else
		DEBUG_MESSAGE( "OpenNIUser::loadCalibrationDataToFile()  Not supported on the version" );
#endif
	}

	//
	// Save current user's calibration data to a file
	//
	void OpenNIUser::saveCalibrationDataToFile( const std::string& filename )
	{
#ifdef OPENNI_POSE_SAVE_CAPABILITY
		xn::SkeletonCapability skelCap = _device->getUserGenerator()->GetSkeletonCap();

		if( skelCap.IsCalibrated( mId ) )
			//if( _device->getUserGenerator() && _device->getUserGenerator()->GetSkeletonCap().IsCalibrated( mId ) )
		{
			// Save user's calibration to file
			XnStatus rc = skelCap.SaveCalibrationDataToFile( mId, filename.c_str() );
			CHECK_RC( rc, "OpenNIUser::saveCalibrationDataToFile()" );
		}
#else
		DEBUG_MESSAGE( "OpenNIUser::saveCalibrationDataToFile()  Not supported on the version" );
#endif
	}

	void OpenNIUser::update()
	{
		//updatePixels();
		updateBody();
		// Update bounding box
		//getUserPosition();
	}


	void OpenNIUser::updatePixels()
	{
/****
		// Get device generators
		//DepthGenerator* depth = _device->getDepthGenerator();
		UserGenerator* user = _device->getUserGenerator();

		// Get metadata
		//xn::DepthMetaData depthMetaData;
		//depth->GetMetaData( depthMetaData );
		//xn::DepthMetaData* depthMetaData = _device->getDepthMetaData();
		xn::SceneMetaData* sceneMetaData = _device->getSceneMetaData();


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

			uint8_t* depthMap24 = _device->getDepthMap24();

			// Get labels
			const XnLabel* labels = sceneMetaData->Data();
			if( labels )
			{
				//
				// Generate a bitmap with the user pixels colored
				//
				int depthWidth = sceneMetaData->XRes();
				int depthHeight = sceneMetaData->YRes();
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
				xnOSMemSet( _backDepthMapRealWorld, 0, depthWidth*depthHeight*sizeof(XnPoint3D) );


				uint16_t* pDepth = _device->getDepthMap();
				uint8_t* pixels = _backUserPixels;
				uint16_t* depthPixels = _backUserDepthPixels;
				XnPoint3D* points = _backDepthMapRealWorld;

				mUserMinZDistance = 65535;
				mUserMaxZDistance = 0;


				int index = 0;
				for( int j=0; j<depthHeight; j++ )
				{
					for( int i=0; i<depthWidth; i++ )
					{
						// Only fill bitmap with current user's data
						////if( *labels != 0 && *labels == mId )
						////{
						////	// Pixel is not empty, deal with it.
						////	uint32_t nValue = *pDepth;

						////	mColor[0] = g_Colors[mId][0];
						////	mColor[1] = g_Colors[mId][1];
						////	mColor[2] = g_Colors[mId][2];

						////	if( nValue != 0 )
						////	{
						////		int nHistValue = _device->getDepthMap24()[nValue];
						////		pixels[index+0] = 0xff & static_cast<uint8_t>(nHistValue * mColor[0]);//g_Colors[nColorID][0]); 
						////		pixels[index+1] = 0xff & static_cast<uint8_t>(nHistValue * mColor[1]);//g_Colors[nColorID][1]);
						////		pixels[index+2] = 0xff & static_cast<uint8_t>(nHistValue * mColor[2]);//g_Colors[nColorID][2]);
						////	}
						////}



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
								uint32_t nHistValue = depthMap24[nValue];
								pixels[index+0] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][0]); 
								pixels[index+1] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][1]);
								pixels[index+2] = 0xff & static_cast<uint8_t>(nHistValue * g_Colors[nColorID][2]);
							}

							// Keep depth value
							*depthPixels = nValue;

							(*points).X = (float)i;
							(*points).Y = (float)j;
							(*points).Z = (float)nValue;

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
						else
						{
							(*points).X = 0;
							(*points).Y = 0;
							(*points).Z = -99999;
						}


						depthPixels++;
						points++;
						pDepth++;
						labels++;
						index += 3;
					}
				}

				memcpy( _userPixels, _backUserPixels, mWidth*mHeight*3 );
				memcpy( _userDepthPixels, _backUserDepthPixels, mWidth*mHeight*sizeof(uint16_t) );  // 16bit
				memcpy( _depthMapRealWorld, _backDepthMapRealWorld, mWidth*mHeight*sizeof(XnPoint3D) );
			}
		}

		//calcDepthImageRealWorld();
****/
	}



	void OpenNIUser::updateBody()
	{
		UserGenerator* user = _device->getUserGenerator();
		if( !user ) return;

		// If not tracking, bail out!
		if( user->GetSkeletonCap().IsTracking(mId) == false )
		{
			//_debugInfo = "User is not being tracked";
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
			//_debugInfo = "User is being tracked";

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
			//DepthGenerator* depth = _device->getDepthGenerator();

			XnSkeletonJointPosition jointPos;
			XnSkeletonJointOrientation jointOri;
			XnPoint3D projectivePos;
			xn::SkeletonCapability skelCap = user->GetSkeletonCap();

			
			mAvgPosConfidence = 0.0f;
			
			int index = 0;
			for( OpenNIBoneList::iterator it = mBoneList.begin(); it != mBoneList.end(); ++it )
			//for( int i=0; i<BONE_COUNT; i++ )
			{
/***				//
				// Update only *USED* positions
				//
				if( (index+1) == g_UsedBoneIndexArray[index2] )
				{
					skelCap.GetSkeletonJointPosition( mId, (XnSkeletonJoint)(g_UsedBoneIndexArray[index2]), jointPos );
					skelCap.GetSkeletonJointOrientation( mId, (XnSkeletonJoint)(g_BoneIndexArray[index2]), jointOri );

					//if( jointOri.fConfidence >= 0.3f || 
						//jointPos.fConfidence >= 0.3f )
					{
						OpenNIBone* bone = *it;
						//OpenNIBone* bone = mBoneList[i];

						//bone->id = g_BoneIndexArray[i];
						// Is active?
						bone->active = true;

						// Position (actual position in world coordinates)
						bone->position[0] = jointPos.position.X;
						bone->position[1] = jointPos.position.Y;
						bone->position[2] = jointPos.position.Z;
						//memcpy( bone->position, (float*)&jointPos.position.X, 3*sizeof(float) );

						// Compute Projective position coordinates
						mDepthGen->ConvertRealWorldToProjective( 1, &jointPos.position, &projectivePos );
						bone->positionProjective[0] = (projectivePos.X > 0) ? projectivePos.X : 0;
						bone->positionProjective[1] = (projectivePos.Y > 0) ? projectivePos.Y : 0;
						bone->positionProjective[2] = (projectivePos.Z > 0) ? projectivePos.Z : 0;

						// Orientation
						memcpy( bone->orientation, jointOri.orientation.elements, 9*sizeof(float) );

						// Confidence
						//bone->positionConfidence = jointPos.fConfidence;
						//bone->orientationConfidence = jointOri.fConfidence;
					}

					index2++;
				}
**/


				//
				// Update *ALL* positions
				//
				skelCap.GetSkeletonJointPosition( mId, (XnSkeletonJoint)(g_BoneIndexArray[index]), jointPos );
				skelCap.GetSkeletonJointOrientation( mId, (XnSkeletonJoint)(g_BoneIndexArray[index]), jointOri );

				OpenNIBone* bone = *it;

				//bone->id = g_BoneIndexArray[i];
				// Is active?
				bone->active = true;

				// Position (actual position in world coordinates)
				bone->position[0] = jointPos.position.X;
				bone->position[1] = jointPos.position.Y;
				bone->position[2] = jointPos.position.Z;
				//memcpy( bone->position, (float*)&jointPos.position.X, 3*sizeof(float) );

				// Compute Projective position coordinates
				mDepthGen->ConvertRealWorldToProjective( 1, &jointPos.position, &projectivePos );
				bone->positionProjective[0] = (projectivePos.X > 0) ? projectivePos.X : 0;
				bone->positionProjective[1] = (projectivePos.Y > 0) ? projectivePos.Y : 0;
				bone->positionProjective[2] = (projectivePos.Z > 0) ? projectivePos.Z : 0;

				// Orientation
				memcpy( bone->orientation, jointOri.orientation.elements, 9*sizeof(float) );

				// Confidence
				bone->positionConfidence = jointPos.fConfidence;
				bone->orientationConfidence = jointOri.fConfidence;
				
				mAvgPosConfidence += jointPos.fConfidence;

				index++;
			}
			
			// Compute average position confidence.
			mAvgPosConfidence /= (float)mBoneList.size();
			

			mUserState = USER_TRACKING;
		}
	}


	XnVector3D OpenNIUser::GetForwardVector()
	{
		if( mId ) 
		{
			UserGenerator* userGen = _device->getUserGenerator();

			// OpenNI's orientation does not work very well.
			/*
			XnSkeletonJointTransformation t;
			m_userGen->GetSkeletonCap().GetSkeletonJoint(userID, XN_SKEL_TORSO, t);	
			float* e = t.orientation.orientation.elements;
			return XV3(e[6], e[7], -e[8]);
			*/

			XnSkeletonJointPosition p0, p1, p2;
			userGen->GetSkeletonCap().GetSkeletonJointPosition( mId, XN_SKEL_RIGHT_SHOULDER, p0 );
			userGen->GetSkeletonCap().GetSkeletonJointPosition( mId, XN_SKEL_TORSO, p1 );
			userGen->GetSkeletonCap().GetSkeletonJointPosition( mId, XN_SKEL_LEFT_SHOULDER, p2 );
			XnVector3D v0(p0.position), v1(p1.position), v2(p2.position);

			XnVector3D res1, res2;
			res1 = sub( v1, v0 );
			res2 = sub( v2, v0 );
			XnVector3D res = cross( res1, res2 );
			res = normalize( res );
		}
		
		return XnVector3D();
	}

	XnVector3D OpenNIUser::GetUpVector()
	{
		if( mId ) 
		{
			UserGenerator* userGen = _device->getUserGenerator();

			XnSkeletonJointPosition p0, p1;
			userGen->GetSkeletonCap().GetSkeletonJointPosition( mId, XN_SKEL_TORSO, p0 );
			userGen->GetSkeletonCap().GetSkeletonJointPosition( mId, XN_SKEL_NECK, p1 );
			XnVector3D v0(p0.position), v1(p1.position);
			XnVector3D res1 = sub( v1, v0 );
			res1 = sub( v1, v0 );
			res1 = normalize( res1 );
			return res1;
		}
		
		return XnVector3D();
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
			
			
			// Low confidence, bail out!
//			if( mAvgPosConfidence < _device->getConfidenceThreshold() )
//				return;

			
			
			//
			// Old OpenGL rendering method. it's fine for now.
			//
			glBegin( GL_QUADS );
			int index = 1;
			for( std::vector<OpenNIBone*>::iterator it = mBoneList.begin(); it != mBoneList.end(); ++it, index++ )
			{
				OpenNIBone* bone = *it;


				// Bail out in case any bone is not confident
				if( bone->positionConfidence < 0.5f || bone->orientationConfidence < 0.5f )
					break;

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

			renderBone( SKEL_TORSO, SKEL_LEFT_SHOULDER, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_ELBOW, SKEL_LEFT_HAND, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_RIGHT_SHOULDER, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND, width, height, depth, true, renderDepth );

//			renderBone( SKEL_LEFT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );
//			renderBone( SKEL_RIGHT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_HIP, SKEL_LEFT_KNEE, width, height, depth, true, renderDepth );
			renderBone( SKEL_LEFT_KNEE, SKEL_LEFT_FOOT, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE, width, height, depth, true, renderDepth );
			renderBone( SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );
			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, width, height, depth, true, renderDepth );
//			renderBone( SKEL_LEFT_HIP, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );

			renderBone( SKEL_TORSO, SKEL_NECK, width, height, depth, true, renderDepth );

			// Restore texture
			//glEnable( GL_TEXTURE_2D );
		}
	}

	void OpenNIUser::renderJointsRealWorld( float pointSize, float zScale )
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
			
			// Low confidence, bail out!
			if( mAvgPosConfidence < _device->getConfidenceThreshold() )
				return;


			// Old OpenGL rendering method. it's fine for now.
			glBegin( GL_QUADS );
			int index = 1;
			for( OpenNIBoneList::iterator it = mBoneList.begin(); it != mBoneList.end(); ++it, index++ )
			{
				OpenNIBone* bone = *it;

				// Convert a point from world coordinates to screen coordinates
				XnPoint3D point;
				point.X = bone->position[0];
				point.Y = bone->position[1];
				point.Z = bone->position[2] * zScale;
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
			renderBone( SKEL_HEAD, SKEL_NECK, 0, 0, 1, false, false );
			
			renderBone( SKEL_TORSO, SKEL_LEFT_SHOULDER, 0, 0, 1, false, false );
			renderBone( SKEL_LEFT_SHOULDER, SKEL_LEFT_ELBOW, 0, 0, 1, false, false );
			renderBone( SKEL_LEFT_ELBOW, SKEL_LEFT_HAND, 0, 0, 1, false, false );
			
			renderBone( SKEL_TORSO, SKEL_RIGHT_SHOULDER, 0, 0, 1, false, false );
			renderBone( SKEL_RIGHT_SHOULDER, SKEL_RIGHT_ELBOW, 0, 0, 1, false, false );
			renderBone( SKEL_RIGHT_ELBOW, SKEL_RIGHT_HAND, 0, 0, 1, false, false );
			
//			renderBone( SKEL_LEFT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );
//			renderBone( SKEL_RIGHT_SHOULDER, SKEL_TORSO, width, height, depth, true, renderDepth );
			
			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, 0, 0, 1, false, false );
			renderBone( SKEL_LEFT_HIP, SKEL_LEFT_KNEE, 0, 0, 1, false, false );
			renderBone( SKEL_LEFT_KNEE, SKEL_LEFT_FOOT, 0, 0, 1, false, false );
			
			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, 0, 0, 1, false, false );
			renderBone( SKEL_RIGHT_HIP, SKEL_RIGHT_KNEE, 0, 0, 1, false, false );
			renderBone( SKEL_RIGHT_KNEE, SKEL_RIGHT_FOOT, 0, 0, 1, false, false );
			
			renderBone( SKEL_TORSO, SKEL_RIGHT_HIP, 0, 0, 1, false, false );
			renderBone( SKEL_TORSO, SKEL_LEFT_HIP, 0, 0, 1, false, false );
//            renderBone( SKEL_LEFT_HIP, SKEL_RIGHT_HIP, width, height, depth, true, renderDepth );
			
			renderBone( SKEL_TORSO, SKEL_NECK, 0, 0, 1, false, false );

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

	bool OpenNIUser::getUserPosition()
	{
		if( !_device->getDepthGenerator()->IsValid() )
			return false;

		if( _device->getDepthGenerator()->IsCapabilitySupported( XN_CAPABILITY_USER_POSITION ) )
		{
			UserPositionCapability  userPosCap = _device->getDepthGenerator()->GetUserPositionCap();
			userPosCap.GetUserPosition( mId, mBoundingBox );
			return true;
		}
		return false;
	}

	//uint16_t* OpenNIUser::getDepthPixels()
	//{
	//	_device->getLabelMap( mId, _userDepthPixels );
	//	return _userDepthPixels;
	//}
	//void OpenNIUser::calcDepthImageRealWorld( XnPoint3D* points )
	//{
	//	// convert all point into realworld coord
	//	_device->getDepthGenerator()->ConvertProjectiveToRealWorld( mWidth*mHeight, _depthMapRealWorld, points );
	//}


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
}