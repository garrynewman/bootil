#pragma once

namespace Bootil
{
	namespace Console
	{
		class BOOTIL_EXPORT Input
		{
			public:

				BString GetLine();
				BString GetLineInProgress();
				void SetOutputInput( bool b );

			protected:

				void Update();

				bool IsEndOfLine( int c );
				bool IsCancel( int c );
				void LineFinished();

				Bootil::BString					m_strLine;
				std::queue<Bootil::BString>		m_Lines;

				bool	m_bOutputInput;
		};
	}
}