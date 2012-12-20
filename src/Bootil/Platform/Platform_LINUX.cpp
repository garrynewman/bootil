
#ifdef __linux__

//#define X11_GRAPHICAL // Don't use X11 code on a server, that's just stupid

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Bootil/Bootil.h"
#include <libgen.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <cstdlib>

#ifdef X11_GRAPHICAL
#include <X11/Xlib.h>
#endif

extern char *program_invocation_name;
extern char *program_invocation_short_name;

int wildcmp(const char *wild, const char *string) {
  // Written by Jack Handy - jakkhandy@hotmail.com
  const char *cp = NULL, *mp = NULL;

  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }

  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string+1;
    } else if ((*wild == *string) || (*wild == '?')) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }

  while (*wild == '*') {
    wild++;
  }
  return !*wild;
}

namespace Bootil 
{
	namespace Platform
	{
		BOOTIL_EXPORT BString LastError()
		{
			return strerror( errno );
		}

		BString FullProgramName( void )
		{
			return program_invocation_name;
		}

		BString ProgramName( void )
		{
			return program_invocation_short_name;
		}

		BString ProgramFolder( void )
		{
			BString strFolder = FullProgramName();
			String::File::StripFilename( strFolder );
			return strFolder;
		}

		BString CurrentUserName( void )
		{
			passwd *pw;
			uid_t uid;

			uid = geteuid();
			pw = getpwuid( uid );

			if ( pw )
			{
				return pw->pw_name;
			}
			else
				return "<UNKNOWN>";
		}

		BOOTIL_EXPORT void SetupAssociation( BString ext )
		{
			
		}

		BOOTIL_EXPORT unsigned int DesktopWidth( void )
		{
			#ifdef X11_GRAPHICAL
			Display* display = XOpenDisplay( NULL );
			int screen = DefaultScreen( display );

			int width = XDisplayWidth( display, screen );

			XCloseDisplay( display );

			return width;
			#else
			return 800;
			#endif
		}

		BOOTIL_EXPORT unsigned int DesktopHeight( void )
		{
			#ifdef X11_GRAPHICAL
			Display* display = XOpenDisplay( NULL );
			int screen = DefaultScreen( display );

			int height = XDisplayHeight( display, screen );

			XCloseDisplay( display );

			return height;
			#else
			return 600;
			#endif
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
			BString dirName = strdup( strFind.c_str() );
			dirName = dirname( (char*)dirName.c_str() );

			BString findName = strdup( strFind.c_str() );
			findName = basename( (char*)findName.c_str() );

			DIR *dp;
			dirent *dirp;
			int iFiles = 0;

			if ( (dp = opendir(dirName.c_str())) == NULL )
				return 0;

			while ( (dirp = readdir(dp)) != NULL )
			{
				BString name(dirp->d_name);
				BString fullName = dirName + "/" + name;

				if ( wildcmp(findName.c_str(), name.c_str()) )
				{
					if ( Bootil::File::IsFolder(fullName) && folders )
					{
						if ( bUpUpFolders || (name != "." && name != "..") )
						{
							folders->push_back( fullName );
							iFiles++;
						}
					}
					else if ( files )
					{
						files->push_back( fullName );
						iFiles++;
					}
				}
			}

			closedir(dp);
			return iFiles;
		}

		BOOTIL_EXPORT void OpenWebpage( const BString& strURL )
		{
			pid_t pid;
			const char *args[2];

			args[0] = strURL.c_str();
			args[1] = NULL;

			pid = fork();
			
			if( pid == 0 )
				execvp( "/usr/bin/xdg-open", (char*const*)args );
		}

		BOOTIL_EXPORT void StartProcess( const BString& strProcess, bool AndWait )
		{
			pid_t pid = fork();

			if ( pid != 0 )
			{
				bool isOk = (pid > 0);
				if ( isOk && AndWait )
					wait();
			}
			else
			{
				const char* args[2];
				args[0] = strProcess.c_str();
				args[1] = NULL;

				execvp( strProcess.c_str(), (char*const*)args );
			}
		}
	}
}

#endif