// TX stands for textures. Short and cool.

namespace TX
{
	enum { PerroFrames = 0, Ruby, Ground, MAX };

	enum Shadows
	{
		kShadowNone = 0,
		kShadowL,
		kShadowT,
		kShadowTL,
		kShadowLR,
		kShadowTB,
		kShadowTLR,
		kShadowTLB,
		kShadowTLBR,
		kShadowTl,
		kShadowTlR,
		kShadowTlB,
		kShadowTlRB,
		kShadowTlBr,
		kShadowTlTr,
		kShadowTlBl,
		kShadowTlTrB,
		kShadowTlBlR,
		kShadowTlBlTr,
		kShadowTlBlTrBr
	};

	struct sprite
	{
		sizei size;
		pointi origin;
		sizei tileSize;
	};

	bool load(int id, const char *filename, pointi origin = pointi(), sizei tileSize = sizei(), bool repeat = false);

	extern sprite sprites[TX::MAX];
}
