#include "UniformMarchingCubes.h"

#include <assert.h>
#include <memory.h>
#include <time.h>
#include "Sampler.h"
#include "Util.h"
#include "MCTable.h"
#include "MortonCoding.h"
#include "DebugHeader.h"
#include "Options.h"

#define INDEX3D(x,y,z,d) ((x) * (d) * (d) + (y) * (d) + (z))
#define ISOLEVEL 0.0f
#define EDGE_X 0
#define EDGE_Y 1
#define EDGE_Z 2

#define USE_MORTON_CODING 0
#define ENCODE3D(x,y,z,dp1) (USE_MORTON_CODING ? GetMortonCode((x),(y),(z)) : INDEX3D(x,y,z,dp1))

#define MC_POLYGONIZE_L(xoff, yoff, zoff, m) \
	if (grid_signs[ENCODE3D((x + xoff) >> 1, (y + yoff) >> 1, (z + zoff) >> 1, dimp1_h)] & (1 << ((((z + zoff) & 1) * 1) + (((y + yoff) & 1) * 2) + (((x + xoff) & 1) * 4)))) \
		mask |= 1 << m;

#define SNAPMC_POLYGONIZE_L(xoff, yoff, zoff, m, idx) \
	lsh = (((((z + zoff) & 1) * 1) + (((y + yoff) & 1) * 2) + (((x + xoff) & 1) * 4)) * 2); \
	v = (grid_signs[ENCODE3D((x + xoff) >> 1, (y + yoff) >> 1, (z + zoff) >> 1, dimp1_h)] >> lsh) & 3; \
	mask += v * m; \
	if (v == 1) \
		cell.iso_verts[idx] = &grid_verts[INDEX3D(x + xoff, y + yoff, z + zoff, dim + 1)].index;

#define ADD_OUTPUT_INDEX(index3d) \
if (next_index == out_ind_size) \
{ \
	out_ind_size *= 2; \
	out_indexes = realloc(out_indexes, out_ind_size * sizeof(uint32_t)); \
} \
out_indexes[next_index++] = index3d;

#define SNAPMC_EDGE_CHECK(x, y, z, i) \
e = edges[INDEX3D(x, y, z, dim + 1) * 3 + i]; \
if (e.grid_v0 != e.grid_v1 && e.length > 0.0f) \
{ \
	assert(e.grid_v0 == v_index || e.grid_v1 == v_index); \
	assert(e.length > 0.0f); \
	if (e.length > max_length) \
		max_length = e.length; \
	distance = vec3_distance(e.iso_vertex.position, v->position) / e.length; \
	if (distance < min_distance) \
	{ \
		min_distance = distance; \
		min_edge = e; \
	} \
}

typedef uint32_t COORD_TYPE;

const float(*sampler_fn)(float x, float y, float z, float w, struct osn_context* osn) = &SurfaceFn_3d_terrain;

void UMC_Chunk_init(struct UMC_Chunk* dest, uint32_t dim, int index_primitives, int use_pem, float threshold)
{
	assert(dim != 0);

	dest->timer = 0;
	dest->indexed_primitives = index_primitives;
	dest->pem = use_pem;
	dest->snap_threshold = threshold;
	dest->initialized = 0;

	dest->vao = 0;
	dest->v_vbo = 0;
	dest->n_vbo = 0;
	dest->ibo = 0;

	dest->dim = dim;
	dest->v_count = 0;
	dest->p_count = 0;
	dest->snapped_count = 0;

	dest->grid_verts = 0;
	dest->edges = 0;

	dest->grid_signs = 0;
	dest->grid_verts = 0;
	dest->edges = 0;
	dest->edge_v_indexes = 0;

	dest->v_out = 0;
	dest->n_out = 0;
	dest->vn_size = 0;
	dest->vn_next = 0;
	dest->i_out = 0;
	dest->i_size = 0;
	dest->i_next = 0;
}

void UMC_Chunk_destroy(struct UMC_Chunk* chunk)
{
	assert(chunk);
	free(chunk->grid_signs);
	free(chunk->grid_verts);
	free(chunk->edges);
	free(chunk->edge_v_indexes);
	if (chunk->initialized)
	{
		//glDeleteVertexArrays(1, &chunk->vao);
		//glDeleteBuffers((chunk->indexed_primitives ? 3 : 2), &chunk->v_vbo);
	}

	chunk->timer = 0;
	chunk->indexed_primitives = 0;
	chunk->pem = 0;
	chunk->snap_threshold = 0;
	chunk->initialized = 0;

	chunk->vao = 0;
	chunk->v_vbo = 0;
	chunk->n_vbo = 0;
	chunk->ibo = 0;

	chunk->dim = 0;
	chunk->v_count = 0;
	chunk->p_count = 0;
	chunk->snapped_count = 0;

	chunk->grid_verts = 0;
	chunk->edges = 0;

	chunk->grid_signs = 0;
	chunk->grid_verts = 0;
	chunk->edges = 0;
	chunk->edge_v_indexes = 0;
}

