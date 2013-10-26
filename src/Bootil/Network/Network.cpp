
#include "Bootil/Bootil.h"

#ifdef _WIN32 
	#include <winsock2.h>
#else 
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif 


namespace Bootil
{
	namespace Network
	{
		BOOTIL_EXPORT void Start()
		{
			#ifdef _WIN32
			WSAData wsaData;
			WSAStartup( MAKEWORD(1, 1), &wsaData );
			#endif 
		}

		BOOTIL_EXPORT void End()
		{
			#ifdef _WIN32

			WSACleanup();
			
			#endif 
		}

		namespace Util 
		{
			BOOTIL_EXPORT BString IpToString( unsigned long IP )
			{
				struct in_addr addr;
				addr.s_addr = IP;

				const char* strResult = inet_ntoa( addr );
				if ( !strResult ) return "";

				return strResult;
			}

			BOOTIL_EXPORT unsigned long StringToIp( const BString& strIP )
			{
				return htonl( inet_addr( strIP.c_str() ) );
			}
		}
	}
}