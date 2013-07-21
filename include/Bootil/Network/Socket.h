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

				//
				// Regular connection mode
				//
				bool Connect( const Bootil::BString& strIP, unsigned int iPort );

				// Returns true if connection is all fine and dandy
				bool IsConnected();

				// Returns true if we're in the process of connecting
				bool IsConnecting();

				// Closes the socket, ends all connections.
				void Close();

				//
				// Cycle - sends and received - call every frame!
				//
				void Cycle();

				//
				// Writing
				//
				bool Write( void* pData, unsigned long iDataLen );

				//
				// Returns the read buffer
				//
				Bootil::Buffer& Read();
				
			private:

				void InitializeSocket();
				bool PreventedBlock();
				void SendQueued();
				void ReceiveToQueue();
				void FinishConnecting();

			protected:

				int					m_pSocket;
				Bootil::AutoBuffer	m_SendQueue;
				Bootil::AutoBuffer	m_RecvQueue;
				

				bool				m_bAttemptingConnect;
				Time::Timer			m_ConnectionTimer;
		};

	}
}