void UMC_Chunk_run(struct UMC_Chunk* chunk, vec3* corner_verts, int silent, struct osn_context* osn)
{
	assert(chunk);
	double total_ms = 0;
	double temp = 0;

	if (!silent)
		printf("Running MC on chunk.\n--dim: %i\n--indexed: %s\n--pem: %s\n", chunk->dim, BOOL_TO_STRING(chunk->indexed_primitives), BOOL_TO_STRING(chunk->pem));

	if (!chunk->initialized)
	{
		uint32_t dimp1 = chunk->dim + 1;
		uint32_t dimp1_h = dimp1 / 2;

		chunk->grid_signs = malloc(dimp1_h * dimp1_h * dimp1_h * sizeof(uint16_t));
		chunk->grid_verts = malloc(dimp1 * dimp1 * dimp1 * sizeof(struct UMC_Isovertex));
		chunk->edges = malloc(dimp1 * dimp1 * dimp1 * 3 * sizeof(struct UMC_Edge));
		chunk->edge_v_indexes = malloc(dimp1 * dimp1 * dimp1 * 3 * sizeof(uint32_t));

		memset(chunk->grid_signs, 0, dimp1_h * dimp1_h * dimp1_h * sizeof(uint16_t));
		memset(chunk->edges, 0, dimp1 * dimp1 * dimp1 * 3 * sizeof(struct UMC_Edge));
		memset(chunk->edge_v_indexes, 0, dimp1 * dimp1 * dimp1 * 3 * sizeof(uint32_t));
	}
	else
	{
		if (!silent)
			printf("-Reset chunk...");
		clock_t start_clock = clock();
		uint32_t dimp1 = chunk->dim + 1;
		uint32_t dimp1_h = dimp1 / 2;

		memset(chunk->grid_signs, 0, dimp1_h * dimp1_h * dimp1_h * sizeof(uint16_t));
		memset(chunk->edges, 0, dimp1 * dimp1 * dimp1 * 3 * sizeof(struct UMC_Edge));
		memset(chunk->edge_v_indexes, 0, dimp1 * dimp1 * dimp1 * 3 * sizeof(uint32_t));

		total_ms += clock() - start_clock;
		if (!silent)
			printf("done (%i ms)\n", (int)(total_ms / (double)CLOCKS_PER_SEC * 1000.0));
	}


	if (!silent)
		printf("-Label grid...");
	clock_t start_clock = clock();
	_UMC_Chunk_label_grid(chunk, corner_verts, osn);
	temp = clock() - start_clock;
	total_ms += temp;
	if (!silent)
		printf("done (%i ms)\n-Label edges...", (int)(temp / (double)CLOCKS_PER_SEC * 1000.0));

	start_clock = clock();
	if (!_UMC_Chunk_label_edges(chunk, silent, osn))
	{
		if (!silent)
			printf("done.\nNo edge crossings detected. Early abandon.\n");
	}
	else
	{
		temp = clock() - start_clock;
		total_ms += temp;
		if (!silent)
			printf("done (%i ms)\n-Polygonize...", (int)(temp / (double)CLOCKS_PER_SEC * 1000.0));

		start_clock = clock();
		_UMC_Chunk_polygonize(chunk);
		temp = clock() - start_clock;
		total_ms += temp;
		if (!silent)
			printf("done (%i ms)\n-Create VAO...", (int)(temp / (double)CLOCKS_PER_SEC * 1000.0));

		start_clock = clock();
		_UMC_Chunk_create_VAO(chunk);
		temp = clock() - start_clock;
		total_ms += temp;

		if (!silent)
			printf("done (%i ms)\nComplete in %i ms. %i verts, %i prims (%i snapped).\n\n", (int)(temp / (double)CLOCKS_PER_SEC * 1000.0), (int)(total_ms / (double)CLOCKS_PER_SEC * 1000.0), chunk->v_count, chunk->p_count / 3, chunk->snapped_count);

		chunk->initialized = 1;
	}
}

