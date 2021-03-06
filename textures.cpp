#include <stdlib.h>
#include "inc/FreeImage.h"
#include "elementals.h"
#include "textures.h"
#include "opengl.h"

namespace TX
{
	sprite sprites[TX::MAX];

	bool load(int id, const char *filename, pointi origin, sizei tileSize, bool repeat)
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

		GFX::loadTexture(id, width, height, bits, repeat);
		free(bits);

		sprites[id].size.width = width;
		sprites[id].size.height = height;
		sprites[id].origin = origin;

		if (tileSize.width && tileSize.height)
		{
			sprites[id].tileSize.width = tileSize.width;
			sprites[id].tileSize.height = tileSize.height;
		}

		return true;
	}

	void saveImage(const char *filename, int width, int height, ui8 *data)
	{
		FIBITMAP *image = FreeImage_Allocate(width, height, 24);

		if (image)
		{
			RGBQUAD color;

			for (int x = 0; x < width; x++)
			{
				for (int y = 0; y < height; y++)
				{
					int i = (y * width + x) * 3;

					color.rgbRed = data[i + 0];
					color.rgbGreen = data[i + 1];
					color.rgbBlue = data[i + 2];

					FreeImage_SetPixelColor(image, x, y, &color);
				}
			}

			FreeImage_Save(FIF_PNG, image, filename);
			FreeImage_Unload(image);
		}
	}
}