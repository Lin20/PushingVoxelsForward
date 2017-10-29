#include "Sampler.h"

#include <math.h>

const float Sampler_world_size = 256;

__forceinline void Sampler_get_intersection(vec3 v0, vec3 v1, float s0, float s1, float isolevel, vec3 out)
{
	float mu = (isolevel - s0) / (s1 - s0);
	vec3 delta_v;
	glm_vec_sub(v1, v0, delta_v);
	delta_v[0] *= mu;
	delta_v[1] *= mu;
	delta_v[2] *= mu;
	glm_vec_add(delta_v, v0, out);
}

__forceinline float SurfaceFn_sphere(float x, float y, float z, float w, struct osn_context* osn_context)
{
	x += w;
	const float r = Sampler_world_size * 0.45f;
	return x * x + y * y + z * z - r * r;
}

__forceinline float SurfaceD_sphere(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float r = Sampler_world_size * 0.45f;
	return sqrtf(x * x + y * y + z * z) - r;
}


__forceinline float SurfaceD_torus_z(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float r1 = (float)Sampler_world_size / 4.0f;
	const float r2 = (float)Sampler_world_size / 10.0f;
	float q_x = fabsf(sqrtf(x * x + y * y)) - r1;
	float len = sqrtf(q_x * q_x + z * z);
	return len - r2;
}

float SurfaceD_plane(float x, float y, float z, float w, struct osn_context* osn_context)
{
	return -z + 0.01f;
}

float SurfaceFn_Klein_bottle(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float m = 8.0f / Sampler_world_size;
	x *= m;
	y *= m;
	z *= m;
	float a = (x*x + y*y + z*z + 2.0f * y - 1.0f);
	float a_2 = (x*x + y*y + z*z - 2.0f * y - 1.0f);
	float b = (a_2 * a_2 - 8.0f * z * z);
	float c = 16.0f * x * z * a_2;
	return a * b + c;
}

float SurfaceFn_2d_terrain(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float scale = 0.025f;
	return y - open_simplex_noise2(osn_context, x * scale + w, z * scale) / 4.0f * Sampler_world_size;
}

float SurfaceFn_3d_terrain(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float scale = 0.025f;
	return y - open_simplex_noise3(osn_context, x * scale + w, y * scale, z * scale) * 0.5f * Sampler_world_size;
}

float SurfaceFn_sphere_r(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float scale = 0.15f;
	float r = Sampler_world_size * 0.8f;
	r += open_simplex_noise3(osn_context, x * scale + w, y * scale, z * scale) * Sampler_world_size * 4.0f;
	return x * x + y * y + z * z - r;
}

__forceinline float SurfaceFn_torus_r(float x, float y, float z, float w, struct osn_context* osn_context)
{
	const float scale = 0.15f;
	const float r1 = (float)Sampler_world_size / 4.0f + open_simplex_noise3(osn_context, x * scale + w, y * scale, z * scale) * 4.0f;
	const float r2 = (float)Sampler_world_size / 10.0f;
	float q_x = fabsf(sqrtf(x * x + y * y)) - r1;
	float len = sqrtf(q_x * q_x + z * z);
	return len - r2;
}

