#pragma once

#include <cglm\cglm.h>
#include "OpenSimplexNoise.h"

// Provides a bunch of different functions representing difference surfaces.
// Fn means it provides a raw scalar.
// D means it provides an actual distance distance value.

extern __forceinline void Sampler_get_intersection(vec3 v0, vec3 v1, float s0, float s1, float isolevel, vec3 out);
extern __forceinline float SurfaceFn_sphere(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_sphere_sliced(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceD_sphere(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceD_torus_z(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceD_plane(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_Klein_bottle(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_2d_terrain(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_3d_terrain(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_sphere_r(float x, float y, float z, float w, struct osn_context* osn_context);
extern __forceinline float SurfaceFn_torus_r(float x, float y, float z, float w, struct osn_context* osn_context);
