#include "Hexahedron.h"
#include "TetrahedronTable.h"
#include "Util.h"
#include "Options.h"

#define CHUNK_SIZE 7

void Hexahedron_init(struct Hexahedron* h, vec3 t_verts[4], int index, int flip, int pem, float threshold)
{
	for (int i = 0; i < 8; i++)
	{
		vec3 v = { 0, 0, 0 };
		for (int k = 0; k < H_VERTEX_COUNTS[i]; k++)
		{
			int j = H_VERTEX_MAPS[i][k];
			j = H_VERTEX_TRANSLATIONS[index][j];
			glm_vec_add(v, t_verts[j], v);
		}

		float d = 1.0f / (float)H_VERTEX_COUNTS[i];
		v[0] *= d;
		v[1] *= d;
		v[2] *= d;
		vec3_copy(v, h->corner_verts[(flip ? (i ^ 1) : i)]);
	}

	UMC_Chunk_init(&h->chunk, CHUNK_SIZE, 1, pem, threshold);
}

void Hexahedron_destroy(struct Hexahedron* h)
{
	UMC_Chunk_destroy(&h->chunk);
}

void Hexahedron_run(struct Hexahedron* h, vec3** v_out, vec3** n_out, uint32_t* vn_size, uint32_t* vn_next, uint32_t** i_out, uint32_t* i_size, uint32_t* i_next, struct osn_context* osn)
{
	h->chunk.v_out = v_out;
	h->chunk.n_out = n_out;
	h->chunk.vn_size = vn_size;
	h->chunk.vn_next = vn_next;
	h->chunk.i_out = i_out;
	h->chunk.i_size = i_size;
	h->chunk.i_next = i_next;
	UMC_Chunk_run(&h->chunk, h->corner_verts, 1, osn);
}
