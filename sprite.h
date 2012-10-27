// Stores information about a single image and does the loading job.

struct sprite
{
	Sizei size;
	Pointi origin;

	static bool load(int id, const char *filename, Pointi origin);
	static sprite sprites[TX::MAX];
};
