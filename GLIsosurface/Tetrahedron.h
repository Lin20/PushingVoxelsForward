#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <cglm\cglm.h>
#include <stdint.h>
#include "Hexahedron.h"

struct TetrahedronNode
{
	int child_index : 1;
	int stored_as_leaf : 1;
	int hex_init : 1;
	int gl_init : 1;
	struct TetrahedronNode* next;
	struct TetrahedronNode* prev;
	int level;
	int type;
	int branch;
	int refinement_edge[2];
	uint32_t size;
	struct TetrahedronNode* parent;
	struct TetrahedronNode* children[2];
	vec3 vertices[4];
	vec3 middle;
	vec3 refinement_key;
	float radius;
	struct Hexahedron hexahedra[4];

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
};

void TetrahedronNode_init_top_level(struct TetrahedronNode* out, int branch, int size, vec3 start);
void TetrahedronNode_init_child(struct TetrahedronNode* out, struct TetrahedronNode* parent, int child_index, vec3 mv, int* vs);
void TetrahedronNode_destroy(struct TetrahedronNode* t);
int TetrahedronNode_split(struct TetrahedronNode* t, struct TDiamondStorage* storage);
int TetrahedronNode_add_outline(struct TetrahedronNode* out, vec3** out_verts, uint32_t** out_inds, uint32_t* v_next, uint32_t* v_size, uint32_t* i_next, uint32_t* i_size);
int TetrahedronNode_is_leaf(struct TetrahedronNode* t);
int TetrahedronNode_extract(struct TetrahedronNode* t, uint32_t* v_count, uint32_t* p_count, int pem, float threshold, struct osn_context* osn, int sub_resolution);