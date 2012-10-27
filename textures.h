// TX stands for textures. Short and cool.

namespace TX
{
	enum { PerroFrames = 0, Ruby, Ground, MAX };

	struct sprite
	{
		sizei size;
		pointi origin;
		sizei tileSize;
	};

	bool load(int id, const char *filename, pointi origin = pointi(), sizei tileSize = sizei(), bool repeat = false);

	extern sprite sprites[TX::MAX];
}