void _UMC_Chunk_label_grid(struct UMC_Chunk* chunk, vec3* corner_verts, struct osn_context* osn)
{
	assert(chunk);
	assert(chunk->grid_verts);
	assert(sampler_fn);

	int pem = chunk->pem;
	uint32_t dim = chunk->dim + 1;
	uint16_t* grid_signs = chunk->grid_signs;
	struct UMC_Isovertex* grid_verts = chunk->grid_verts;
	struct UMC_Isovertex* v;
	float fx, fy, fz, s;
	float w = chunk->timer;

	if (!corner_verts)
	{
		for (uint32_t x = 0; x < dim; x++)
		{
			fx = (float)x - (float)(dim / 2);
			for (uint32_t y = 0; y < dim; y++)
			{
				fy = (float)y - (float)(dim / 2);
				for (uint32_t z = 0; z < dim; z++)
				{
					fz = (float)z - (float)(dim / 2);
					s = sampler_fn(fx, fy, fz, w, osn);
					v = &grid_verts[INDEX3D(x, y, z, dim)];
					v->value = s;
					v->index = -1;
					vec3_set(v->position, fx, fy, fz);
					if (!pem)
					{
						if (s < ISOLEVEL)
						{
							uint32_t lsh = (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4));
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
					}
					else
					{
						uint32_t lsh = (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4)) * 2;
						assert(((grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] >> lsh) & 3) == 0);
						if (s < ISOLEVEL) // 0
						{
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] &= ~mask;
						}
						else if (s == ISOLEVEL) // 1
						{
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
						else if (s > ISOLEVEL) // 2
						{
							uint32_t mask = 2 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
					}
				}
			}
		}
	}
	else
	{
		float f_delta = 1.0f / (float)(chunk->dim);
		vec3 interpolated_point;
		for (uint32_t x = 0; x < dim; x++)
		{
			fx = (float)x * f_delta;
			for (uint32_t y = 0; y < dim; y++)
			{
				fy = (float)y * f_delta;
				for (uint32_t z = 0; z < dim; z++)
				{
					fz = (float)z * f_delta;
					_UMC_Chunk_trilerp(fx, fy, fz, corner_verts, interpolated_point);
					s = sampler_fn(interpolated_point[0], interpolated_point[1], interpolated_point[2], w, osn);
					v = &grid_verts[INDEX3D(x, y, z, dim)];
					v->value = s;
					v->index = -1;
					vec3_set(v->position, interpolated_point[0], interpolated_point[1], interpolated_point[2]);
					if (!pem)
					{
						if (s < ISOLEVEL)
						{
							uint32_t lsh = (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4));
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
					}
					else
					{
						uint32_t lsh = (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4)) * 2;
						assert(((grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] >> lsh) & 3) == 0);
						if (s < ISOLEVEL) // 0
						{
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] &= ~mask;
						}
						else if (s == ISOLEVEL) // 1
						{
							uint32_t mask = 1 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
						else if (s > ISOLEVEL) // 2
						{
							uint32_t mask = 2 << lsh;
							grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim / 2)] |= mask;
						}
					}
				}
			}
		}
	}
}

