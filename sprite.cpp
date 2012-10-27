#include <stdlib.h>
#include "inc/FreeImage.h"
#include "elementals.h"
#include "textures.h"
#include "opengl.h"
#include "sprite.h"

bool sprite::load(int id, const char *filename, Pointi origin)
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

	FIBITMAP *dib, *tmp = FreeImage_Load(fif, filename);

	if (!tmp)
	{
		return false;
	}

	dib = FreeImage_ConvertTo32Bits(tmp);
	FreeImage_Unload(tmp);

	if (!dib)
	{
		return false;
	}

	int width = FreeImage_GetWidth(dib);
	int height = FreeImage_GetHeight(dib);
	int pitch = FreeImage_GetPitch(dib);
	BYTE *bits = (BYTE*)malloc(height * pitch);

	if (!bits)
	{
		FreeImage_Unload(dib);
		return false;
	}

	FreeImage_ConvertToRawBits(bits, dib, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Unload(dib);

	GFX::loadTexture(id, width, height, bits);
	free(bits);

	sprites[id].size.x = width;
	sprites[id].size.y = height;
	sprites[id].origin = origin;

	return true;
}
