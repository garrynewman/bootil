#include "Bootil/Bootil.h"
#include <conio.h>

#ifdef _WIN32
#include <windows.h>
#endif 

namespace Bootil
{
	namespace Console
	{
		void Input::Update()
		{
#ifdef _WIN32
			HANDLE stdIn = GetStdHandle( STD_INPUT_HANDLE );

			DWORD eventnum = 0;
			GetNumberOfConsoleInputEvents( stdIn, &eventnum );
			if ( eventnum <= 0 ) return;

			INPUT_RECORD input[ 128 ];
			ReadConsoleInput( stdIn, input, 128, &eventnum );
			if ( eventnum <= 0 ) return;

			for ( int i=0; i<eventnum; i++ )
			{
				if ( input[i].EventType != KEY_EVENT )
					continue;

				if ( !input[i].Event.KeyEvent.bKeyDown )
					continue;

				// Backspace
				if ( input[i].Event.KeyEvent.wVirtualKeyCode == VK_BACK )
				{
					if ( m_strLine.length() > 0 ) m_strLine.pop_back();
					continue;
				}

				// Return/Enter
				if ( IsEndOfLine( input[i].Event.KeyEvent.wVirtualKeyCode ) )
				{
					LineFinished();

					if ( m_bOutputInput )
						Output::Msg( "\n" );

					continue;
				}

				// Escape
				if ( IsCancel( input[i].Event.KeyEvent.wVirtualKeyCode ) )
				{
					m_strLine = "";
				}

				char c = input[i].Event.KeyEvent.uChar.AsciiChar;
				if ( c == 0 )
					continue;
	
				m_strLine.push_back( c );

				if ( m_bOutputInput )
					Output::Msg( "%c", c );

			}

			// Recurse.
			Update();
#else 

#endif 
		}

		bool Input::IsEndOfLine( int c )
		{
#ifdef _WIN32
			if ( c == VK_RETURN ) return true;
#endif 

			return false;
		}

		bool Input::IsCancel( int c )
		{
#ifdef _WIN32
			if ( c == VK_ESCAPE ) return true;
#endif

			return false;
		}

		BString Input::GetLine()
		{
			Update();

			BString str = "";

			if ( m_Lines.size() > 0 )
			{
				str = m_Lines.front();
				m_Lines.pop();
			}

			return str;
		}

		BString Input::GetLineInProgress()
		{
			Update();

			return m_strLine;
		}

		void Input::LineFinished()
		{
			m_Lines.push( m_strLine );
			m_strLine = "";
		}

		void Input::SetOutputInput( bool b )
		{
			m_bOutputInput = b;
		}


	}
}
