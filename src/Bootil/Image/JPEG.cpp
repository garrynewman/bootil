
#include "Bootil/Bootil.h"

#pragma once

#define STBI_NO_STDIO
#include "stb_image/stb_image.h"

namespace Bootil
{
	namespace Image
	{
		namespace JPEG
		{
			BOOTIL_EXPORT bool Load( Bootil::Buffer & buf, Bootil::Image::Format & imageout )
			{
				stbi s;
				start_mem( &s, ( const uint8* ) buf.GetBase(), buf.GetWritten() );

				if ( !stbi_jpeg_test( &s ) )
				{ return false; }

				int x, y, comp;
				unsigned char* pData = stbi_jpeg_load( &s, &x, &y, &comp, 3 );
				imageout.width	= x;
				imageout.height = y;
				imageout.alpha	= false;
				imageout.data.Write( pData, x * y * 3 );
				stbi_image_free( pData );
				return true;
			}
		}
	}

}