#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <stdio.h>

#include "Camera.h"
#include "UniformMarchingCubes.h"
#include "THierarchy.h"

struct DebugScene
{
	int last_space : 1;
	int outline_visible : 1;
	int smooth_shading : 1;
	int fillmode;
	float line_width;
	float line_color[4];
	float fill_color[4];
	float clear_color[4];
	GLuint points_vbo;
	GLuint colors_vbo;
	GLuint vao;
	GLuint ibo;

	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint shader_program;

	GLuint outline_vs;
	GLuint outline_fs;
	GLuint outline_sp;

	GLuint shader_projection;
	GLuint shader_view;
	GLuint shader_mul_clr;
	GLuint shader_eye_pos;
	GLuint shader_smooth_shading;

	GLuint outline_shader_projection;
	GLuint outline_shader_view;
	GLuint outline_shader_mul_clr;

	struct FPSCamera camera;
	struct UMC_Chunk test_chunk;
	struct THierarchy hierarchy;

	struct nk_context* nkc;
};

int DebugScene_init(struct DebugScene* out, struct RenderInput* render_input);
int DebugScene_cleanup(struct DebugScene* scene);
int DebugScene_update(struct DebugScene* scene, struct RenderInput* input);
int DebugScene_render(struct DebugScene* scene, struct RenderInput* input);
