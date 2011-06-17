#pragma once

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <WinSock.h>
#else
#error "Unknown platform"
#endif


#define NETWORK_USE_THREAD

namespace V
{

	// Based on http://ndruger.lolipop.jp/hatena/20110108/tcp.diff
	class OpenNINetwork
	{
		public:
			OpenNINetwork( const std::string& hostName, uint16_t port, bool isServer=true )
				: mHostName(hostName), mHostPort(port), mIsServer(isServer)
			{
				mSocketId = INVALID_SOCKET;
				mSocketType = SOCK_STREAM;
				mIsRunning = false;
			}


			~OpenNINetwork()
			{
			}


			void Init()
			{
				WSADATA data;
				WSAStartup( MAKEWORD(2,0), &data );

				mSocketId = ::socket( AF_INET, mSocketType, IPPROTO_TCP );
				if( mSocketId == INVALID_SOCKET )
				{
					OutputDebugStringA( "Invalid socket!\n" );
					return;
				}
				
				struct sockaddr_in dest_addr;
				memset( &dest_addr, 0, sizeof(dest_addr) );
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_addr.s_addr = inet_addr( mHostName.c_str() );
				dest_addr.sin_port = htons( mHostPort );
				
				if( ::bind(mSocketId, (struct sockaddr *)&dest_addr, sizeof(sockaddr_in)) == SOCKET_ERROR )
				{
					closesocket( mSocketId );
					OutputDebugStringA( "Failed on connection!\n" );
					return;
				}


				//if( mIsServer ) 
				{
					if( ::listen( mSocketId, 1) == SOCKET_ERROR )
					{
						int res = WSAGetLastError();
						OutputDebugStringA( "listen(): Error listening on socket %ld.\n" );
						return;
					}
					else
					{
						OutputDebugStringA("listen() is OK, I'm waiting for connections...\n");
					}
#ifdef NETWORK_USE_THREAD
					DEBUG_MESSAGE( "Starting thread on network\n" );
					assert( !_thread );
					_thread = boost::shared_ptr<boost::thread>( new boost::thread(&OpenNINetwork::run, this) );
					mIsRunning = true;
#endif
				}

				std::stringstream ss;
				ss << "SERVER: Listen to '" << mHostName << "' at port: " << mHostPort << std::endl;
				OutputDebugStringA( ss.str().c_str() );
			}


			void InitClient()
			{
				WSADATA data;
				WSAStartup( MAKEWORD(2,0), &data );

				mSocketId = ::socket( AF_INET, mSocketType, IPPROTO_TCP );
				if( mSocketId == INVALID_SOCKET )
				{
					OutputDebugStringA( "Invalid socket!\n" );
					return;
				}

				struct sockaddr_in dest_addr;
				memset( &dest_addr, 0, sizeof(dest_addr) );
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_addr.s_addr = inet_addr( mHostName.c_str() );
				dest_addr.sin_port = htons( mHostPort );

				if( ::connect(mSocketId, (struct sockaddr *)&dest_addr, sizeof(sockaddr_in)) == SOCKET_ERROR )
				{
					closesocket( mSocketId );
					OutputDebugStringA( "Failed on connection!\n" );
					return;
				}

				std::stringstream ss;
				ss << "CLIENT: Connect to '" << mHostName << "' at port: " << mHostPort << std::endl;
				OutputDebugStringA( ss.str().c_str() );
			}


			void Release()
			{
#ifdef NETWORK_USE_THREAD
				if( mIsServer && mIsRunning )
				{
					DEBUG_MESSAGE( "Stop running thread on network\n" );
					assert( _thread );
					mIsRunning = false;
					//_thread->join();
					_thread.reset();
				}
#endif
				::closesocket( mSocketId );
				::WSACleanup();
			}

			int Send( char* message )
			{
				int res = ::send( mSocketId, message, strlen(message), 0 );
				//int error = WSAGetLastError();
				//if( error != 0 ) checkStatus( error );
				//std::stringstream ss;
				//ss << "Message size: " << msgSize << " error: " << error << std::endl;
				//OutputDebugStringA( ss.str().c_str() );

				return res;
			}


			int Send( const float* message, int count )
			{
				char* msg = (char*)message;
				int res = ::send( mSocketId, msg, count*sizeof(float), 0 );
				int error = WSAGetLastError();
				//if( error != 0 ) checkStatus( error );
				//std::stringstream ss;
				//ss << "Message size: " << 6*4 << " error: " << error << std::endl;
				//OutputDebugStringA( ss.str().c_str() );

				return res;
			}


			int Receive()
			{
				static const int bufferSize = 1024;
				char message[bufferSize];
				memset( message, 0, bufferSize );
				int res = ::recv( mSocketId, message, bufferSize, 0 );
				int error = WSAGetLastError();
				if( error != 0 ) checkStatus( error );
				//if( WSAGetLastError() == 0 )
				{
					std::stringstream ss;
					ss << "Size: " << res << "   Message: " << message << " " << std::endl;
					OutputDebugStringA( ss.str().c_str() );
				}

				return res;
			}

			int ReceiveFloatBuffer( int count )
			{
				static const int bufferSize = 32;
				float message[bufferSize];
				memset( message, 0, bufferSize );
				int res = ::recv( mSocketId, (char*)message, count*sizeof(float), 0 );
				int error = WSAGetLastError();
				if( error != 0 ) checkStatus( error );
				//if( WSAGetLastError() == 0 )
				{
					std::stringstream ss;
					ss << "Size: " << res << "   Message: " << message[2] << " " << std::endl;
					OutputDebugStringA( ss.str().c_str() );
				}

				return res;
			}


#ifdef NETWORK_USE_THREAD
			void run()
			{
				SOCKET AcceptSocket;


				while( mIsRunning )
				{
					AcceptSocket = SOCKET_ERROR;

					while( AcceptSocket == SOCKET_ERROR && mIsRunning )
					{
						AcceptSocket = accept( mSocketId, NULL, NULL );

						boost::this_thread::sleep( boost::posix_time::millisec(30) ); 
					}

					// else, accept the connection...
					// When the client connection has been accepted, transfer control from the
					// temporary socket to the original socket and stop checking for new connections.
					OutputDebugStringA( "Server: Client Connected!\n" );
					mSocketId = AcceptSocket;
					break;				
				}
			}
#endif


			void checkStatus( int error )
			{
				std::stringstream ss;
				switch( error )
				{
				case WSANOTINITIALISED:
					ss << "WSANOTINITIALISED" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAENETDOWN:
					ss << "WSAENETDOWN" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAEACCES:
					ss << "WSAEACCES" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAEINTR:
					ss << "WSAEINTR" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAEINPROGRESS:
					ss << "WSAEINPROGRESS" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAEFAULT:
					ss << "WSAEFAULT" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAENETRESET:
					ss << "WSAENETRESET" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAENOBUFS:
					ss << "WSAENOBUFS" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				case WSAEMSGSIZE:
					ss << "WSAEMSGSIZE" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				default:
					ss << "Other error not listed!" << std::endl;
					OutputDebugStringA( ss.str().c_str() );
					break;
				}
			}

		private:
			bool				mIsServer;

			volatile bool		mIsRunning;
			boost::shared_ptr<boost::thread> _thread;

			std::string			mHostName;
			uint16_t			mHostPort;
			SOCKET				mSocketId;
			int					mSocketType;
	};
}