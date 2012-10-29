// GFX stands for graphics. It sounds cool that way.

namespace GFX
{
	struct RGBAf
	{
		float R, G, B, A;

		RGBAf(float r, float g, float b, float a)
		{
			R = r;
			G = g;
			B = b;
			A = a;
		}
	};

	bool init(const char *title, sizei resolution, bool fullscreen = false);
	void setResolution(sizei resolution);
	void terminate();
	void loadTexture(int id, int width, int height, void *bits, bool repeat);
	void unloadTexture(int id);
	void drawSprite(int id, float x, float y, float angle = 0, float size = 1.0f);
	void drawTiledSprite(int id, int tileIndex, float x, float y, float angle = 0, float size = 1.0f, bool flipX = false, bool flipY = false);
	void drawGradient(float x, float y, float width, float height, RGBAf startColor, RGBAf endColor, bool isHorizontal = false);
	void renderObjects();
}
