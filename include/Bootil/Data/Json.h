#pragma once

namespace Bootil
{
	namespace Data
	{
		namespace Json
		{
			bool Export( const Bootil::Data::Tree & tree, Bootil::BString & output, bool bPretty = false );
			bool Import( Bootil::Data::Tree & tree, const Bootil::BString & input );
		}
	}
}
