#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <cglm\cglm.h>
#include <stdint.h>

#include "OpenSimplexNoise.h"

struct UMC_Isovertex
{
	uint32_t index;
	float value;
	vec3 position;
	vec3 normal;
};

struct UMC_Chunk
{
	int indexed_primitives : 1;
	int pem : 1;
	int initialized : 1;
	float timer;

	GLuint vao;
	GLuint v_vbo;
	GLuint n_vbo; 
	GLuint ibo;

	uint32_t dim;
	uint32_t v_count;
	uint32_t p_count;
	uint32_t snapped_count;
	uint32_t vbo_size;
	uint32_t ibo_size;

	uint16_t* grid_signs;
	struct UMC_Isovertex* grid_verts;
	struct UMC_Edge* edges;
	uint32_t* edge_v_indexes;
	struct osn_context* osn;
};

struct UMC_Edge
{
	uint32_t grid_v0;
	uint32_t grid_v1;
	struct UMC_Isovertex iso_vertex;
};

struct UMC_Cell
{
	uint16_t mask;
	uint32_t* iso_verts[20];
};

void UMC_Chunk_init(struct UMC_Chunk* dest, uint32_t dim, int index_vertices, int use_pem);
void UMC_Chunk_destroy(struct UMC_Chunk* chunk);
void UMC_Chunk_run(struct UMC_Chunk* chunk, vec3* corner_verts, int silent);
void _UMC_Chunk_label_grid(struct UMC_Chunk* chunk, vec3* corner_verts);
int _UMC_Chunk_label_edges(struct UMC_Chunk* chunk, int silent);
void _UMC_Chunk_snap_verts(struct UMC_Chunk* chunk, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, uint32_t* out_indexes, uint32_t out_index_size, float w, struct osn_context* osn);
void _UMC_Chunk_polygonize(struct UMC_Chunk* chunk);
void _UMC_Chunk_create_VAO(struct UMC_Chunk* chunk);
extern __forceinline int _UMC_Chunk_calc_edge_crossing(uint32_t dim, uint16_t* grid_signs, uint32_t x0, uint32_t y0, uint32_t z0, uint32_t x1, uint32_t y1, uint32_t z1, uint32_t s0, int pem);
extern __forceinline int _UMC_Chunk_calc_edge_isov(struct UMC_Chunk* chunk, struct UMC_Edge* edge, struct UMC_Isovertex* grid, uint32_t* edge_v, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, float w, struct osn_context* osn);
extern inline void _UMC_Chunk_gen_tris(struct UMC_Cell* cell, uint32_t** out_indexes, uint32_t* next_index, uint32_t* outsize, int pem);
extern inline void _UMC_get_grad(float x, float y, float z, float w, vec3 out, struct osn_context* osn);
extern __forceinline void _UMC_Chunk_set_isov(struct UMC_Isovertex* isov, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, float w, struct osn_context* osn);
extern __forceinline void _UMC_Chunk_trilerp(float x, float y, float z, vec3* verts, vec3 out);