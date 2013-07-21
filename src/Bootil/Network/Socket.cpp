
#include "Bootil/Bootil.h"

#ifdef _WIN32 
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else 
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#define ioctlsocket ioctl
	#define closesocket close
	#define WSAGetLastError() errno
#endif 

namespace Bootil
{
	namespace Network
	{
		Socket::Socket()
		{
			m_pSocket = 0;
		}

		bool Socket::InitAsListener( unsigned int iPort )
		{
			BAssert( m_pSocket == 0 );

			// create the socket
			m_pSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
			if ( m_pSocket == 0 )
				return false;

			// Set up some common config settings
			InitializeSocket();
			
			// bind the socket to this port
			struct sockaddr_in addr;
			addr.sin_family			= AF_INET;
			addr.sin_port			= htons( iPort );
			addr.sin_addr.s_addr	= htonl( INADDR_ANY );
			
			if ( bind( m_pSocket, (struct sockaddr *) &addr, sizeof(addr) ) == -1 )
			{
				Close();
				return false;
			}

			// start listening
			if ( listen( m_pSocket, 64 ) == -1 )
			{
				Close();
				return false;
			}

			return true;
		}

		Socket* Socket::Accept()
		{
			sockaddr saddr;
			socklen_t saddr_len = sizeof( saddr );

			unsigned int newsock = accept( m_pSocket, &saddr, &saddr_len );
			if ( newsock == -1 )
			{
				return NULL;
			}

			Socket* socket = new Socket();
			socket->m_pSocket = newsock;
			socket->InitializeSocket();

			return socket;
		}

		void Socket::Close()
		{
			if ( m_pSocket != 0 ) return;

			closesocket( m_pSocket );
			m_pSocket = 0;
		}

		bool Socket::IsConnected()
		{
			if ( m_pSocket == 0 ) return false;
			return true;
		}

		void Socket::InitializeSocket()
		{
			// Set to non blocking
			u_long iNonBlocking = 1;
			ioctlsocket( m_pSocket, FIONBIO, &iNonBlocking );

			// disable nagle 
			char iNagle = 1;
			setsockopt( m_pSocket, IPPROTO_TCP, TCP_NODELAY, &iNagle, sizeof(iNagle) );

			// Reusable address
			int reusable = 1;
			setsockopt( m_pSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reusable, sizeof(reusable) );
		}

		unsigned long Socket::ReadLength()
		{
			unsigned long readlength = 0;
			ioctlsocket( m_pSocket, FIONREAD, &readlength );

			return readlength;
		}

		unsigned long Socket::Read( void* pData, unsigned long iDataLen )
		{
			unsigned long iDataToRead = Bootil::Min( ReadLength(), iDataLen );
			if ( iDataToRead == 0 ) return 0;

			int ireceived = recv( m_pSocket, (char*) pData, iDataToRead, 0 );

			// Closed
			if ( ireceived == 0 )
			{
				Close();
				return 0;
			}

			// An error
			if ( ireceived < 0 )
			{
				if ( PreventedBlock() )	return 0; // It's normal, just chill

				Close();
				return 0;
			}

			return ireceived;
		}

		//
		// This returns true if winsock threw an error
		// because the socket would have blocked.. so
		// it shouldn't br treated as a real error.
		//
		bool Socket::PreventedBlock()
		{

#ifdef _WIN32	 
			if ( WSAGetLastError() == WSAEWOULDBLOCK )
				return true;
#else 
			if ( errno == EAGAIN ) return true;
			if ( errno == EINPROGRESS ) return true;
			if ( errno == EWOULDBLOCK ) return true;
#endif

			return false;
		}
	}
}