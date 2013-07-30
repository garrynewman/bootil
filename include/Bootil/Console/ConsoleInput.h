#pragma once

namespace Bootil
{
	namespace Console
	{
		class BOOTIL_EXPORT Input : public Bootil::Threads::Thread
		{
			public:

				void Run();

				BString GetLine();
				BString GetLineInProgress();

			protected:

				bool IsEndOfLine( int c );
				bool IsCancel( int c );
				void LineFinished();

				Bootil::BString m_strLine;
				std::queue<Bootil::BString>	m_Lines;
		};
	}
}