#include "THierarchy.h"
#include "TetrahedronTable.h"
#include <time.h>

#define TVEC3DICTIONARYENTRY_CMP(left, right) left->hash == right->hash ? vec3_compare(left->key, right->key) : 1
#define TVEC3DICTIONARYENTRY_HASH(entry) entry->hash
DECLARE_HASHMAP(TVec3Dictionary, TVEC3DICTIONARYENTRY_CMP, TVEC3DICTIONARYENTRY_HASH, free, realloc)

void TDiamond_init(struct TDiamond* dest)
{
	dest->needs_split = 0;
	dest->t_count = 0;
}

void TDiamondStorage_init(struct TDiamondStorage* dest)
{
	TVec3DictionaryNew(&dest->diamonds);
	poolInitialize(&dest->t_pool, sizeof(struct TetrahedronNode), 512);
}

void TDiamondStorage_destroy(struct TDiamondStorage* dest)
{
	TVec3DictionaryDestroy(&dest->diamonds);
	poolFreePool(&dest->t_pool);
}

int TDiamondStorage_add_tetrahedron(struct TDiamondStorage* storage, struct TetrahedronNode* t)
{
	if (t->level == 0) // Top level tetrahedra only
	{
		// There should only be one diamond at the top level
		struct TVec3DictionaryEntry query, *result;
		result = &query;
		query.hash = vec3_hash(t->refinement_key);
		vec3_copy(t->refinement_key, query.key);
		query.value.id = storage->diamonds.size;
		TDiamond_init(&query.value);
		HashMapPutResult hr = TVec3DictionaryPut(&storage->diamonds, &result, HMDR_FIND);
		if (hr == HMPR_FAILED) // TODO: handle memory failures etc
			return 1;

		assert(result->value.t_count < 6);
		result->value.tetrahedra[result->value.t_count++] = t;
		//t->refinement_diamond = &result->value;

	}
	else
	{
		// The tetrahedra has to be added to each diamond for all 6 edges
		struct TVec3DictionaryEntry query, *result;
		for (int i = 0; i < 6; i++)
		{
			int v0 = T_VERTEX_EDGE_MAPS[i][0];
			int v1 = T_VERTEX_EDGE_MAPS[i][1];
			vec3_midpoint(query.key, t->vertices[v0], t->vertices[v1]);
			query.hash = vec3_hash(query.key);
			query.value.id = storage->diamonds.size;
			TDiamond_init(&query.value);
			result = &query;

			HashMapPutResult hr = TVec3DictionaryPut(&storage->diamonds, &result, HMDR_FIND);
			if (hr == HMPR_FAILED) // TODO: handle memory failures etc
			{
				assert(0);
				return 1;
			}

			assert(result->value.t_count < 128);
			result->value.tetrahedra[result->value.t_count++] = t;
			//if (v0 == t->refinement_edge[0] && v1 == t->refinement_edge[1])
			//	t->refinement_diamond = &result->value;
		}
	}

	return 0;
}

void _TDiamondStorage_update_lookup(struct TDiamondStorage* storage, struct TVec3DictionaryEntry* entry)
{
	struct TVec3DictionaryEntry queury = *entry;
	struct TVec3DictionaryEntry* result = &queury;
	HashMapPutResult hr = TVec3DictionaryPut(&storage->diamonds, &result, HMDR_FIND);
	if (hr == HMPR_FAILED) // This should never happen
		assert(0);
	*entry = *result;
}

void THierarchy_init(struct THierarchy* dest, int t_resolution)
{
	int size = 1 << t_resolution;
	dest->t_resolution = t_resolution;
	dest->size = size;
	dest->max_depth = 0;
	dest->leaf_count = 0;
	dest->first_leaf = 0;
	dest->last_leaf = 0;
	dest->outline_vbo = 0;
	dest->outline_ibo = 0;
	dest->outline_vao = 0;
	dest->outline_created = 0;

	dest->splits.queue = malloc(sizeof(struct TetrahedronNode*) * 256);
	dest->splits.next = 0;
	dest->splits.size = 256;
	if (!dest->splits.queue)
		printf("Failed to alloc THierarchy split queue.n");

	TDiamondStorage_init(&dest->diamonds);

	vec3 start;
	vec3_set(start, (float)size * -0.5f, (float)size * -0.5f, (float)size * -0.5f);
	for (int i = 0; i < 6; i++)
	{
		TetrahedronNode_init_top_level(&dest->top_level[i], i, size, start);
		TDiamondStorage_add_tetrahedron(&dest->diamonds, &dest->top_level[i]);
	}

	vec3 view_pos = { 0, 128, 0 };
	THierarchy_split_first(dest, view_pos);
	_THierarchy_update_leaves(dest);
	THierarchy_extract_all_leaves(dest);
}

