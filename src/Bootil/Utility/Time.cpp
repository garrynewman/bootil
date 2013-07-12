
#include "Bootil/Bootil.h"
#include <ctime>

namespace Bootil
{
	namespace Time
	{
		BOOTIL_EXPORT unsigned long long UnixTimestamp()
		{
			return ( unsigned long long ) time( NULL );
		}

		BOOTIL_EXPORT Bootil::BString UnixTimestampAsString()
		{
			return Bootil::String::Format::Print( "%llu", UnixTimestamp() );
		}

		BOOTIL_EXPORT float Seconds()
		{
			clock_t t = clock();
			return ( (float) t) / CLOCKS_PER_SEC;
		}

		BOOTIL_EXPORT float MilliSeconds()
		{
			clock_t t = clock();
			return ( (double) t) / ( CLOCKS_PER_SEC * 1000.0 );
		}
	}

}