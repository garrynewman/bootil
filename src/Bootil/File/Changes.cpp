
#include "Bootil/Bootil.h"

#ifdef _WIN32
#define _WIN32_WINNT  0x0502
#include <windows.h>
#endif 

namespace Bootil 
{
	namespace File 
	{
		ChangeMonitor::ChangeMonitor()
		{
			m_dirHandle		= NULL;
			m_pData			= NULL;
			m_bWatchSubtree	= false;

			memset( m_Buffer, 0, sizeof(m_Buffer) );

			#ifdef _WIN32
				m_pData = (void*)new OVERLAPPED;
				memset( m_pData, 0, sizeof(OVERLAPPED) );
			#endif			
		}

		ChangeMonitor::~ChangeMonitor()
		{
			#ifdef _WIN32
				delete ((OVERLAPPED*)m_pData);
			#endif

			Stop();
		}

		bool ChangeMonitor::WatchFolder( const BString& strFolder, bool bWatchSubtree )
		{
			Stop();

			m_bWatchSubtree = bWatchSubtree;

			#ifdef _WIN32

				memset( m_pData, 0, sizeof(OVERLAPPED) );

				m_dirHandle = CreateFileA( strFolder.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );
				if ( m_dirHandle == INVALID_HANDLE_VALUE )
					return false;

				((OVERLAPPED*)m_pData)->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

				StartWatch();

			#endif

			m_strFolderName = strFolder;
			return true;
		}

		void ChangeMonitor::NoteFileChanged( const BString& strChanged )
		{
			BOOTIL_FOREACH( str, m_Changes, std::list<BString> )
			{
				if ( (*str) == strChanged ) return;
			}

			m_Changes.push_back( strChanged );
		}

		void ChangeMonitor::Stop()
		{
			if ( m_dirHandle )
			{
				#ifdef _WIN32
					CloseHandle( m_dirHandle );
					m_dirHandle = NULL;
				#endif
			}

			#ifdef _WIN32 
				if ( ((OVERLAPPED*)m_pData)->hEvent )
				{
					CloseHandle(((OVERLAPPED*)m_pData)->hEvent);
					((OVERLAPPED*)m_pData)->hEvent = NULL;
				}
			#endif 
		}

		bool ChangeMonitor::HasChanges()
		{
			CheckForChanges();
			return m_Changes.size() > 0;
		}

		BString ChangeMonitor::GetChange()
		{
			if ( m_Changes.size() == 0 ) return "";

			BString strName = m_Changes.front();
			m_Changes.pop_front();
			return strName;
		}

		void ChangeMonitor::CheckForChanges()
		{
			if ( m_dirHandle == NULL ) return;

			#ifdef _WIN32 

				DWORD dwNumberbytes;
				if ( !GetOverlappedResult( m_dirHandle, ((OVERLAPPED*)m_pData), &dwNumberbytes, false ) )
					return;

				ResetEvent( ((OVERLAPPED*)m_pData)->hEvent );

				int iOffset = 0;

				while( true )
				{
					FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)&m_Buffer[iOffset];

					BString strName = Bootil::String::Convert::FromWide( WString( pNotify->FileName, pNotify->FileNameLength / sizeof(wchar_t) ) );

					if ( pNotify->FileNameLength > 0 )
					{
						NoteFileChanged( strName );
					}

					if ( pNotify->NextEntryOffset == 0 ) 
					{
						StartWatch();
						return;
					}

					iOffset += pNotify->NextEntryOffset;
				}

			#endif

		}

		void ChangeMonitor::StartWatch()
		{
			#ifdef _WIN32 
				ReadDirectoryChangesW( m_dirHandle, m_Buffer, sizeof(m_Buffer), m_bWatchSubtree, FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, ((OVERLAPPED*)m_pData), NULL );
			#endif 

			CheckForChanges();
		}
	}
}

