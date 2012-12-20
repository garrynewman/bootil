#include "Bootil/Bootil.h"

namespace Bootil 
{
	//LONG WINAPI BootilCrashHandler( PEXCEPTION_POINTERS pExceptionInfo );

	static bool g_bShuttingDown = false;

	BOOTIL_EXPORT void Startup()
	{
		//Bootil::Threads::Internal::Initialize();

		//timeBeginPeriod( 1 );
		//SetUnhandledExceptionFilter( BootilCrashHandler );
	}

	BOOTIL_EXPORT void Shutdown()
	{
		g_bShuttingDown = true;
		Output::Msg( "Exiting..\n" );

		//Interface::UnloadAll();
		//File::Shutdown();

		Output::Msg( "Exited\n" );
	}

	BOOTIL_EXPORT bool IsShuttingDown()
	{
		return g_bShuttingDown;
	}
}