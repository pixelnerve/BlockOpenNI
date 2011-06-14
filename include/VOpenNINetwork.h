#pragma once

//#include "VOpenNICommon.h"
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock.h>
//#include <winsock2.h>
//#include <WS2tcpip.h>
#else
#error "Unknown platform"
#endif

namespace V
{

	// Based on http://ndruger.lolipop.jp/hatena/20110108/tcp.diff
	class OpenNINetwork
	{
		public:
			OpenNINetwork( const std::string& hostName, uint16_t port )
				: mHostName(hostName), mHostPort(port)
			{
			}

			~OpenNINetwork()
			{
				::closesocket( mSocketId );
				::WSACleanup();
			}

			void Init()
			{
				//int __stdcall moo = 0;

				WSADATA data;
				WSAStartup( MAKEWORD(2,0), &data );

				mSocketId = ::socket( AF_INET, SOCK_STREAM, 0 );
				
				struct sockaddr_in dest_addr;
				memset( &dest_addr, 0, sizeof(dest_addr) );
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_addr.s_addr = inet_addr( mHostName.c_str() );
				dest_addr.sin_port = htons( mHostPort );
				
				int result = connect( mSocketId, (struct sockaddr *)&dest_addr, sizeof(dest_addr) );
				if( result == SOCKET_ERROR )
				{
					fprintf( stderr, "Connection error!\n" );
					OutputDebugStringA( "Connection error!\n" );
					return;
				}
			}


			int Send( char* message )
			{
				int res = ::send( mSocketId, message, strlen(message), 0 );
				return (res);
			}


		private:
			std::string			mHostName;
			uint16_t			mHostPort;
			SOCKET				mSocketId;
	};
}