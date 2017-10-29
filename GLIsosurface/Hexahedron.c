#include "Hexahedron.h"
#include "TetrahedronTable.h"
#include "Util.h"
#include "Options.h"

#define CHUNK_SIZE 7

void Hexahedron_init(struct Hexahedron* h, vec3 t_verts[4], int index, int flip)
{
	for (int i = 0; i < 8; i++)
	{
		vec3 v = { 0,0,0 };
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

	UMC_Chunk_init(&h->chunk, CHUNK_SIZE, 1, !USE_REGULAR_MC);
}

void Hexahedron_destroy(struct Hexahedron* h)
{
	UMC_Chunk_destroy(&h->chunk);
}

void Hexahedron_run(struct Hexahedron* h)
{
	UMC_Chunk_run(&h->chunk, h->corner_verts, 1);
}
