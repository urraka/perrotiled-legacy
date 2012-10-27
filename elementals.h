// Useful stuff that will be needed across the game

#include <math.h>

typedef signed __int8 i8;
typedef unsigned __int8 ui8;
typedef signed __int16 i16;
typedef unsigned __int16 ui16;
typedef signed __int32 i32;
typedef unsigned __int32 ui32;
typedef signed __int64 i64;
typedef unsigned __int64 ui64;
typedef float f32;
typedef double f64;

inline f32 round(f32 r)
{
    return (r > 0.0f) ? floor(r + 0.5f) : ceil(r - 0.5f);
}

inline f64 round(f64 r)
{
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

template<typename T, typename U> class point2d_t
{
public:
	T x, y;
	point2d_t() : x(0), y(0) { }
	point2d_t(T _x, T _y) : x(_x), y(_y) { }

	operator point2d_t<U, T>()
	{
		return point2d_t<U, T>((U)x, (U)y);
	}

	/*operator bool() const
	{
		return x || y;
	}*/
};

template<typename T> class size2d_t
{
public:
	T width, height;
	size2d_t() : width(0), height(0) { }
	size2d_t(T _width, T _height) : width(_width), height(_height) { }
};

template<typename T, typename U> class rect2d_t
{
public:
	T x, y;
	T width, height;

	rect2d_t() :  x(0), y(0), width(0), height(0) { }
	rect2d_t(T _x, T _y, T _width, T _height) : x(_x), y(_y), width(_width), height(_height) { }

	bool intersects(const rect2d_t<T, U> &rc) const
	{
		if (x < rc.x + rc.width && x + width > rc.x && y < rc.y + rc.height && y + height > rc.y)
		{
			return true;
		}

		return false;
	}

	rect2d_t<T, U> &add(const rect2d_t<T, U> &rc)
	{
		T x2 = x + width, y2 = y + height;

		if (rc.x + rc.width > x2) x2 = rc.x + rc.width;
		if (rc.y + rc.height > y2) y2 = rc.y + rc.height;
		if (rc.x < x) x = rc.x;
		if (rc.y < y) y = rc.y;

		width = x2 - x;
		height = y2 - y;

		return *this;
	}

	operator rect2d_t<U, T>()
	{
		return rect2d_t<U, T>((U)x, (U)y, (U)width, (U)height);
	}
};

typedef point2d_t<f32, i32> pointf;
typedef point2d_t<f32, i32> vectorf;
typedef size2d_t<f32> sizef;
typedef rect2d_t<f32, i32> rectf;

typedef point2d_t<i32, f32> pointi;
typedef point2d_t<i32, f32> vectori;
typedef size2d_t<i32> sizei;
typedef rect2d_t<i32, f32> recti;
