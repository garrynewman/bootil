#include "Bootil/Bootil.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace Bootil
{
	namespace Console 
	{
		static std::stack<ConsoleColor>	s_fg;
		static std::stack<ConsoleColor>	s_bg;

		void UpdateColor()
		{
			ConsoleColor fg = Grey;
			ConsoleColor bg = Black;

			if ( !s_fg.empty() ) fg = s_fg.top();
			if ( !s_bg.empty() ) bg = s_bg.top();

#ifdef _WIN32

			unsigned short win_bg = 0;
			unsigned short win_fg = 0;

			switch ( fg )
			{
				case White:		win_fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
				case Black:		win_fg = 0; break;
				case Grey:		win_fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
				case Red:		win_fg = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
				case Blue:		win_fg = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
				case Green:		win_fg = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
				case Yellow:	win_fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
			}

			switch ( bg )
			{
				case White:		win_bg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
				case Black:		win_bg = 0; break;
				case Grey:		win_bg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
				case Red:		win_bg = BACKGROUND_RED | BACKGROUND_INTENSITY; break;
				case Blue:		win_bg = BACKGROUND_BLUE | BACKGROUND_INTENSITY; break;
				case Green:		win_bg = BACKGROUND_GREEN | BACKGROUND_INTENSITY; break;
				case Yellow:	win_bg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY; break;
			}

			HANDLE hcon = GetStdHandle( STD_OUTPUT_HANDLE );
			SetConsoleTextAttribute( hcon, win_bg | win_fg );

#endif

		}

		BOOTIL_EXPORT void FGColorPush( ConsoleColor col )
		{
			s_fg.push( col );
			UpdateColor();
		}

		BOOTIL_EXPORT void FGColorPop()
		{
			s_fg.pop();
			UpdateColor();
		}

		BOOTIL_EXPORT void BGColorPush( ConsoleColor col )
		{
			s_bg.push( col );
			UpdateColor();
		}

		BOOTIL_EXPORT void BGColorPop()
		{
			s_bg.pop();
			UpdateColor();
		}

		BOOTIL_EXPORT char WaitForKey()
		{
			char c;

			c = std::getchar();

			return c;
		}
	}
}
