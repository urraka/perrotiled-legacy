namespace MAP
{
	struct mapinfo
	{
		ui8 *data;
		ui32 width;
		ui32 height;
		ui32 tileSize;

		mapinfo() : data(0), width(0), height(0), tileSize(32) {}
	};

	extern mapinfo mapData;

	bool load(const char *filename);
	void unload();
	inline int getWidth() { return mapData.width; };
	inline int getHeight() { return mapData.height; };
	inline int getTileSize() { return mapData.tileSize; };
	inline int getTile(ui32 x, ui32 y) { return mapData.data[mapData.width * y + x]; };

	inline bool collides(const recti &rc)
	{
		pointi endpoint(
			(rc.x + rc.width) / mapData.tileSize,
			(rc.y + rc.height) / mapData.tileSize
		);

		for (i32 y = rc.y / mapData.tileSize; y <= endpoint.y; y++)
		{
			for (i32 x = rc.x / mapData.tileSize; x <= endpoint.x; x++)
			{
				if (mapData.data[mapData.width * y + x] && rc.intersects(recti(x * mapData.tileSize, y * mapData.tileSize, mapData.tileSize, mapData.tileSize)))
				{
					return true;
				}
			}
		}

		return false;
	}
}
