#include "Bootil/Bootil.h"
#include <conio.h>

namespace Bootil
{
	namespace Console
	{
		void Input::Run()
		{
			while ( !WantsToClose() )
			{
				//
				// This blocks until it has a character
				//
				int c = getch();

				Lock();

				if ( IsEndOfLine(c) )
				{
					LineFinished();
				}
				else if ( IsCancel( c ) )
				{
					m_strLine = "";
				}
				else
				{
					m_strLine += c;
				}

				Unlock();
			}
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
			BString str = "";

			Lock();
			if ( m_Lines.size() > 0 )
			{
				str = m_Lines.front();
				m_Lines.pop();
			}
			Unlock();	

			return str;
		}

		BString Input::GetLineInProgress()
		{
			BString str = "";

			Lock();
			str = m_strLine;
			Unlock();	

			return str;
		}

		void Input::LineFinished()
		{
			m_Lines.push( m_strLine );
			m_strLine = "";
		}

	}
}
