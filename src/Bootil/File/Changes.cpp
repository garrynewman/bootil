
#include "Bootil/Bootil.h"

#ifdef _WIN32
#define _WIN32_WINNT  0x0502
#include <windows.h>
#elif __linux__
#include <sys/select.h>
#include <sys/inotify.h>

typedef std::map<int, Bootil::BString> WatchMap;

namespace
{
	// Slightly ugly, but it works.
	void RecurseDirectories(Bootil::String::List& folders, const Bootil::BString& folder)
	{
		Bootil::String::List curDir;
		int i = Bootil::File::Find( NULL, &curDir, folder + "/*", false );

		BOOTIL_FOREACH_CONST( f, curDir, Bootil::String::List )
		{
			folders.push_back( *f );
			RecurseDirectories( folders, *f );
		}
	}
}

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
			#elif __linux__
				m_pData = (void*)new int( inotify_init() );
			#endif			
		}

		ChangeMonitor::~ChangeMonitor()
		{
			#ifdef _WIN32
				delete ((OVERLAPPED*)m_pData);
			#endif

			Stop();

			#ifdef __linux__
				close( *(int*)m_pData );
				delete (int*)m_pData;
			#endif
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

			#elif __linux__

				int flags = IN_CREATE|IN_DELETE|IN_MODIFY|IN_MOVED_FROM|IN_MOVED_TO;
				int handle;
				m_dirHandle = new WatchMap();

				handle = inotify_add_watch( *(int*)m_pData, strFolder.c_str(), flags );
				if ( handle < 0 )
					return false;

				(*(WatchMap*)m_dirHandle)[handle] = strFolder;

				if ( bWatchSubtree )
				{
					String::List folders;
					RecurseDirectories( folders, strFolder );

					BOOTIL_FOREACH_CONST( folder, folders, String::List )
					{
						handle = inotify_add_watch( *(int*)m_pData, folder->c_str(), flags );

						if ( handle < 0 )
							return false;

						(*(WatchMap*)m_dirHandle)[handle] = *folder;
					}
				}

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
				#elif __linux__
					BOOTIL_FOREACH_CONST( handle, (*(WatchMap*)m_dirHandle), WatchMap )
					{
						inotify_rm_watch( *(int*)m_pData, handle->first );
					}

					delete (WatchMap*)m_dirHandle;
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

			#elif __linux__

				int inotify_fd = *(int*)m_pData;

				timeval timeout;
				timeout.tv_sec = 0;
				timeout.tv_usec = 0;

				fd_set fds;
				FD_ZERO( &fds );
				FD_SET( inotify_fd, &fds );

				select( inotify_fd + 1, &fds, NULL, NULL, &timeout );

				if ( FD_ISSET(inotify_fd, &fds) )
				{
					int i = 0;
					int ret = read( inotify_fd, m_Buffer, sizeof(m_Buffer) );

					if ( ret < 0 )
						return;

					while ( i < ret )
					{	
						inotify_event* event = (inotify_event*)&m_Buffer[i];

						if (event->len > 0)
						{
							NoteFileChanged( (*(WatchMap*)m_dirHandle)[event->wd] + "/" + event->name );
						}

						i += sizeof( inotify_event ) + event->len;
					}
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

