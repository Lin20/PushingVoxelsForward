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

inline const int vec3_compare(const vec3 a, const vec3 b)
{
	return !(a[0] == b[0] && a[1] == b[1] && a[2] == b[2]);
}

inline uint64_t vec3_hash(vec3 v)
{
	const float p1 = 73856093.0f;
	const float p2 = 19349663.0f;
	const float p3 = 83492791.0f;

	float a = v[0] * p1;
	float b = v[1] * p2;
	float c = v[2] * p3;

	int64_t i = *((int*)&a);
	int64_t j = *((int*)&b);
	int64_t k = *((int*)&c);

	return i ^ j ^ k;
}

inline void vec3_midpoint(vec3 dest, vec3 a, vec3 b)
{
	vec3_set(dest, (a[0] + b[0]) * 0.5f, (a[1] + b[1]) * 0.5f, (a[2] + b[2]) * 0.5f);
}

inline void vec3_add_coeff(vec3 dest, vec3 a, vec3 b, float a_coeff)
{
	dest[0] = a[0] * a_coeff + b[0];
	dest[1] = a[1] * a_coeff + b[1];
	dest[2] = a[2] * a_coeff + b[2];
}