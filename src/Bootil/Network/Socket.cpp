
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
			m_pSocket = INVALID_SOCKET;
		}

		bool Socket::InitAsListener( unsigned int iPort )
		{
			BAssert( m_pSocket == INVALID_SOCKET );

			// create the socket
			m_pSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
			if ( m_pSocket == INVALID_SOCKET )
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
			if ( m_pSocket != INVALID_SOCKET ) return;

			closesocket( m_pSocket );
			m_pSocket = INVALID_SOCKET;
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
	}
}