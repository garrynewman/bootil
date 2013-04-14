#define _WIN32_WINNT 0x0502

#include <windows.h>
#include "Bootil/Bootil.h"

namespace
{
	struct WatcherData
	{
		Bootil::BString directory;
		HANDLE directoryHandle;

		OVERLAPPED overlapped;

		char buffer[512];
	};

	void RecurseDirectories( Bootil::String::List & directories, const Bootil::BString & path )
	{
		Bootil::String::List currentDirectory;

		Bootil::File::Find( NULL, &currentDirectory, path + "/*", false );
		BOOTIL_FOREACH_CONST( directory, currentDirectory, Bootil::String::List )
		{
			Bootil::BString directoryPath = path + "/" + *directory;
			directories.push_back( directoryPath );
			RecurseDirectories( directories, directoryPath );
		}
	}
}

namespace Bootil
{
	namespace File
	{
		ChangeMonitor::ChangeMonitor()
		{
			m_dirHandles = NULL;
		}

		ChangeMonitor::~ChangeMonitor()
		{
			Stop();
		}

		bool ChangeMonitor::WatchFolder( const BString & strFolder, bool bWatchSubtree )
		{
			Stop();

			m_strFolderName = strFolder;
			m_bWatchSubtree = bWatchSubtree;

			m_dirHandles = new std::vector<WatcherData>;

			// Watch the main folder.

			WatcherData watcherData;

			watcherData.directory = strFolder;
			watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

			memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
			watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			std::fill( watcherData.buffer, watcherData.buffer + 512, 0 );

			( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );

			// And subfolders if we were told to.

			if ( m_bWatchSubtree )
			{
				Bootil::String::List directories;
				RecurseDirectories( directories, strFolder );
				BOOTIL_FOREACH_CONST( directory, directories, Bootil::String::List )
				{
					WatcherData watcherData;

					watcherData.directory = *directory;
					watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

					memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
					watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

					std::fill( watcherData.buffer, watcherData.buffer + 512, 0 );

					( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );
				}
			}

			StartWatch();

			return true;
		}

		void ChangeMonitor::NoteFileChanged( const BString & strChanged )
		{
			BOOTIL_FOREACH( str, m_Changes, std::list<BString> )
			{
				if ( ( *str ) == strChanged ) { return; }
			}
			m_Changes.push_back( strChanged );
		}

		void ChangeMonitor::Stop()
		{
			if ( !m_dirHandles )
			{
				return;
			}

			for ( WatcherData& watcherData: *( ( std::vector<WatcherData>* ) m_dirHandles ) )
			{
				CloseHandle( watcherData.directoryHandle );
				watcherData.directoryHandle = NULL;
			}

			delete m_dirHandles;
		}

		bool ChangeMonitor::HasChanges()
		{
			CheckForChanges();
			return m_Changes.size() > 0;
		}

		BString ChangeMonitor::GetChange()
		{
			if ( m_Changes.size() == 0 ) { return ""; }

			BString strName = m_Changes.front();
			m_Changes.pop_front();
			return strName;
		}

		void ChangeMonitor::CheckForChanges()
		{
			if ( !m_dirHandles )
			{
				return;
			}

			// Check for changes.

			for ( WatcherData& watcherData: *( ( std::vector<WatcherData>* ) m_dirHandles ) )
			{
				CancelIo( watcherData.directoryHandle );

				DWORD dwNumberBytes;

				GetOverlappedResult( watcherData.directoryHandle, &watcherData.overlapped, &dwNumberBytes, FALSE );

				ResetEvent( watcherData.overlapped.hEvent );

				int iOffset = 0;
				while ( true )
				{
					FILE_NOTIFY_INFORMATION* pNotify = ( FILE_NOTIFY_INFORMATION* ) &watcherData.buffer[iOffset];
					iOffset += pNotify->NextEntryOffset;

					if ( pNotify->FileNameLength > 0 )
					{
						// Found a changed file. Get the relative path to it.

						BString path;

						if ( watcherData.directory == m_strFolderName )
						{
							path = "";
						}
						else
						{
							path = watcherData.directory.substr( m_strFolderName.size() + 1 ) + "/";	
						}

						path += Bootil::String::Convert::FromWide( WString( pNotify->FileName, pNotify->FileNameLength / sizeof( wchar_t ) ) );

						// Add it to the change list.

						NoteFileChanged( path );
					}

					if ( pNotify->NextEntryOffset == 0 )
					{
						// We reached the end of the changes. Start looking for them again.

						std::fill( watcherData.buffer, watcherData.buffer + 512, 0 );

						ReadDirectoryChangesW( watcherData.directoryHandle, &watcherData.buffer, sizeof( watcherData.buffer ), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &watcherData.overlapped, NULL );

						break;
					}
				}
			}
		}

		void ChangeMonitor::StartWatch()
		{
			// Start looking for changes.

			for ( WatcherData& watcherData: *( ( std::vector<WatcherData>* ) m_dirHandles ) )
			{
				std::fill( watcherData.buffer, watcherData.buffer + 512, 0 );

				ReadDirectoryChangesW( watcherData.directoryHandle, &watcherData.buffer, sizeof( watcherData.buffer ), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &watcherData.overlapped, NULL );
			}

			CheckForChanges();
		}
	}
}

