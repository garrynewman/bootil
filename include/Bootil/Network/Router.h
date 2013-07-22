#pragma once

namespace Bootil
{
	namespace Network
	{
		//
		// This is the standard message that is passed from the Router
		// to its callbacks and handlers.
		//
		struct Message
		{
			Message( Bootil::Buffer& d ) : data( d )
			{
				type			= 0;
				messageid		= 0;
				replyingto		= 0;
			}

			Bootil::Buffer&			data;
			unsigned int			type;
			unsigned int			messageid;
			unsigned int			replyingto;
		};

		//
		// Use this as a base class on whatever object you want to 
		// receive messages from the router.
		//
		class MsgHandler
		{
			public:

				typedef void ( MsgHandler::*FunctionWithData )( Message& data );
		};


		//
		// This can be used as a base class or as a member variable to parse
		// messages from a socket. The messages are in the format:
		//
		// 1: TPacketSize		a char/short/int etc describing the of the packet
		// 2: MessageID			A unique Id for this message
		// 3: ReplyID			If this message is replying to another, this is its ID
		// 4: TMessageType		a char/short/int etc with the message type
		// 5: Data				data of the message
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

				//
				// I'm undecided on what MessageID should be. 
				// We could probably get away with an unsigned char.
				//
				typedef unsigned short MessageID;


				struct FunctionCallback
				{
					void*							pthis;
					MsgHandler::FunctionWithData	fncWithData;

				};

				typedef std::map<TMessageType, FunctionCallback>	ProcessorMap;
				typedef std::map<MessageID, FunctionCallback>		ResponderMap;

				Router()
				{
					m_iMessageID = 0;
				}

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
				void SetHandler( TMessageType type, void* pthis, TFunction fnc )
				{
					FunctionCallback cb;
						cb.pthis		= pthis;
						cb.fncWithData	= static_cast<MsgHandler::FunctionWithData>(fnc);
					m_Processor[ type ] = cb;

				}

				//
				// Call a function when a message is replied to
				//
				template< typename TFunction >
				void ReplyHandler( MessageID iReplyTo, void* pthis, TFunction fnc )
				{
					FunctionCallback cb;
						cb.pthis		= pthis;
						cb.fncWithData	= static_cast<MsgHandler::FunctionWithData>(fnc);
					m_Responder[ iReplyTo ] = cb;
				}

				//
				// Writes a message in the correct format
				//
				MessageID WriteMessage( Bootil::Network::Socket& socket, TMessageType msg, Bootil::Buffer& data, MessageID iRespondingToMsg = 0 )
				{
					m_iMessageID++;

					// We may have looped, so clear the reply handler
					ClearReplyHandler( m_iMessageID );

					socket.Write( data.GetWritten() );
					socket.Write( m_iMessageID );
					socket.Write( iRespondingToMsg );
					socket.Write( msg );
					socket.WriteData( data );

					return m_iMessageID;
				}


				void ClearReplyHandler( MessageID i )
				{
					m_Responder.erase( i );
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
					// The header consists of the packet size, message type and message ID's
					//
					unsigned int iHeaderSize = sizeof( TPacketSize ) + sizeof( TMessageType ) + sizeof( MessageID ) * 2;

					//
					// Packet is larger than the amount of data we have. Wait for more.
					//
					if ( iPacketSize + iHeaderSize > data.GetWritten() ) return false;

					//
					// Get the messageid and request id
					//
					MessageID iMessageID = data.ReadType<MessageID>();
					MessageID iReplyID = data.ReadType<MessageID>();

					//
					// Get the message type
					//
					TMessageType iMessageType = data.ReadType<TMessageType>();

					//
					// Put the data in its own sub-buffer to send to the handler
					//
					Bootil::Buffer isolated_data = Bootil::Buffer( data.GetCurrent(), iPacketSize );

					Message msg( isolated_data );
						msg.messageid		= iMessageID;
						msg.replyingto		= iReplyID;
						msg.type			= iMessageType;

					//
					// Call the actual function
					//
					CallNetworkMessage( msg );

					//
					// Call the reply handler
					//
					CallReplyHandler( msg );

					//
					// This message has been processed, so we can crop it off the buffer
					//
					data.TrimLeft( data.GetPos() + iPacketSize );
					data.SetPos( 0 );
					return true;
				}

				//
				// Call a regular message handler
				//
				void CallNetworkMessage( Message& msg )
				{
					ProcessorMap::iterator it = m_Processor.find( msg.type );
					if ( it == m_Processor.end() ) return;

					(((MsgHandler*)(it->second).pthis)->*(it->second).fncWithData)( msg );
				}

				//
				// Call a handler in response to a reply code
				//
				void CallReplyHandler( Message& msg )
				{
					if ( msg.replyingto == 0 ) return;

					ResponderMap::iterator it = m_Responder.find( msg.replyingto );
					if ( it == m_Responder.end() ) return;

					((MsgHandler*)((it->second).pthis)->*(it->second).fncWithData)( msg );

					ClearReplyHandler( msg.replyingto );
				}

			private:

				MessageID			m_iMessageID;
				ResponderMap		m_Responder;
				ProcessorMap		m_Processor;
		};

	}
}
