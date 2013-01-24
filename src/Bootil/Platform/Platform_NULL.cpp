// foundation.cpp : Defines the entry point for the DLL application.
//

#include "Bootil/Bootil.h"

#if !defined(_WIN32) && !defined(__linux__)

namespace Bootil 
{
	namespace Platform
	{
		BOOTIL_EXPORT BString LastError()
		{
			return "<UNKNOWN>";
		}

		BString FullProgramName( void )
		{
			return "<UNKNOWN>";
		}

		BString ProgramName( void )
		{
			return "<UNKNOWN>";
		}

		BString ProgramFolder( void )
		{
			BString strFolder = FullProgramName();
			String::File::StripFilename( strFolder );
			return strFolder;
		}

		BString CurrentUserName( void )
		{
			return "<UNKNOWN>";
		}

		BOOTIL_EXPORT void SetupAssociation( BString ext )
		{
			
		}

		BOOTIL_EXPORT unsigned int DesktopWidth( void )
		{
			return 1024;
		}

		BOOTIL_EXPORT unsigned int DesktopHeight( void )
		{
			return 768;
		}

		BOOTIL_EXPORT bool Popup( const BString& strName, const BString& strText )
		{
			return true;
		}

		BOOTIL_EXPORT void DebuggerOutput( const BString& strText )
		{
			printf( "%s", strText.c_str() );
		}

		BOOTIL_EXPORT int FindFiles( String::List* files, String::List* folders, const BString& strFind, bool bUpUpFolders )
		{
			return 0;
		}

		BOOTIL_EXPORT void OpenWebpage( const BString& strURL )
		{
			
		}

		BOOTIL_EXPORT BString TemporaryFile( void )
		{
			return "";
		}
	}
}

#endif