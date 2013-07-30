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

				char c = input[i].Event.KeyEvent.uChar.AsciiChar;

				if ( IsEndOfLine( c ) )
				{
					LineFinished();

					if ( m_bOutputInput )
						Output::Msg( "\n" );
				}
				else if ( IsCancel( c ) )
				{
					m_strLine = "";
				}
				else
				{
					m_strLine += c;

					if ( m_bOutputInput )
						Output::Msg( "%c", c );
				}

			}

			// Recurse.
			Update();
#else 

#endif 
		}

		bool Input::IsEndOfLine( int c )
		{
			if ( c <= 0 ) return true;
			if ( c == '\n' ) return true;
			if ( c == '\r' ) return true;

			return false;
		}

		bool Input::IsCancel( int c )
		{
			if ( c == 27 ) return true;

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