void THierarchy_destroy(struct THierarchy* dest)
{
	uint32_t safety_counter = 0;
	struct TetrahedronNode* next_node = dest->first_leaf;

	while (safety_counter++ < 1000000 && next_node)
	{
		TetrahedronNode_destroy(next_node);
		next_node = next_node->next;
	}

	free(dest->splits.queue);
	TDiamondStorage_destroy(&dest->diamonds);
	if (dest->outline_created)
	{
		glDeleteVertexArrays(1, &dest->outline_vao);
		glDeleteBuffers(2, &dest->outline_vbo);
	}
}

void THierarchy_create_outline(struct THierarchy* dest)
{
	printf("Creating hierarchy outline...");
	vec3* verts = malloc(128 * sizeof(vec3));
	if (!verts)
	{
		printf("nVertex allocation failed.n");
		return;
	}
	uint32_t* inds = malloc(128 * sizeof(uint32_t));
	if (!inds)
	{
		free(verts);
		printf("Indices allocation failed.n");
		return;
	}
	uint32_t v_next = 0;
	uint32_t i_next = 0;
	uint32_t v_size = 128;
	uint32_t i_size = 128;
	for (int i = 0; i < 6; i++)
	{
		if (TetrahedronNode_add_outline(&dest->top_level[i], &verts, &inds, &v_next, &v_size, &i_next, &i_size))
		{
			printf("nFailed to create outline on branch %i.n", i);
			break;
		}
	}

	if (v_size > 0 && i_size > 0)
	{
		if (!dest->outline_created)
		{
			dest->outline_vbo_size = v_size;
			glGenBuffers(1, &dest->outline_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, dest->outline_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * v_next, verts, GL_STATIC_DRAW);

			dest->outline_ibo_size = i_size;
			glGenBuffers(1, &dest->outline_ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest->outline_ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * i_next, inds, GL_STATIC_DRAW);

			glGenVertexArrays(1, &dest->outline_vao);
		}
		else
		{
			if (v_size <= dest->outline_vbo_size)
			{
				glBindBuffer(GL_ARRAY_BUFFER, dest->outline_vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * v_next, verts, GL_STATIC_DRAW);
			}
			else
			{
				glDeleteBuffers(1, &dest->outline_vbo);
				dest->outline_vbo_size = v_size;

				glGenBuffers(1, &dest->outline_vbo);
				glBindBuffer(GL_ARRAY_BUFFER, dest->outline_vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * v_next, verts, GL_STATIC_DRAW);
			}

			if (i_size <= dest->outline_ibo_size)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest->outline_ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * i_next, inds, GL_STATIC_DRAW);
			}
			else
			{
				glDeleteBuffers(1, &dest->outline_ibo);
				dest->outline_ibo_size = i_size;

				glGenBuffers(1, &dest->outline_ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest->outline_ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * i_next, inds, GL_STATIC_DRAW);
			}
		}

		glBindVertexArray(dest->outline_vao);
		glBindBuffer(GL_ARRAY_BUFFER, dest->outline_vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest->outline_ibo);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	}

	dest->outline_created = 1;
	dest->outline_p_count = i_next;
	free(verts);
	free(inds);
	printf("Done.\n\n");
}

void THierarchy_split_first(struct THierarchy* dest, vec3 view_pos)
{
	dest->splits.next = 0;
	for (int i = 0; i < 6; i++)
	{
		_THierarchy_enqueue_split(dest, &dest->top_level[i]);
		THierarchy_check_split(dest, &dest->top_level[i], view_pos);
	}
}

