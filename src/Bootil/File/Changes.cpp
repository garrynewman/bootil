
#include "Bootil/Bootil.h"
#ifdef _WIN32
#define _WIN32_WINNT 0x0502

#include <windows.h>
#elif defined( __APPLE__ ) && defined( __MACH__ )
#include <sys/param.h>
#include <sys/event.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
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
#elif defined( __APPLE__ ) && defined( __MACH__ )
		struct kevent event;

		int handle;
#elif defined( __linux__ )
		int handle;
#endif
	};

#if ( defined( __APPLE__ ) && defined( __MACH__ ) ) || defined( __linux__ )
	struct WatcherFind
	{
		int handle;

		bool operator()(const WatcherData& b) { return handle == b.handle; }
	};
#endif

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
}

namespace Bootil
{
	namespace File
	{
		ChangeMonitor::ChangeMonitor()
		{
			m_dirHandles = NULL;
#if defined( __APPLE__ ) && defined( __MACH__ )
			m_inotify = kqueue();

			struct rlimit rl;
			if ( !getrlimit( RLIMIT_NOFILE, &rl ) )
			{
				rl.rlim_cur = MIN( rl.rlim_max, OPEN_MAX );
				setrlimit( RLIMIT_NOFILE, &rl );
			}
#elif defined( __linux__ )
			m_inotify = inotify_init();
#endif
		}

		ChangeMonitor::~ChangeMonitor()
		{
			Stop();
#if ( defined( __APPLE__ ) && defined( __MACH__ ) ) || defined( __linux__ )
			close( m_inotify );
#endif
		}

		bool ChangeMonitor::WatchFolder( const BString & strFolder, bool bWatchSubtree )
		{
			Stop();

			m_strFolderName = strFolder;
			Bootil::String::Util::TrimRight( m_strFolderName, &separator );
			Bootil::String::Util::FindAndReplace( m_strFolderName, BString( &separator ).append( &separator ), &separator );

			m_bWatchSubtree = bWatchSubtree;

			m_dirHandles = new std::vector<WatcherData>;

			// Watch the main folder.

			WatcherData watcherData;

			watcherData.directory = m_strFolderName;
#ifdef _WIN32
			watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

			memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
			watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			std::fill( watcherData.buffer, watcherData.buffer + 1024, 0 );
#elif defined( __APPLE__ ) && defined( __MACH__ )
			if ( m_inotify == -1 )
			{ return false; }

			struct timespec timeout = { 0, 0 };

			watcherData.handle = open( watcherData.directory.c_str(), O_EVTONLY );
			if ( watcherData.handle == -1 )
			{ return false; }

			EV_SET( &watcherData.event, watcherData.handle, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_RENAME | NOTE_ATTRIB, 0, NULL );
			kevent( m_inotify, &watcherData.event, 1, NULL, 0, &timeout );

			Bootil::String::List files;
			Find( &files, &files, watcherData.directory + separator + "*", false );
			BOOTIL_FOREACH_CONST( file, files, Bootil::String::List )
			{
				WatcherData watcherData;

				watcherData.directory = m_strFolderName + separator + *file;
				watcherData.handle = open( watcherData.directory.c_str(), O_EVTONLY );
				if ( watcherData.handle == -1 )
				{ return false; }

				EV_SET( &watcherData.event, watcherData.handle, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_RENAME | NOTE_ATTRIB, 0, NULL );
				kevent( m_inotify, &watcherData.event, 1, NULL, 0, &timeout );

				( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );
			}
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
					watcherData.directoryHandle = CreateFileA( watcherData.directory.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL );

					// Windows automatically watches all the subfolders except for symbolic links. We need to watch them manually. So watch this one only if it's a symbolic link.

					if ( !( GetFileAttributes( watcherData.directory.c_str() ) & FILE_ATTRIBUTE_REPARSE_POINT ) )
					{
						continue;
					}

					memset( &watcherData.overlapped, 0, sizeof( watcherData.overlapped ) );
					watcherData.overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

					std::fill( watcherData.buffer, watcherData.buffer + 1024, 0 );
#elif defined( __APPLE__ ) && defined( __MACH__ )
					watcherData.handle = open( watcherData.directory.c_str(), O_EVTONLY );
					if ( watcherData.handle == -1 )
					{ return false; }

					EV_SET( &watcherData.event, watcherData.handle, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_RENAME | NOTE_ATTRIB, 0, NULL );
					kevent( m_inotify, &watcherData.event, 1, NULL, 0, &timeout );

					Bootil::String::List files;
					Find( &files, &files, watcherData.directory + separator + "*", false );
					BOOTIL_FOREACH_CONST( file, files, Bootil::String::List )
					{
						WatcherData watcherData;

						watcherData.directory = *directory + separator + *file;
						watcherData.handle = open( watcherData.directory.c_str(), O_EVTONLY );
						if ( watcherData.handle == -1 )
						{ return false; }

						EV_SET( &watcherData.event, watcherData.handle, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_RENAME | NOTE_ATTRIB, 0, NULL );
						kevent( m_inotify, &watcherData.event, 1, NULL, 0, &timeout );

						( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );
					}
#elif defined( __linux__ )
					watcherData.handle = inotify_add_watch( m_inotify, watcherData.directory.c_str(), flags );

					if ( watcherData.handle < 0 )
					{ return false; }
#endif

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
			if ( !m_dirHandles ) { return; }
			BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
			{
#ifdef _WIN32
				CloseHandle( watcherData->directoryHandle );
				watcherData->directoryHandle = NULL;
#elif defined( __APPLE__ ) && defined( __MACH__ )
				close( watcherData->handle );
#elif defined( __linux__ )
				inotify_rm_watch( m_inotify, watcherData->handle );
#endif
			}

			delete ( std::vector<WatcherData>* ) m_dirHandles;
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
							break;
						}
					}

