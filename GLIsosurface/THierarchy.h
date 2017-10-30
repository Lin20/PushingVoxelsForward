#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include "Tetrahedron.h"
#include "Util.h"
#include "Hashmap.h"
#include "MemoryPool.h"

struct TDiamond
{
	int needs_split : 1;
	int t_count;
	int id;
	struct TetrahedronNode* tetrahedra[128];
};

struct TVec3DictionaryEntry
{
	uint64_t hash;
	vec3 key;
	struct TDiamond value;
};

DEFINE_HASHMAP(TVec3Dictionary, struct TVec3DictionaryEntry)

struct TDiamondStorage
{
	TVec3Dictionary diamonds;
	pool t_pool;
};

struct SplitCheckQueue
{
	struct TetrahedronNode** queue;
	uint32_t next;
	uint32_t size;
};

struct THierarchy
{
	int pem : 1;
	int t_resolution;
	int size;
	float snap_threshold;
	uint32_t vertex_count;
	int max_depth;
	int leaf_count;
	uint32_t v_count;
	uint32_t p_count;
	uint32_t last_extract_time;
	struct TetrahedronNode top_level[6];
	struct TetrahedronNode* first_leaf;
	struct TetrahedronNode* last_leaf;
	struct TDiamondStorage diamonds;

	struct SplitCheckQueue splits;

	GLuint outline_vao;
	GLuint outline_vbo;
	GLuint outline_ibo;
	uint32_t outline_vbo_size;
	uint32_t outline_ibo_size;
	uint32_t outline_p_count;
	int outline_created : 1;

	struct osn_context* osn;
};

void TDiamond_init(struct TDiamond* dest);

void TDiamondStorage_init(struct TDiamondStorage* dest);
void TDiamondStorage_destroy(struct TDiamondStorage* dest);
int TDiamondStorage_add_tetrahedron(struct TDiamondStorage* storage, struct TetrahedronNode* t);
void _TDiamondStorage_update_lookup(struct TDiamondStorage* storage, struct TVec3DictionaryEntry* entry);

void THierarchy_init(struct THierarchy* dest, int t_resolution);
void THierarchy_destroy(struct THierarchy* dest);
void THierarchy_create_outline(struct THierarchy* dest);
void THierarchy_split_first(struct THierarchy* dest, vec3 view_pos);
void THierarchy_check_split(struct THierarchy* dest, struct TetrahedronNode* t, vec3 view_pos);
void THierarchy_split_diamond(struct THierarchy* dest, struct TVec3DictionaryEntry* diamond);
void THierarchy_extract_all_leaves(struct THierarchy* dest);

int _THierarchy_enqueue_split(struct THierarchy* dest, struct TetrahedronNode* t);
int _THierarchy_needs_split(struct TetrahedronNode* t, vec3 v, int tetra_resolution);
void _THierarchy_update_leaves(struct THierarchy* dest);