void THierarchy_check_split(struct THierarchy* dest, struct TetrahedronNode* t, vec3 view_pos)
{
	if (_THierarchy_needs_split(t, view_pos, dest->t_resolution))
	{
		struct TVec3DictionaryEntry entry;
		entry.hash = vec3_hash(t->refinement_key);
		vec3_copy(t->refinement_key, entry.key);
		_TDiamondStorage_update_lookup(&dest->diamonds, &entry);

		THierarchy_split_diamond(dest, &entry);
		THierarchy_check_split(dest, t->children[0], view_pos);
		THierarchy_check_split(dest, t->children[1], view_pos);
	}
}

void THierarchy_split_diamond(struct THierarchy* dest, struct TVec3DictionaryEntry* diamond)
{
	int id = diamond->value.id;
	for (int i = 0; i < diamond->value.t_count; i++)
	{
		struct TetrahedronNode* t = diamond->value.tetrahedra[i];
		assert(diamond->value.id == id);
		if (!t->children[0] && !t->children[1] && t->parent)
		{
			struct TVec3DictionaryEntry entry;
			entry.hash = vec3_hash(t->parent->refinement_key);
			vec3_copy(t->parent->refinement_key, entry.key);
			_TDiamondStorage_update_lookup(&dest->diamonds, &entry);

			THierarchy_split_diamond(dest, &entry);
			_TDiamondStorage_update_lookup(&dest->diamonds, diamond);
		}

		assert(diamond->value.id == id);
		if (!t->children[0] && !t->children[1])
		{
			TetrahedronNode_split(t, &dest->diamonds);
			_THierarchy_enqueue_split(dest, t->children[0]);
			_THierarchy_enqueue_split(dest, t->children[1]);
		}

		_TDiamondStorage_update_lookup(&dest->diamonds, diamond);
	}
}

void THierarchy_extract_all_leaves(struct THierarchy* dest)
{
	printf("Extracting mesh on %i leaves...", dest->leaf_count);
	clock_t start_clock = clock();

	uint32_t safety_counter = 0;
	uint32_t leaf_counter = 0;
	uint32_t v_count = 0, p_count = 0;
	struct TetrahedronNode* next_node = dest->first_leaf;

	while (safety_counter++ < 1000000 && next_node)
	{
		leaf_counter++;
		TetrahedronNode_extract(next_node, &v_count, &p_count);
		next_node = next_node->next;
	}

	if (safety_counter < 1000000)
	{
		double delta = clock() - start_clock;
		printf("done (%i ms)\n%i verts, %i prims.\n\n", (int)(delta / (double)CLOCKS_PER_SEC * 1000.0), v_count, p_count);
		assert(leaf_counter == dest->leaf_count);
	}
	else
		printf("Safety counter triggered!\n\n");
}

int _THierarchy_enqueue_split(struct THierarchy* dest, struct TetrahedronNode* t)
{
	if (dest->splits.next >= dest->splits.size)
	{
		dest->splits.size *= 2;
		dest->splits.queue = realloc(dest->splits.queue, dest->splits.size * sizeof(struct TetrahedronNode*));
		if (!*dest->splits.queue)
			return 1;
	}

	dest->splits.queue[dest->splits.next++] = t;
	return 0;
}

int _THierarchy_needs_split(struct TetrahedronNode* t, vec3 v, int tetra_resolution)
{
	//return 0;
	if (t->level < tetra_resolution * 2 + 4)
	{
		float a = 1.0f;
		float b = 2.0f;
		float c = 0.7f;
		float r = t->radius;
		float d = vec3_distance2(v, t->middle) / r - b;

		int split = d * a < c;
		return split;
	}

	return 0;
}

void _THierarchy_update_leaves(struct THierarchy* dest)
{
	uint32_t splits_count = dest->splits.next;
	for (uint32_t i = 0; i < splits_count; i++)
	{
		struct TetrahedronNode* t = dest->splits.queue[i];
		assert(t);
		if (!t->stored_as_leaf && TetrahedronNode_is_leaf(t))
		{
			t->stored_as_leaf = 1;
			if (!dest->first_leaf)
			{
				dest->first_leaf = t;
				dest->last_leaf = t;
				dest->leaf_count++;
			}
			else
			{
				assert(dest->last_leaf);
				t->prev = dest->last_leaf;
				dest->last_leaf->next = t;
				dest->last_leaf = t;
				dest->leaf_count++;
			}
		}
	}

	printf("Updated leaves (%i).\n", dest->leaf_count);
}
