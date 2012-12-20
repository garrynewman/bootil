
#include "Bootil/Bootil.h"
#include <ctime>

namespace Bootil 
{
	namespace Time
	{
		BOOTIL_EXPORT unsigned long long UnixTimestamp()
		{
			return (unsigned long long) time( NULL );
		}

		BOOTIL_EXPORT Bootil::BString UnixTimestampAsString()
		{
			return Bootil::String::Format::Print( "%llu", UnixTimestamp() );
		}
	}

}