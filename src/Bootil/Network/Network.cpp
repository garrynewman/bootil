
#include "Bootil/Bootil.h"

#ifdef _WIN32 
	#include <winsock2.h>
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
	}
}