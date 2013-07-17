
#include "Bootil/Bootil.h"
#include <ctime>

#include "../../3rdParty/happyhttp/happyhttp.h"

#ifdef _WIN32
#include <winsock2.h>
#endif 

namespace Bootil
{
	namespace HTTP
	{

		BOOTIL_EXPORT void Open()
		{
			#ifdef _WIN32
			{
				WSAData wsaData;
				int code = WSAStartup( MAKEWORD(1, 1), &wsaData );
				if ( code != 0 )
				{

				}
			}
			#endif 
		}

		BOOTIL_EXPORT void Close()
		{
			#ifdef _WIN32
			{
				WSACleanup();
			}
			#endif 
		}

		//
		// Add this data to the response
		//
		void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
		{
			Query* q = static_cast<Query*>(userdata);
			q->GetResponse().Write( data, n );
		}

		void OnComplete( const happyhttp::Response* r, void* userdata ){}

		void Query::Run()
		{
			m_Response.Clear();

			try
			{
				Lock();
				happyhttp::Connection conn( m_strHost.c_str(), m_iPort );
				conn.setcallbacks( NULL, OnData, NULL, (void*) this );

				conn.putrequest( m_strMethod.c_str(), m_strRequest.c_str() );
					conn.putheader( "Accept", "*/*" );
					conn.putheader( "User-Agent", "Agent:Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/27.0.1453.116 Safari/537.36" );

					if ( !m_PostParams.empty() )
					{
						conn.putheader( "Content-Length", m_PostParams.length() );
						conn.putheader( "Content-type", "application/x-www-form-urlencoded" );
					}

				conn.endheaders();

				conn.send( (const unsigned char*) m_PostParams.c_str(), m_PostParams.length() );
				Unlock();

				while( conn.outstanding() )
				{
					conn.pump();
				}
			}
			catch ( happyhttp::Wobbly& e )
			{
				// Failed for some reason.
			}
		}


		void Query::SetPostVar( const Bootil::BString& k, const Bootil::BString& v )
		{
			if ( !m_PostParams.empty() ) m_PostParams += "&";

			// URLENCODE
			m_PostParams += k + "=" + v;
		}

	}

}