#include "VOpenNINetwork.h"

#ifdef _WIN32
namespace V
{
	OpenNINetwork::OpenNINetwork( const std::string& hostName, uint16_t port, bool isServer/*=true*/ )
		: mHostName(hostName), mHostPort(port), mIsServer(isServer)
	{
		mSocketId = INVALID_SOCKET;
		mSocketType = SOCK_STREAM;
		mIsRunning = false;

		mUIntMessage = NULL;
	}


	OpenNINetwork::~OpenNINetwork()
	{
		delete[] mUIntMessage;
		mUIntMessage = NULL;
	}


	void OpenNINetwork::Init()
	{
		WSADATA data;
		WSAStartup( MAKEWORD(2,2), &data );

		//mSocketId = ::socket( AF_INET, SOCK_DGRAM, 0 );
		mSocketId = ::socket( PF_INET, mSocketType, IPPROTO_TCP );
		if( mSocketId == INVALID_SOCKET )
		{
			OutputDebugStringA( "Invalid socket!\n" );
			return;
		}

		int unused = true;
		setsockopt( mSocketId, SOL_SOCKET, SO_REUSEADDR, (char*)&unused, sizeof(unused) );

		struct sockaddr_in dest_addr;
		memset( &dest_addr, 0, sizeof(dest_addr) );
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr( mHostName.c_str() );
		dest_addr.sin_port = htons( mHostPort );
				
		if( ::bind(mSocketId, (struct sockaddr *)&dest_addr, sizeof(sockaddr_in)) == SOCKET_ERROR )
		{
			closesocket( mSocketId );
			OutputDebugStringA( "Failed on connection!\n" );
			return;
		}


		//if( mIsServer ) 
		{
			if( ::listen( mSocketId, 6) == SOCKET_ERROR )
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


	void OpenNINetwork::InitClient()
	{
		mUIntMessage = new uint16_t[640*480];

		WSADATA data;
		WSAStartup( MAKEWORD(2,0), &data );

		//mSocketId = ::socket( AF_INET, SOCK_DGRAM, 0 );
		mSocketId = ::socket( PF_INET, mSocketType, IPPROTO_TCP );
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


	void OpenNINetwork::Release()
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

	int OpenNINetwork::Send( char* message )
	{
		int res = ::send( mSocketId, message, strlen(message), 0 );

		return res;
	}


	int OpenNINetwork::ReceiveFloatBuffer( int count )
	{
		static const int bufferSize = 32;
		float message[bufferSize];
		int error = 0;
		int bytesReceived = 0;
		memset( message, 0, bufferSize*sizeof(float) );

		bytesReceived = ::recv( mSocketId, (char*)message, count*sizeof(float), 0 );
		error = WSAGetLastError();
		if( error != 0 ) 
		{
			CheckStatus( error );
			return -1;
		}
		//if( WSAGetLastError() == 0 )
// 				{
			std::stringstream ss;
			ss << "Size: " << bytesReceived << "   Message: " << message[2] << " " << std::endl;
			OutputDebugStringA( ss.str().c_str() );
// 				}

		return bytesReceived;
	}



#ifdef NETWORK_USE_THREAD
	void OpenNINetwork::run()
	{
		SOCKET AcceptSocket;

		while( mIsRunning )
		{
			AcceptSocket = SOCKET_ERROR;

			while( AcceptSocket == SOCKET_ERROR && mIsRunning )
			{
				AcceptSocket = accept( mSocketId, NULL, NULL );

				Sleep( 1 );
				//boost::this_thread::sleep( boost::posix_time::millisec(1) ); 
			}

			// else, accept the connection...
			// When the client connection has been accepted, transfer control from the
			// temporary socket to the original socket and stop checking for new connections.
			OutputDebugStringA( "Server: Client Connected!\n" );
			mSocketId = AcceptSocket;
			//break;
		}
	}
#endif


	void OpenNINetwork::CheckStatus( int error )
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
		case WSAENOTCONN:
			ss << "Socket not connected!" << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			break;
		default:
			ss << "Other error not listed!" << std::endl;
			OutputDebugStringA( ss.str().c_str() );
			break;
		}
	}
}
#endif
