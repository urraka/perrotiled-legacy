#include <stdlib.h>
#include "inc/FreeImage.h"
#include "elementals.h"
#include "map.h"

namespace MAP
{
	mapinfo mapData;
	
	bool load(const char *filename)
	{
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);

		if (fif == FIF_UNKNOWN)
		{
			fif = FreeImage_GetFIFFromFilename(filename);
		}

		if (fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif))
		{
			return false;
		}

		FIBITMAP *dib = FreeImage_Load(fif, filename);

		if (!dib)
		{
			return false;
		}

		mapData.width = FreeImage_GetWidth(dib);
		mapData.height = FreeImage_GetHeight(dib);
		i32 bpp = FreeImage_GetLine(dib) / mapData.width;

		mapData.data = new ui8[mapData.width * mapData.height];

		for (ui32 y = 0; y < mapData.height; y++)
		{
			ui8 *bits = FreeImage_GetScanLine(dib, mapData.height - y - 1);

			for (ui32 x = 0; x < mapData.width; x++)
			{
				ui32 color = (bits[FI_RGBA_RED] << 16) | (bits[FI_RGBA_GREEN] << 8) | (bits[FI_RGBA_BLUE]);
				mapData.data[mapData.width * y + x] = (color == 0 ? 1 : 0);
				bits += bpp;
			}
		}

		FreeImage_Unload(dib);

		return true;
	}

	void unload()
	{
		if (mapData.data) delete[] mapData.data;
	}
}
