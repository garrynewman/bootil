#pragma once

namespace Bootil
{
	namespace Console
	{
		enum ConsoleColor
		{
			White,
			Black,
			Grey,
			Red,
			Blue,
			Green,
			Yellow
		};

		BOOTIL_EXPORT void FGColorPush( ConsoleColor col );
		BOOTIL_EXPORT void FGColorPop();

		BOOTIL_EXPORT void BGColorPush( ConsoleColor col );
		BOOTIL_EXPORT void BGColorPop();

		BOOTIL_EXPORT char WaitForKey();
	}
}