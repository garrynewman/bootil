#pragma once

#include "Bootil/Bootil.h"

#if defined( __APPLE__ ) && defined( __MACH__ )
#include <MacTypes.h>
struct __FSEventStream; // Forward declaration
#endif

namespace Bootil
{
	namespace File
	{
		/*

		- Declare

		ChangeMonitor mychangemonitor;


		- Set up

		mychangemonitor.WatchFolder( "blah/blah/", true );


		- In a loop somewhere safe

		if ( mychangemonitor.HasChanges() )
		{
			BString strFileChanged = GetChange();

			- do something here about strFileChanged being changed
		}

		*/

		class BOOTIL_EXPORT ChangeMonitor
		{
			public:

				ChangeMonitor();
				~ChangeMonitor();

				bool WatchFolder( const BString & strFolder, bool bWatchSubtree = false );
				void Stop();

				bool HasChanges();
				BString GetChange();

				const BString & FolderName() { return m_strFolderName; }

			private:

				void NoteFileChanged( const BString & strName );
				void CheckForChanges();
				void StartWatch();

#if defined( __APPLE__ ) && defined( __MACH__ )
				static void HandleFSEvent( const __FSEventStream *stream, void *ctx, size_t numEvents, void *eventPaths, const UInt32 evFlags[], const UInt64 evIds[] );
			
				__FSEventStream*	m_fsStreamRef;
#elif defined(__linux__)
				int					m_inotify;
#endif
				void* 				m_dirHandles;
				BString				m_strFolderName;
				std::list<BString>	m_Changes;
				bool				m_bWatchSubtree;
		};
	}
}