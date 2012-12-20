
#include "Bootil/Bootil.h"

namespace Bootil 
{
	namespace CommandLine
	{
		String::List	g_ArgList;

		void AddArgument( const BString& str )
		{
			g_ArgList.push_back( str );
		}

		BOOTIL_EXPORT void Set( int argc, char *argv[] )
		{
			for (int i=1; i<argc; i++ )
			{
				AddArgument( argv[i] );
			}
		}

		BOOTIL_EXPORT int GetArgCount()
		{
			return g_ArgList.size();
		}

		BOOTIL_EXPORT BString GetArg( int iNum, const BString& strDefault )
		{
			if ( iNum < 0 || iNum >= GetArgCount() ) return strDefault;

			return g_ArgList[ iNum ];
		}
	}

}