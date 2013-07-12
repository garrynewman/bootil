#pragma once

namespace Bootil
{
	namespace Time
	{
		BOOTIL_EXPORT unsigned long long	UnixTimestamp();
		BOOTIL_EXPORT Bootil::BString		UnixTimestampAsString();

		BOOTIL_EXPORT float Seconds();
		BOOTIL_EXPORT float MilliSeconds();

		class Timer
		{
			public:

				Timer()
				{
					Reset();
				}

				void Reset( float fTimeOffset = 0.0f )
				{
					m_TimeStarted = Bootil::Time::Seconds() - fTimeOffset;
				}

				float Seconds()
				{
					return Bootil::Time::Seconds() - m_TimeStarted;
				}

				float m_TimeStarted;
				float m_fScale;
		};
	}

}