					// We reached the end of the changes. Start looking for them again.

					CancelIo( watcherData->directoryHandle );

					std::fill( watcherData->buffer, watcherData->buffer + 1024, 0 );

					ReadDirectoryChangesW( watcherData->directoryHandle, &watcherData->buffer, sizeof( watcherData->buffer ), m_bWatchSubtree, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &watcherData->overlapped, NULL );
				}
			}
#elif defined( __APPLE__ ) && defined( __MACH__ )
			std::vector<WatcherData>& watches = *( std::vector<WatcherData>* ) m_dirHandles;

			struct timespec timeout = { 0, 5000 }; // Should be enough to catch simulataneous events
			struct kevent received;
			while ( true )
			{
				if ( kevent( m_inotify, NULL, 0, &received, 1, &timeout ) <= 0 )
				{
					break;
				}

				WatcherFind find = {};
				find.handle = (int)received.ident;
				std::vector<WatcherData>::const_iterator it = std::find_if( watches.begin(), watches.end(), find );
				if ( it != watches.end() )
				{
					BString path = it->directory;
					if ( !Exists( path ) )
					{
						close( it->handle );
						( ( std::vector<WatcherData>* ) m_dirHandles )->erase( it );
						NoteFileChanged( path );
					}
					else if ( IsFolder( path ) )
					{
						std::set<BString> dirHandles;
						BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
						{
							std::vector<char> tmp( watcherData->directory.c_str(), watcherData->directory.c_str() + watcherData->directory.size() + 1 );
							BString directory( dirname( &tmp[0] ) );
							if ( directory == path )
							{
								dirHandles.insert( watcherData->directory );
							}
						}
						Bootil::String::List fileList;
						Find( &fileList, &fileList, path + separator + "*", false );
						BOOTIL_FOREACH( file, fileList, Bootil::String::List)
						{
							*file = path + separator + *file;
						}

						std::set<BString> files( fileList.begin(), fileList.end() );
						std::set<BString> changedFiles;
						std::set_symmetric_difference( files.begin(), files.end(), dirHandles.begin(), dirHandles.end(), std::inserter( changedFiles, changedFiles.begin() ) );

						struct timespec timeout2 = { 0, 0 };

						BOOTIL_FOREACH_CONST( file, changedFiles, std::set<BString> )
						{
							if ( Exists( *file ) )
							{
								WatcherData watcherData;

								watcherData.directory = *file;

								watcherData.handle = open( file->c_str(), O_EVTONLY );
								if ( watcherData.handle != -1 )
								{
									EV_SET( &watcherData.event, watcherData.handle, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_EXTEND | NOTE_WRITE | NOTE_RENAME | NOTE_ATTRIB, 0, NULL );
									kevent( m_inotify, &watcherData.event, 1, NULL, 0, &timeout2 );

									( ( std::vector<WatcherData>* ) m_dirHandles )->push_back( watcherData );

									NoteFileChanged( *file );
								}
							}
							else
							{
								BOOTIL_FOREACH( watcherData, ( *( std::vector<WatcherData>* ) m_dirHandles ), std::vector<WatcherData> )
								{
									if ( watcherData->directory == *file )
									{
										close( watcherData->handle );
										( ( std::vector<WatcherData>* ) m_dirHandles )->erase( it );

										NoteFileChanged( *file );
										break;
									}
								}
							}
						}
					}
					else
					{
						NoteFileChanged( path );
					}
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
						NoteFileChanged( it->directory + separator + event->name );
					}

					i += sizeof( inotify_event ) + event->len;
				}
			}
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
