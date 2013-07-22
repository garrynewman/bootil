#pragma once

namespace Bootil
{
	namespace Network
	{
		//
		// Use this as a base class on whatever object you want to 
		// receive messages from the router.
		//
		class MsgHandler
		{
			public:

				typedef void ( MsgHandler::*FunctionWithData )( Bootil::Buffer& data );
		};

		//
		// You can use this as a base class or as a member variable to parse
		// messages from a socket. The messages should be in the format:
		//
		// 1: TPacketSize		a char/short/int etc describing the of the packet
		// 2: TMessageType		a char/short/int etc with the message type
		// 3: Data				data of the message
		//
		// This class is already derived from MsgHandler so that if you derive
		// a class from this - you can handle messages on it without having to 
		// derive from MsgHandler too.
		//
		//
		template < typename TPacketSize, typename TMessageType >
		class Router : public MsgHandler
		{
			public:

				struct FunctionCallback
				{
					MsgHandler*						pthis;
					MsgHandler::FunctionWithData	fncWithData;

				};

				typedef std::map<TMessageType, FunctionCallback>	ProcessorMap;

				//
				// Processes the received data. Call this as often as possible!
				//
				void ParseMessages( Bootil::Network::Socket& socket )
				{
					if ( !socket.IsConnected() ) return;

					Bootil::Buffer& data = socket.Read();
					data.SetPos( 0 );

					while ( ProcessNetworkMessage( data ) ){}
				}

				//
				// Bind a received message to a function call
				//
				template< typename TFunction >
				void SetHandler( TMessageType type, MsgHandler* pthis, TFunction fnc )
				{
					BAssert( m_Processor.find( type ) == m_Processor.end() ); // Shouldn't be multiple handlers for one type

					FunctionCallback cb;
					cb.pthis = this;
					cb.fncWithData = static_cast<MsgHandler::FunctionWithData>(fnc);

					m_Processor[ type ] = cb;
				}

				//
				// Writes a message in the correct format
				//
				bool WriteMessage( Bootil::Network::Socket& socket, TMessageType msg, Bootil::Buffer& data )
				{
					if ( !socket.Write( data.GetWritten() ) ) return false;
					if ( !socket.Write( msg ) ) return false;
					return socket.WriteData( data );
				}

			protected:

				bool ProcessNetworkMessage( Bootil::Buffer& data )
				{
					//
					// Not enough data to get the packet size
					//
					if ( data.GetWritten() < sizeof( TPacketSize ) ) return false;

					//
					// Get the packet size
					//
					TPacketSize iPacketSize = data.ReadType<TPacketSize>();

					//
					// Packet is larger than the amount of data we have. Wait for more.
					//
					if ( iPacketSize > data.GetWritten() - sizeof( TPacketSize ) - sizeof( TMessageType ) ) return false;

					//
					// Get the message type
					//
					TMessageType iMessageType = data.ReadType<TMessageType>();

					//
					// Put the data in its own sub-buffer to send to the handler
					//
					Bootil::Buffer isolated_data = Bootil::Buffer( data.GetCurrent(), iPacketSize );

					//
					// Call the actual function
					//
					CallNetworkMessage( iMessageType, isolated_data );

					//
					// This message has been processed, so we can crop it off the buffer
					//
					data.TrimLeft( data.GetPos() + iPacketSize );
					data.SetPos( 0 );
					return true;
				}

				void CallNetworkMessage( TMessageType type, Bootil::Buffer& data )
				{
					ProcessorMap::iterator it = m_Processor.find( type );
					if ( it == m_Processor.end() )
					{
						#ifdef _DEBUG 
						Bootil::Output::Warning( "unhandled message %i\n", (int) type );
						#endif 
						return;
					}

					(((it->second).pthis)->*(it->second).fncWithData)( data );
				}
				


			private:

				ProcessorMap		m_Processor;
		};

	}
}
