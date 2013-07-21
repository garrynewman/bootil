#pragma once

namespace Bootil
{
	namespace Network
	{
		class Socket
		{
			public:

				Socket();

				//
				// Listener Mode (Start as a server)
				//
				bool InitAsListener( unsigned int iPort );
				Socket* Accept();



				// Returns true if connected
				bool IsConnected();

				// Closes the socket, ends all connections
				void Close();


				//
				// Reading
				//
				unsigned long ReadLength();
				unsigned long Read( void* pData, unsigned long iDataLen );
				
			private:

				void InitializeSocket();
				bool PreventedBlock();

			protected:

				int		m_pSocket;
		};

	}
}
