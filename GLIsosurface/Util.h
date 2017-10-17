#pragma once

#include <cglm\cglm.h>

#define PI 3.1415926535897932384626433832795f
#define PI_D 3.1415926535897932384626433832795

#define BOOL_TO_STRING(x) ((x) ? "true" : "false")

inline void vec3_set(vec3 v, float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

inline void vec3_copy(vec3 from, vec3 to)
{
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
}

inline void vec3_zero(vec3 dest)
{
	dest[0] = 0;
	dest[1] = 0;
	dest[2] = 0;
}

inline void vec3_right(vec3 dest)
{
	dest[0] = 1;
	dest[1] = 0;
	dest[2] = 0;
}

inline void vec3_up(vec3 dest)
{
	dest[0] = 0;
	dest[1] = 1;
	dest[2] = 0;
}

inline void vec3_forward(vec3 dest)
{
	dest[0] = 0;
	dest[1] = 0;
	dest[2] = 1;
}

inline void vec3_negate(vec3 dest)
{
	dest[0] = -dest[0];
	dest[1] = -dest[1];
	dest[2] = -dest[2];
}

inline float vec3_distance2(vec3 a, vec3 b)
{
	float x, y, z;
	x = b[0] - a[0];
	y = b[1] - a[1];
	z = b[2] - a[2];
	return x*x + y*y + z*z;
}