int _UMC_Chunk_label_edges(struct UMC_Chunk* chunk, int silent, struct osn_context* osn)
{
	assert(chunk);
	assert(chunk->edges);

	int pem = chunk->pem;
	uint32_t dim = chunk->dim;
	uint32_t dim_h = (chunk->dim + 1) / 2;
	uint16_t* grid_signs = chunk->grid_signs;
	struct UMC_Isovertex* grid = chunk->grid_verts;
	struct UMC_Edge* edges = chunk->edges;
	struct UMC_Edge *e_x, *e_y, *e_z;
	uint32_t* edge_v_indexes = chunk->edge_v_indexes;
	uint32_t* edge_v;

	//vec3* out_vertices = malloc(4096 * sizeof(vec3));
	//vec3* out_normals = malloc(4096 * sizeof(vec3));
	//uint32_t next_vertex = 0;
	//uint32_t out_size = 4096;

	uint32_t start_index = *chunk->vn_next;
	vec3** out_vertices = chunk->v_out;
	vec3** out_normals = chunk->n_out;
	uint32_t* next_vertex = chunk->vn_next;
	uint32_t* out_size = chunk->v_out;

	uint32_t* out_indexes = 0;
	uint32_t next_index = 0;
	uint32_t out_ind_size = 4096;

	if (pem)
		out_indexes = malloc(4096 * sizeof(uint32_t));

	float w = chunk->timer;

	uint32_t v0;
	int result_mask;
	float distance;
	uint32_t s0, s0_mask;

	for (uint32_t x = 0; x < dim + 1; x++)
	{
		for (uint32_t y = 0; y < dim + 1; y++)
		{
			for (uint32_t z = 0; z < dim + 1; z++)
			{
				v0 = INDEX3D(x, y, z, dim + 1);
				if (!pem)
				{
					s0_mask = 1 << (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4));
					s0 = grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim_h)] & s0_mask;
				}
				else
				{
					uint32_t ls_0 = ((((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4))) * 2;
					s0_mask = 3 << ls_0;
					s0 = (grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim_h)] & s0_mask);
					s0 >>= ls_0;
				}

				if (x < dim)
				{
					result_mask = _UMC_Chunk_calc_edge_crossing(dim_h, grid_signs, x, y, z, x + 1, y, z, s0, pem);
					if ((!pem && result_mask) || (pem && (result_mask & 4)))
					{
						e_x = edges + v0 * 3;
						e_x->grid_v0 = v0;
						e_x->grid_v1 = INDEX3D(x + 1, y, z, dim + 1);
						edge_v = edge_v_indexes + v0 * 3;
						_UMC_Chunk_calc_edge_isov(chunk, e_x, grid, edge_v, out_vertices, out_normals, next_vertex, out_size, w, osn);

						if (pem)
						{
							grid[e_x->grid_v0].index = -2;
							grid[e_x->grid_v1].index = -2;
							if (x > 0)
								ADD_OUTPUT_INDEX(e_x->grid_v0);
							if (x < dim)
								ADD_OUTPUT_INDEX(e_x->grid_v1);
						}
					}
					if (pem && (result_mask & 1))
					{
						_UMC_Chunk_set_isov(grid + v0, out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
					if (pem && (result_mask & 2))
					{
						_UMC_Chunk_set_isov(grid + INDEX3D(x + 1, y, z, dim + 1), out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
				}
				if (y < dim)
				{
					result_mask = _UMC_Chunk_calc_edge_crossing(dim_h, grid_signs, x, y, z, x, y + 1, z, s0, pem);
					if ((!pem && result_mask) || (pem && (result_mask & 4)))
					{
						e_y = edges + v0 * 3 + 1;
						e_y->grid_v0 = v0;
						e_y->grid_v1 = INDEX3D(x, y + 1, z, dim + 1);
						edge_v = edge_v_indexes + v0 * 3 + 1;
						_UMC_Chunk_calc_edge_isov(chunk, e_y, grid, edge_v, out_vertices, out_normals, next_vertex, out_size, w, osn);

						if (pem)
						{
							grid[e_y->grid_v0].index = -2;
							grid[e_y->grid_v1].index = -2;
							if (y > 0)
								ADD_OUTPUT_INDEX(e_y->grid_v0);
							if (y < dim)
								ADD_OUTPUT_INDEX(e_y->grid_v1);
						}
					}
					if (pem && (result_mask & 1))
					{
						_UMC_Chunk_set_isov(grid + v0, out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
					if (pem && (result_mask & 2))
					{
						_UMC_Chunk_set_isov(grid + INDEX3D(x, y + 1, z, dim + 1), out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
				}
				if (z < dim)
				{
					result_mask = _UMC_Chunk_calc_edge_crossing(dim_h, grid_signs, x, y, z, x, y, z + 1, s0, pem);
					if ((!pem && result_mask) || (pem && (result_mask & 4)))
					{
						e_z = edges + v0 * 3 + 2;
						e_z->grid_v0 = v0;
						e_z->grid_v1 = INDEX3D(x, y, z + 1, dim + 1);
						edge_v = edge_v_indexes + v0 * 3 + 2;
						_UMC_Chunk_calc_edge_isov(chunk, e_z, grid, edge_v, out_vertices, out_normals, next_vertex, out_size, w, osn);

						if (pem)
						{
							grid[e_z->grid_v0].index = -2;
							grid[e_z->grid_v1].index = -2;
							if (z > 0)
								ADD_OUTPUT_INDEX(e_z->grid_v0);
							if (z < dim)
								ADD_OUTPUT_INDEX(e_z->grid_v1);
						}
					}
					if (pem && (result_mask & 1))
					{
						_UMC_Chunk_set_isov(grid + v0, out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
					if (pem && (result_mask & 2))
					{
						_UMC_Chunk_set_isov(grid + INDEX3D(x, y, z + 1, dim + 1), out_vertices, out_normals, next_vertex, out_size, w, osn);
					}
				}
			}
		}
	}

	if (pem)
	{
		if (!silent)
			printf("Snapping vertices...");
		_UMC_Chunk_snap_verts(chunk, out_vertices, out_normals, next_vertex, out_size, out_indexes, next_index, w, osn);
	}

	/*if (!chunk->initialized)
	{
		if (next_vertex == 0)
			goto Cleanup;
		chunk->vbo_size = out_size;
		glGenBuffers(1, &chunk->v_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, chunk->v_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &chunk->n_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, chunk->n_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_normals, GL_STATIC_DRAW);
	}
	else
	{
		if (out_size <= chunk->vbo_size)
		{
			glBindBuffer(GL_ARRAY_BUFFER, chunk->v_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_vertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, chunk->n_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_normals, GL_STATIC_DRAW);
		}
		else
		{
			glDeleteBuffers(2, &chunk->v_vbo);
			chunk->vbo_size = out_size;

			glGenBuffers(1, &chunk->v_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, chunk->v_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_vertices, GL_STATIC_DRAW);

			glGenBuffers(1, &chunk->n_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, chunk->n_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * next_vertex, out_normals, GL_STATIC_DRAW);
		}
	}*/
Cleanup:
	chunk->v_count = *next_vertex - start_index;

	//free(out_vertices);
	//free(out_normals);
	if (pem)
		free(out_indexes);

	return 1;
}

void _UMC_Chunk_snap_verts(struct UMC_Chunk* chunk, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, uint32_t* out_indexes, uint32_t out_index_size, float w, struct osn_context* osn)
{
	assert(chunk);
	assert(chunk->grid_verts);
	assert(sampler_fn);

	uint32_t snapped_count = 0;
	int pem = chunk->pem;
	uint32_t dim = chunk->dim;
	uint32_t dim_h = (chunk->dim + 1) / 2;
	uint16_t* grid_signs = chunk->grid_signs;
	struct UMC_Isovertex* grid_verts = chunk->grid_verts;
	struct UMC_Edge* edges = chunk->edges;
	struct UMC_Isovertex* v;
	float fx, fy, fz, s;
	float snap_threshold = chunk->snap_threshold;

	uint32_t v_index;
	for (uint32_t idx = 0; idx < out_index_size; idx++)
	{
		v_index = out_indexes[idx];
		v = grid_verts + v_index;
		uint32_t x = v_index / (dim + 1) / (dim + 1);
		if (x == 0 || x >= dim)
			continue;
		uint32_t y = v_index / (dim + 1) % (dim + 1);
		if (y == 0 || y >= dim)
			continue;
		uint32_t z = v_index % (dim + 1);
		if (z == 0 || z >= dim)
			continue;

		float min_distance = 3.4e37f;
		float distance;
		float max_length = 0;
		struct UMC_Edge e;
		struct UMC_Edge min_edge;

		SNAPMC_EDGE_CHECK(x, y, z, 0);
		SNAPMC_EDGE_CHECK(x, y, z, 1);
		SNAPMC_EDGE_CHECK(x, y, z, 2);
		SNAPMC_EDGE_CHECK(x - 1, y, z, 0);
		SNAPMC_EDGE_CHECK(x, y - 1, z, 1);
		SNAPMC_EDGE_CHECK(x, y, z - 1, 2);

		if (min_distance <= snap_threshold)
		{
			uint32_t lsh = (((z & 1) * 1) + ((y & 1) * 2) + ((x & 1) * 4)) * 2;
			grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim_h)] &= ~(3 << lsh);
			grid_signs[ENCODE3D(x >> 1, y >> 1, z >> 1, dim_h)] |= (1 << lsh);
			vec3_copy(min_edge.iso_vertex.position, v->position);

			v->index = min_edge.iso_vertex.index;
			snapped_count++;
		}
	}

	chunk->snapped_count = snapped_count;
}

void _UMC_Chunk_polygonize(struct UMC_Chunk* chunk)
{
	assert(chunk);
	int pem = chunk->pem;
	uint32_t dim = chunk->dim;
	uint32_t dimp1_h = (chunk->dim + 1) / 2;
	uint16_t* grid_signs = chunk->grid_signs;
	struct UMC_Isovertex* grid_verts = chunk->grid_verts;
	struct UMC_Edge* edges = chunk->edges;
	uint32_t* edge_v_indexes = chunk->edge_v_indexes;
	struct UMC_Cell cell;
	uint32_t v0;

	//uint32_t* out_indexes = malloc(4096 * sizeof(uint32_t));
	//uint32_t next_index = 0;
	//uint32_t out_size = 4096;
	uint32_t start_index = *chunk->i_next;
	uint32_t** out_indexes = chunk->i_out;
	uint32_t* next_index = chunk->i_next;
	uint32_t out_size = chunk->i_size;
	uint8_t temp_signs[8];

	for (uint32_t x = 0; x < dim; x++)
	{
		for (uint32_t y = 0; y < dim; y++)
		{
			for (uint32_t z = 0; z < dim; z++)
			{
				cell.mask = 0;
				if (!pem)
				{
					uint32_t mask = 0;

					MC_POLYGONIZE_L(0, 0, 0, 0);
					MC_POLYGONIZE_L(1, 0, 0, 1);
					MC_POLYGONIZE_L(1, 0, 1, 2);
					MC_POLYGONIZE_L(0, 0, 1, 3);
					MC_POLYGONIZE_L(0, 1, 0, 4);
					MC_POLYGONIZE_L(1, 1, 0, 5);
					MC_POLYGONIZE_L(1, 1, 1, 6);
					MC_POLYGONIZE_L(0, 1, 1, 7);

					if (mask == 0 || mask == 255)
						continue;
					cell.mask = mask;
				}
				else
				{
					uint32_t v;
					uint32_t mask = 0;
					uint32_t lsh = 0;

					SNAPMC_POLYGONIZE_L(0, 0, 0, 1, 0);
					SNAPMC_POLYGONIZE_L(1, 0, 0, 3, 1);
					SNAPMC_POLYGONIZE_L(0, 1, 0, 9, 2);
					SNAPMC_POLYGONIZE_L(1, 1, 0, 27, 3);
					SNAPMC_POLYGONIZE_L(0, 0, 1, 81, 4);
					SNAPMC_POLYGONIZE_L(1, 0, 1, 243, 5);
					SNAPMC_POLYGONIZE_L(0, 1, 1, 729, 6);
					SNAPMC_POLYGONIZE_L(1, 1, 1, 2187, 7);

					if (MCPEM_Table[mask][0] == 0)
						continue;
					cell.mask = mask;
				}

				v0 = INDEX3D(x, y, z, dim + 1);
				if (!pem)
				{
					// Follow common mc format
					cell.iso_verts[0] = edge_v_indexes + v0 * 3 + EDGE_X;
					cell.iso_verts[1] = edge_v_indexes + INDEX3D(x + 1, y, z, dim + 1) * 3 + EDGE_Z;
					cell.iso_verts[2] = edge_v_indexes + INDEX3D(x, y, z + 1, dim + 1) * 3 + EDGE_X;
					cell.iso_verts[3] = edge_v_indexes + v0 * 3 + EDGE_Z;

					cell.iso_verts[4] = edge_v_indexes + INDEX3D(x, y + 1, z, dim + 1) * 3 + EDGE_X;
					cell.iso_verts[5] = edge_v_indexes + INDEX3D(x + 1, y + 1, z, dim + 1) * 3 + EDGE_Z;
					cell.iso_verts[6] = edge_v_indexes + INDEX3D(x, y + 1, z + 1, dim + 1) * 3 + EDGE_X;
					cell.iso_verts[7] = edge_v_indexes + INDEX3D(x, y + 1, z, dim + 1) * 3 + EDGE_Z;

					cell.iso_verts[8] = edge_v_indexes + v0 * 3 + EDGE_Y;
					cell.iso_verts[9] = edge_v_indexes + INDEX3D(x + 1, y, z, dim + 1) * 3 + EDGE_Y;
					cell.iso_verts[10] = edge_v_indexes + INDEX3D(x + 1, y, z + 1, dim + 1) * 3 + EDGE_Y;
					cell.iso_verts[11] = edge_v_indexes + INDEX3D(x, y, z + 1, dim + 1) * 3 + EDGE_Y;
				}
				else
				{
					cell.iso_verts[8 + 0] = edge_v_indexes + v0 * 3 + EDGE_X;
					cell.iso_verts[8 + 1] = edge_v_indexes + v0 * 3 + EDGE_Y;
					cell.iso_verts[8 + 2] = edge_v_indexes + INDEX3D(x + 1, y, z, dim + 1) * 3 + EDGE_Y;
					cell.iso_verts[8 + 3] = edge_v_indexes + INDEX3D(x, y + 1, z, dim + 1) * 3 + EDGE_X;

					cell.iso_verts[8 + 4] = edge_v_indexes + v0 * 3 + EDGE_Z;
					cell.iso_verts[8 + 5] = edge_v_indexes + INDEX3D(x + 1, y, z, dim + 1) * 3 + EDGE_Z;
					cell.iso_verts[8 + 6] = edge_v_indexes + INDEX3D(x, y + 1, z, dim + 1) * 3 + EDGE_Z;
					cell.iso_verts[8 + 7] = edge_v_indexes + INDEX3D(x + 1, y + 1, z, dim + 1) * 3 + EDGE_Z;

					cell.iso_verts[8 + 8] = edge_v_indexes + INDEX3D(x, y, z + 1, dim + 1) * 3 + EDGE_X;
					cell.iso_verts[8 + 9] = edge_v_indexes + INDEX3D(x, y, z + 1, dim + 1) * 3 + EDGE_Y;
					cell.iso_verts[8 + 10] = edge_v_indexes + INDEX3D(x + 1, y, z + 1, dim + 1) * 3 + EDGE_Y;
					cell.iso_verts[8 + 11] = edge_v_indexes + INDEX3D(x, y + 1, z + 1, dim + 1) * 3 + EDGE_X;
				}

				_UMC_Chunk_gen_tris(&cell, out_indexes, next_index, out_size, pem);
			}
		}
	}

	/*if (!chunk->initialized)
	{
		chunk->ibo_size = out_size;
		glGenBuffers(1, &chunk->ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * next_index, out_indexes, GL_STATIC_DRAW);
	}
	else
	{
		if (out_size <= chunk->ibo_size)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * next_index, out_indexes, GL_STATIC_DRAW);
		}
		else
		{
			glDeleteBuffers(1, &chunk->ibo);
			chunk->ibo_size = out_size;

			glGenBuffers(1, &chunk->ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * next_index, out_indexes, GL_STATIC_DRAW);
		}
	}*/

	chunk->p_count = *next_index - start_index;
	//free(out_indexes);
}

void _UMC_Chunk_create_VAO(struct UMC_Chunk* chunk)
{
	if (!chunk->initialized)
	{
		glGenVertexArrays(1, &chunk->vao);
	}
	glBindVertexArray(chunk->vao);
	glBindBuffer(GL_ARRAY_BUFFER, chunk->v_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, chunk->n_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->ibo);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

__forceinline int _UMC_Chunk_calc_edge_crossing(uint32_t dimp1_h, uint16_t* grid_signs, uint32_t x0, uint32_t y0, uint32_t z0, uint32_t x1, uint32_t y1, uint32_t z1, uint32_t s0, int pem)
{

	if (!pem)
	{
		int mask = 0;
		//COORD_TYPE s0_mask = 1 << (((z0 & 1) * 1) + ((y0 & 1) * 2) + ((x0 & 1) * 4));
		COORD_TYPE s1_mask = 1 << (((z1 & 1) * 1) + ((y1 & 1) * 2) + ((x1 & 1) * 4));
		//COORD_TYPE s0 = grid_signs[ENCODE3D(x0 >> 1, y0 >> 1, z0 >> 1, dimp1_h)] & s0_mask;
		COORD_TYPE s1 = grid_signs[ENCODE3D(x1 >> 1, y1 >> 1, z1 >> 1, dimp1_h)] & s1_mask;
		if (s0)
			mask |= 1;
		if (s1)
			mask |= 2;

		return mask == 1 || mask == 2;
	}
	else
	{
		int ls_0 = ((((z0 & 1) * 1) + ((y0 & 1) * 2) + ((x0 & 1) * 4))) * 2;
		int ls_1 = ((((z1 & 1) * 1) + ((y1 & 1) * 2) + ((x1 & 1) * 4))) * 2;
		//COORD_TYPE s0_mask = 3 << ls_0;
		COORD_TYPE s1_mask = 3 << ls_1;
		//COORD_TYPE s0 = (grid_signs[ENCODE3D(x0 >> 1, y0 >> 1, z0 >> 1, dimp1_h)] & s0_mask);
		COORD_TYPE s1 = (grid_signs[ENCODE3D(x1 >> 1, y1 >> 1, z1 >> 1, dimp1_h)] & s1_mask);
		//s0 >>= ls_0;
		s1 >>= ls_1;
		assert(s0 < 4 && s1 < 4);

		int out = 0;
		if (s0 == 1)
			out |= 1;
		if (s1 == 1)
			out |= 2;
		if (s0 != s1)
			out |= 4;
		return out;
	}
}

__forceinline int _UMC_Chunk_calc_edge_isov(struct UMC_Chunk* chunk, struct UMC_Edge* edge, struct UMC_Isovertex* grid, uint32_t* edge_v, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, float w, struct osn_context* osn)
{
	struct UMC_Isovertex gv0, gv1;
	gv0 = grid[edge->grid_v0];
	gv1 = grid[edge->grid_v1];

	// Old edge setting stuff, which ended up unnecessary
	// edge->has_crossing = 1;
	// edge->grid_v0 = gv0;
	// edge->grid_v1 = gv1;
	*edge_v = *next_vertex;

	edge->length = vec3_distance(gv0.position, gv1.position);
	Sampler_get_intersection(gv0.position, gv1.position, gv0.value, gv1.value, ISOLEVEL, edge->iso_vertex.position);
	_UMC_get_grad(edge->iso_vertex.position[0], edge->iso_vertex.position[1], edge->iso_vertex.position[2], w, edge->iso_vertex.normal, osn);
	edge->iso_vertex.index = *next_vertex;
	if (*next_vertex == *out_size)
	{
		*out_size *= 2;
		*out_vertices = realloc(*out_vertices, *out_size * sizeof(vec3));
		*out_normals = realloc(*out_normals, *out_size * sizeof(vec3));
	}

	vec3_set((*out_vertices)[*next_vertex], edge->iso_vertex.position[0], edge->iso_vertex.position[1], edge->iso_vertex.position[2]);
	vec3_set((*out_normals)[*next_vertex], edge->iso_vertex.normal[0], edge->iso_vertex.normal[1], edge->iso_vertex.normal[2]);
	(*next_vertex)++;
}

inline void _UMC_Chunk_gen_tris(struct UMC_Cell* cell, uint32_t** out_indexes, uint32_t* next_index, uint32_t* out_size, int pem)
{
	if (!pem)
		assert(cell->mask > 0 && cell->mask < 255);

	// Safe extending
	if (*next_index + 16 >= *out_size)
	{
		*out_size *= 2;
		*out_indexes = realloc(*out_indexes, *out_size * sizeof(uint32_t));
	}

	if (!pem)
	{
		//uint16_t mask = cell->mask;
		uint64_t mct = MCTable_compressed[cell->mask];
		for (int i = 0; i < 16; i++)
		{
			if ((mct & 0xF) == 0xF)
				break;
			int ind = (mct & 0xF);
			(*out_indexes)[*next_index] = *cell->iso_verts[ind];
			(*next_index)++;
			mct >>= 4;
		}
	}
	else
	{
		int count = MCPEM_Table[cell->mask][0];
		for (int i = 1; i < count * 3 + 1; i++)
		{
			int ind = MCPEM_Table[cell->mask][i];
			assert(*cell->iso_verts[ind] != -1);
			(*out_indexes)[*next_index] = *cell->iso_verts[ind];
			(*next_index)++;
		}
	}
}

inline void _UMC_get_grad(float x, float y, float z, float w, vec3 out, struct osn_context* osn)
{
	const float h = 0.1f;
	float dx = sampler_fn(x + h, y, z, w, osn) - sampler_fn(x - h, y, z, w, osn);
	float dy = sampler_fn(x, y + h, z, w, osn) - sampler_fn(x, y - h, z, w, osn);
	float dz = sampler_fn(x, y, z + h, w, osn) - sampler_fn(x, y, z - h, w, osn);
	vec3_set(out, dx, dy, dz);
	//glm_vec_normalize(out);
}

void _UMC_Chunk_set_isov(struct UMC_Isovertex* isov, vec3** out_vertices, vec3** out_normals, uint32_t* next_vertex, uint32_t* out_size, float w, struct osn_context* osn)
{
	if (isov->index != -1 && isov->index != -2)
		return;
	_UMC_get_grad(isov->position[0], isov->position[1], isov->position[2], w, isov->normal, osn);
	isov->index = *next_vertex;
	if (*next_vertex == *out_size)
	{
		*out_size *= 2;
		*out_vertices = realloc(*out_vertices, *out_size * sizeof(vec3));
		*out_normals = realloc(*out_normals, *out_size * sizeof(vec3));
	}

	vec3_set((*out_vertices)[*next_vertex], isov->position[0], isov->position[1], isov->position[2]);
	vec3_set((*out_normals)[*next_vertex], isov->normal[0], isov->normal[1], isov->normal[2]);
	(*next_vertex)++;
}

void _UMC_Chunk_trilerp(float x, float y, float z, vec3* verts, vec3 out)
{
	float percents[8];
	percents[0] = (1.0f - x) * (1.0f - y) * (1.0f - z);
	percents[1] = (0.0f + x) * (1.0f - y) * (1.0f - z);
	percents[2] = (0.0f + x) * (1.0f - y) * (0.0f + z);
	percents[3] = (1.0f - x) * (1.0f - y) * (0.0f + z);
	percents[4] = (1.0f - x) * (0.0f + y) * (1.0f - z);
	percents[5] = (0.0f + x) * (0.0f + y) * (1.0f - z);
	percents[6] = (0.0f + x) * (0.0f + y) * (0.0f + z);
	percents[7] = (1.0f - x) * (0.0f + y) * (0.0f + z);

	vec3_set(out, 0, 0, 0);
	vec3_add_coeff(out, verts[0], out, percents[0]);
	vec3_add_coeff(out, verts[1], out, percents[1]);
	vec3_add_coeff(out, verts[2], out, percents[2]);
	vec3_add_coeff(out, verts[3], out, percents[3]);
	vec3_add_coeff(out, verts[4], out, percents[4]);
	vec3_add_coeff(out, verts[5], out, percents[5]);
	vec3_add_coeff(out, verts[6], out, percents[6]);
	vec3_add_coeff(out, verts[7], out, percents[7]);
}
