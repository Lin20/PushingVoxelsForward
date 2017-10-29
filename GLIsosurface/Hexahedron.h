#pragma once

#include <cglm\cglm.h>
#include "UniformMarchingCubes.h"

struct Hexahedron
{
	vec3 corner_verts[8];
	struct UMC_Chunk chunk;
};

void Hexahedron_init(struct Hexahedron* h, vec3 t_verts[4], int index, int flip);
void Hexahedron_destroy(struct Hexahedron* h);
void Hexahedron_run(struct Hexahedron* h);