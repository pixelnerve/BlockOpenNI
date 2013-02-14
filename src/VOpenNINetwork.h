#pragma once

#ifdef _WIN32

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <WinSock.h>
#else
#error "Unknown platform"
#endif
#include "VOpenNICommon.h"

#define NETWORK_USE_THREAD 1


namespace V
{

	// Based on http://ndruger.lolipop.jp/hatena/20110108/tcp.diff
	class OpenNINetwork
	{
		public:
			OpenNINetwork( const std::string& hostName, uint16_t port, bool isServer=true );
			~OpenNINetwork();
			void Init();
			void InitClient();
			void Release();
			int Send( char* message );

			template<class T>
			int Send( const T* message, int count );

			int Receive();
			int ReceiveFloatBuffer( int count );

			template<class T>
			int ReceiveBuffer( T* dest, int count );

#ifdef NETWORK_USE_THREAD
			void run();
#endif

		private:
			void CheckStatus( int error );

		private:
			bool				mIsServer;

			volatile bool		mIsRunning;
			boost::shared_ptr<std::thread> _thread;

			std::string			mHostName;
			uint16_t			mHostPort;
			SOCKET				mSocketId;
			int					mSocketType;

			uint16_t*			mUIntMessage;
	};




	template<class T>
	int OpenNINetwork::Send( const T* message, int count )
	{
		int error = 0;
		int sent = 0;
		int sentTotal = 0;
		int sendLeft = count*sizeof(T);

		char* msg = (char*)message;

		while( sendLeft > 0 )
		{
			sent = ::send( mSocketId, msg, sendLeft, 0 );
			error = WSAGetLastError();
			if( error != 0 ) 
			{
				CheckStatus( error );
				break;
			}
			msg += sent;
			sendLeft -= sent;
			sentTotal += sent;
		}
		return sentTotal;
	}


	template<class T>
	int OpenNINetwork::ReceiveBuffer( T* dest, int count )
	{
		if( count > 640*480 ) return -1;

		memset( mUIntMessage, 0, count*sizeof(T) );
		char* msg = (char*)mUIntMessage;

		int error = 0;
		int receivedTotal = 0;
		int received = 0;
		int receivedLeft = count*sizeof(T);

		while( receivedLeft > 0 )
		{
			received = ::recv( mSocketId, msg, receivedLeft, 0 );
			error = WSAGetLastError();
			if( error != 0 )
			{
				CheckStatus( error );
				break;
			}
			msg += received;
			receivedLeft -= received;
			receivedTotal += received;
		}
		if( receivedTotal > 0 ) memcpy( dest, mUIntMessage, receivedTotal );
		return receivedTotal;
	}
}


#endif