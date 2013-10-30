
#if defined(__APPLE__) && defined( __MACH__ )

#include "Bootil/Bootil.h"

#include <stdio.h>
#include <unistd.h>


namespace Bootil
{
	namespace Platform
	{
		BOOTIL_EXPORT BString LastError()
		{
			return "todo";
		}

		BOOTIL_EXPORT BString FullProgramName( void )
		{
			return "todo";
		}

		BOOTIL_EXPORT BString ProgramName( void )
		{
			return "todo";
		}

		BOOTIL_EXPORT BString ProgramFolder( void )
		{
			return "todo";
		}

		BOOTIL_EXPORT BString CurrentUserName( void )
		{
			// todo.
			return "unknown";
		}

		BOOTIL_EXPORT void SetupAssociation( BString ext )
		{
			// todo.
		}

		BOOTIL_EXPORT unsigned int DesktopWidth( void )
		{
			return 800;
		}

		BOOTIL_EXPORT unsigned int DesktopHeight( void )
		{
			return 600;
		}

		BOOTIL_EXPORT bool Popup( const BString & strName, const BString & strText )
		{
			return true;
		}

		BOOTIL_EXPORT void DebuggerOutput( const BString & strText )
		{
			printf( "%s", strText.c_str() );
		}

		BOOTIL_EXPORT int FindFiles( String::List* files, String::List* folders, const BString & strFind, bool bUpUpFolders )
		{
			// todo.
			return 0;
		}

		BOOTIL_EXPORT void OpenWebpage( const BString & strURL )
		{
			// todo.
		}

		BOOTIL_EXPORT void StartProcess( const BString & strProcess, bool AndWait )
		{
			// todo.
		}

		BOOTIL_EXPORT BString TemporaryDir( void )
		{
			return "/tmp";
		}

		BOOTIL_EXPORT BString TemporaryFilename( void )
		{
			char buffer[L_tmpnam];
			return Bootil::String::File::GetFixSlashes( TemporaryDir() + tmpnam( buffer ) );
		}

		BOOTIL_EXPORT void Sleep( unsigned int ms )
		{
			::usleep( ms * 1000 );
		}

		BOOTIL_EXPORT BString CurrentDir( void )
		{
			char strCurrentWD[4096];
			getcwd( strCurrentWD, sizeof( strCurrentWD ) );

			return strCurrentWD;
		}
	}
}

#endif