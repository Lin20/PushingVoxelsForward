#pragma once

#include <cglm\cglm.h>
#include "UniformMarchingCubes.h"

struct Hexahedron
{
	vec3 corner_verts[8];
	struct UMC_Chunk chunk;
};

void Hexahedron_init(struct Hexahedron* h, vec3 t_verts[4], int index, int flip, int pem, float threshold);
void Hexahedron_destroy(struct Hexahedron* h);
void Hexahedron_run(struct Hexahedron* h, vec3** v_out, vec3** n_out, uint32_t* vn_size, uint32_t* vn_next, uint32_t** i_out, uint32_t* i_size, uint32_t* i_next, struct osn_context* osn);