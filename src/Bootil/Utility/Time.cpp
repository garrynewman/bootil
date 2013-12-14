
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
			return ( (double) t) / CLOCKS_PER_SEC;
		}

		BOOTIL_EXPORT float MilliSeconds()
		{
			clock_t t = clock();
			return ( (double) t) / ( CLOCKS_PER_SEC * 1000.0 );
		}

		BOOTIL_EXPORT Bootil::BString TimeStamp()
		{
			time_t rawtime;
			struct tm * timeinfo;
			static char buffer[512];

			time( &rawtime );
			timeinfo = localtime( &rawtime );

			strftime( buffer, 512, "%H:%M", timeinfo );

			return buffer;
		}

		BOOTIL_EXPORT Bootil::BString TimeAndDateStamp()
		{
			time_t rawtime;
			struct tm * timeinfo;
			static char buffer[512];

			time( &rawtime );
			timeinfo = localtime( &rawtime );

			strftime( buffer, 512, "%Y-%m-%d %H:%M", timeinfo );

			return buffer;
		}
	}

}