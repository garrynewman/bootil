#pragma once

namespace Bootil
{
	namespace Network
	{
		class Socket
		{
			public:

				Socket();

				// Listener
				bool InitAsListener( unsigned int iPort );
				Socket* Accept();

				void Close();
				
			private:

				void InitializeSocket();

			protected:

				int		m_pSocket;
		};

	}
}
