
#include "Bootil/Bootil.h"
#ifdef _WIN32
#define _WIN32_WINNT 0x0502

#include <windows.h>
#elif defined( __APPLE__ ) && defined( __MACH__ )
#include <CoreServices/CoreServices.h>
#include <libgen.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED < __MAC_10_7
#define kFSEventStreamCreateFlagFileEvents 0x00000010
#endif
#elif defined( __linux__ )
#include <unistd.h>
#include <sys/select.h>
#include <sys/inotify.h>
#endif

namespace
{
#ifdef _WIN32
	const char separator = '\\';
#else
	const char separator = '/';
#endif

	struct WatcherData
	{
		Bootil::BString directory;
#ifdef _WIN32
		HANDLE directoryHandle;

		OVERLAPPED overlapped;

		char buffer[1024];
#elif defined( __linux__ )
		int handle;
#endif
	};

#if defined( __linux__ )
	struct WatcherFind
	{
		int handle;

		bool operator()(const WatcherData& b) { return handle == b.handle; }
	};
#endif

#if !defined( __APPLE__ ) || !defined( __MACH__ ) // if not OS X
	void RecurseDirectories( Bootil::String::List & directories, const Bootil::BString & path )
	{
		Bootil::String::List currentDirectory;

		Bootil::File::Find( NULL, &currentDirectory, path + separator + "*", false );
		BOOTIL_FOREACH_CONST( directory, currentDirectory, Bootil::String::List )
		{
			Bootil::BString directoryPath = path + separator + *directory;
			directories.push_back( directoryPath );
			RecurseDirectories( directories, directoryPath );
		}
	}
#endif
}

namespace Bootil
{
	namespace File
	{
		ChangeMonitor::ChangeMonitor()
		{
			m_dirHandles = NULL;
#if defined( __APPLE__ ) && defined( __MACH__ )
			m_fsStreamRef = NULL;
#elif defined( __linux__ )
			m_inotify = inotify_init();
#endif
		}

		ChangeMonitor::~ChangeMonitor()
		{
			Stop();
#if defined( __linux__ )
			close( m_inotify );
#endif
		}

#if defined( __APPLE__ ) && defined( __MACH__ )
		void ChangeMonitor::HandleFSEvent( const __FSEventStream *stream, void *ctx, size_t numEvents, void *eventPaths, const UInt32 evFlags[], const UInt64 evIds[] )
		{
			char **paths = ( char ** )eventPaths;
			for ( size_t i = 0; i < numEvents; i++ )
			{
				ChangeMonitor *monitor = ( ChangeMonitor * )ctx;
				char *path = paths[i];
				if ( strcmp ( dirname( path ), monitor->m_strFolderName.c_str() ) != 0
					&& strcmp ( path, monitor->m_strFolderName.c_str() ) != 0
					&& !monitor->m_bWatchSubtree )
				{
					continue;
				}

				// Found a changed file. Get the relative path to it.

				BString strPath( path );

				if ( strPath == monitor->m_strFolderName )
				{
					strPath = "";
				}
				else
				{
					strPath = strPath.substr( monitor->m_strFolderName.size() + 1 );
				}

				// Add it to the change list.

				monitor->NoteFileChanged( path );
			}
		}
#endif

		bool ChangeMonitor::WatchFolder( const BString & strFolder, bool bWatchSubtree )
		{
			Stop();

			m_strFolderName = strFolder;
			Bootil::String::Util::TrimRight( m_strFolderName, &separator );
			Bootil::String::Util::FindAndReplace( m_strFolderName, BString( &separator ).append( &separator ), &separator );

			m_bWatchSubtree = bWatchSubtree;

#if defined( __APPLE__ ) && defined( __MACH__ )
			if (kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber10_7)
			{
				Output::Msg("ChangeMonitor requires OS X 10.7 or later.\n");
				return false;
			}

			CFStringRef dir = CFStringCreateWithCString( NULL, m_strFolderName.c_str(), kCFStringEncodingUTF8 );
			CFArrayRef paths = CFArrayCreate( NULL, ( const void** )&dir, 1, NULL );
			FSEventStreamContext ctx = { 0, this, NULL, NULL, NULL };
			m_fsStreamRef = FSEventStreamCreate( NULL, &ChangeMonitor::HandleFSEvent, &ctx, paths, kFSEventStreamEventIdSinceNow, 0.1, kFSEventStreamCreateFlagFileEvents );
			FSEventStreamScheduleWithRunLoop( m_fsStreamRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode );
			FSEventStreamStart( m_fsStreamRef );

			CFRelease( paths );
			CFRelease( dir );
#else

			m_dirHandles = new std::vector<WatcherData>;

			// Watch the main folder.

			WatcherData watcherData;

			watcherData.directory = m_strFolderName;
#ifdef _WIN32
			watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

			memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
			watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			std::fill( watcherData.buffer, watcherData.buffer + 1024, 0 );
#elif defined( __linux__ )
			int flags = IN_ATTRIB | IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO;
			watcherData.handle = inotify_add_watch( m_inotify, watcherData.directory.c_str(), flags );

			if ( watcherData.handle < 0 )
			{ return false; }
#endif

			( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );

			// And subfolders if we were told to.

			if ( m_bWatchSubtree )
			{
				Bootil::String::List directories;
				RecurseDirectories( directories, m_strFolderName );
				BOOTIL_FOREACH_CONST( directory, directories, Bootil::String::List )
				{
					WatcherData watcherData;

					watcherData.directory = *directory;
#ifdef _WIN32
					// Windows automatically watches all the subfolders except for symbolic links. We need to watch them manually. So watch this one only if it's a symbolic link.
					if ( !( GetFileAttributes( watcherData.directory.c_str() ) & FILE_ATTRIBUTE_REPARSE_POINT ) )
					{
						continue;
					}

					watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

					memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
					watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

					std::fill( watcherData.buffer, watcherData.buffer + 1024, 0 );
#elif defined( __linux__ )
					watcherData.handle = inotify_add_watch( m_inotify, watcherData.directory.c_str(), flags );

					if ( watcherData.handle < 0 )
					{ return false; }
#endif

					( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );
				}
			}
#endif
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
#if defined( __APPLE__ ) && defined( __MACH__ )
			if ( m_fsStreamRef )
			{
				FSEventStreamStop( m_fsStreamRef );
				FSEventStreamInvalidate( m_fsStreamRef );
				FSEventStreamRelease( m_fsStreamRef );
				m_fsStreamRef = NULL;
			}
#else
			if ( !m_dirHandles ) { return; }
			BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
			{
#ifdef _WIN32
				CloseHandle( watcherData->directoryHandle );
				watcherData->directoryHandle = NULL;
#elif defined( __linux__ )
				inotify_rm_watch( m_inotify, watcherData->handle );
#endif
			}

			delete ( std::vector<WatcherData>* ) m_dirHandles;
#endif
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
#if defined( __APPLE__ ) && defined( __MACH__ )
			CFRunLoopRunInMode( kCFRunLoopDefaultMode, 0.01, true );
#else
			if ( !m_dirHandles ) { return; }

#ifdef _WIN32
			BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
			{
				DWORD dwNumberBytes = 0;

				GetOverlappedResult( watcherData->directoryHandle, &watcherData->overlapped, &dwNumberBytes, FALSE );

				ResetEvent( watcherData->overlapped.hEvent );

				if ( dwNumberBytes > 0 )
				{
					int iOffset = 0;
					while ( true )
					{
						FILE_NOTIFY_INFORMATION* pNotify = ( FILE_NOTIFY_INFORMATION* ) &watcherData->buffer[iOffset];
						iOffset += pNotify->NextEntryOffset;

						if ( pNotify->FileNameLength > 0 )
						{
							// Found a changed file. Get the relative path to it.

							BString path;

							if ( watcherData->directory == m_strFolderName )
							{
								path = "";
							}
							else
							{
								path = watcherData->directory.substr( m_strFolderName.size() + 1 ) + separator;
							}

							path += Bootil::String::Convert::FromWide( WString( pNotify->FileName, pNotify->FileNameLength / sizeof( wchar_t ) ) );

							// Add it to the change list.

							NoteFileChanged( path );
						}

						if ( pNotify->NextEntryOffset == 0 )
						{
							StartWatch();
							break;
						}
					}

					// We reached the end of the changes. Start looking for them again.

					CancelIo( watcherData->directoryHandle );

					std::fill( watcherData->buffer, watcherData->buffer + 1024, 0 );

					ReadDirectoryChangesW( watcherData->directoryHandle, &watcherData->buffer, sizeof( watcherData->buffer ), m_bWatchSubtree, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &watcherData->overlapped, NULL );
				}
			}
#elif defined( __linux__ )
			timeval tv;
			tv.tv_sec = tv.tv_usec = 0;

			fd_set fds;
			FD_ZERO( &fds );
			FD_SET( m_inotify, &fds );

			select( m_inotify + 1, &fds, NULL, NULL, &tv );

			if ( FD_ISSET( m_inotify, &fds ) )
			{
				char buffer[1024];

				int i = 0, ret = read( m_inotify, buffer, 1024 );

				if ( ret < 0 )
				{ return; }

				while ( i < ret )
				{
					inotify_event* event = ( inotify_event* )&buffer[i];
					std::vector<WatcherData>& watches = *( std::vector<WatcherData>* ) m_dirHandles;
					WatcherFind find = {};
					find.handle = event->wd;
					std::vector<WatcherData>::const_iterator it = std::find_if( watches.begin(), watches.end(), find );

					if ( event->len > 0 && it != watches.end() )
					{
						// Found a changed file. Get the relative path to it.

						BString path;

						if ( it->directory == m_strFolderName )
						{
							path = "";
						}
						else
						{
							path = it->directory.substr( m_strFolderName.size() + 1 ) + separator;
						}

						path += event->name;

						NoteFileChanged( path );
					}

					i += sizeof( inotify_event ) + event->len;
				}
			}
#endif
#endif
		}

		void ChangeMonitor::StartWatch()
		{
#ifdef _WIN32
			BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
			{
				std::fill( watcherData->buffer, watcherData->buffer + 1024, 0 );

				ReadDirectoryChangesW( watcherData->directoryHandle, &watcherData->buffer, sizeof( watcherData->buffer ), m_bWatchSubtree, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &watcherData->overlapped, NULL );
			}
#endif

			CheckForChanges();
		}
	}
}
