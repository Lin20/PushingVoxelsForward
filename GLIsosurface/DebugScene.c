#include <memory.h>
#include <cglm\cglm.h>

#include "DebugScene.h"
#include "Core.h"
#include "Options.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"
#include "nuklear_styles.h"
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define FILL_MODE_FILL 0
#define FILL_MODE_BOTH 1
#define FILL_MODE_WIRE 2

void abbreviate_int(int d, int* i_out, char* c_out);

int DebugScene_init(struct DebugScene* out, struct RenderInput* render_input)
{
	memset(out, 0, sizeof(struct DebugScene));
	out->fillmode = FILL_MODE_BOTH;
	out->line_width = 1.5f;

	float dx[] = { 0, 1, 0, 1, 0, 1, 0, 1 };
	float dy[] = { 0, 0, 1, 1, 0, 0, 1, 1 };
	float dz[] = { 0, 0, 0, 0, 1, 1, 1, 1 };

	float points[24];
	for (size_t i = 0; i < 8; i++)
	{
		points[i * 3] = dx[i];
		points[i * 3 + 1] = dy[i];
		points[i * 3 + 2] = dz[i];
	}

	float colors[] = {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 1.0f
	};

	unsigned int indices[] =
	{
		0, 1, 3,
		3, 2, 0,

		1, 5, 7,
		7, 3, 1,

		4, 5, 7,
		7, 6, 4,

		2, 3, 7,
		7, 6, 2
	};

	const char* vertex_shader =
		"#version 400\n"
		"attribute vec3 vertex_position;"
		"attribute vec3 vertex_normal;"
		"uniform mat4 projection;"
		"uniform mat4 view;"
		"uniform vec3 mul_color;"
		"out vec3 f_normal;"
		"out vec3 f_mul_color;"
		"void main() {"
		"  f_normal = normalize(vertex_normal);"
		"  f_mul_color = mul_color;"
		"  gl_Position = projection * view * vec4(vertex_position, 1);"
		"}";

	const char* fragment_shader =
		"#version 400\n"
		"in vec3 f_normal;"
		"in vec3 f_mul_color;"
		"out vec4 frag_color;"
		"void main() {"
		"  float d = dot(normalize(-vec3(0.1f, -1.0f, 0.5f)), f_normal);"
		"  float m = mix(0.2f, 1.0f, d * 0.5f + 0.5f);"
		"  float s = pow(max(0.0f, d), 32.0f);"
		"  vec3 color = vec3(0.3f, 0.3f, 0.5f);"
		"  vec3 color2 = vec3(0.1f, 0.1f, 0.25f);"
		"  vec3 result = f_mul_color * m + f_mul_color * s;"
		"  frag_color = vec4(result, 1.0f);"
		"}";

	const char* outline_vs =
		"#version 400\n"
		"attribute vec3 vertex_position;"
		"uniform mat4 projection;"
		"uniform mat4 view;"
		"uniform vec3 mul_color;"
		"out vec3 f_mul_color;"
		"void main() {"
		"  f_mul_color = mul_color;"
		"  gl_Position = projection * view * vec4(vertex_position, 1);"
		"}";

	const char* outline_fs =
		"#version 400\n"
		"in vec3 f_mul_color;"
		"out vec4 frag_color;"
		"void main() {"
		"  frag_color = vec4(f_mul_color, 1.0f);"
		"}";

	out->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(out->vertex_shader, 1, &vertex_shader, NULL);
	glCompileShader(out->vertex_shader);
	out->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(out->fragment_shader, 1, &fragment_shader, NULL);
	glCompileShader(out->fragment_shader);

	out->shader_program = glCreateProgram();
	glAttachShader(out->shader_program, out->fragment_shader);
	glAttachShader(out->shader_program, out->vertex_shader);

	glBindAttribLocation(out->shader_program, 0, "vertex_position");
	glBindAttribLocation(out->shader_program, 1, "vertex_normal");

	glLinkProgram(out->shader_program);
	out->shader_projection = glGetUniformLocation(out->shader_program, "projection");
	out->shader_view = glGetUniformLocation(out->shader_program, "view");
	out->shader_mul_clr = glGetUniformLocation(out->shader_program, "mul_color");

	out->outline_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(out->outline_vs, 1, &outline_vs, NULL);
	glCompileShader(out->outline_vs);
	out->outline_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(out->outline_fs, 1, &outline_fs, NULL);
	glCompileShader(out->outline_fs);

	out->outline_sp = glCreateProgram();
	glAttachShader(out->outline_sp, out->outline_fs);
	glAttachShader(out->outline_sp, out->outline_vs);

	glBindAttribLocation(out->outline_sp, 0, "vertex_position");

	glLinkProgram(out->outline_sp);
	out->outline_shader_projection = glGetUniformLocation(out->outline_sp, "projection");
	out->outline_shader_view = glGetUniformLocation(out->outline_sp, "view");
	out->outline_shader_mul_clr = glGetUniformLocation(out->outline_sp, "mul_color");

	printf("Initializing nuklear...");
	out->nkc = nk_glfw3_init(render_input->window, NK_GLFW3_INSTALL_CALLBACKS);
	struct nk_font_atlas *atlas;
	nk_glfw3_font_stash_begin(&atlas);
	nk_glfw3_font_stash_end();
	set_style(out->nkc, THEME_DARK);
	printf("Done.\n\n");

	FPSCamera_init(&out->camera, render_input->width, render_input->height, out->shader_projection, out->shader_view, render_input);
	//FPSCamera_set_shader(&out->camera, out->outline_shader_projection, out->outline_shader_view);

	THierarchy_init(&out->hierarchy, 8);
	THierarchy_create_outline(&out->hierarchy);

	//UMC_Chunk_init(&out->test_chunk, 63, 1, 1);
	//UMC_Chunk_run(&out->test_chunk, 0, 0);

	return 0;
}

int DebugScene_cleanup(struct DebugScene* scene)
{
	//UMC_Chunk_destroy(&scene->test_chunk);
	THierarchy_destroy(&scene->hierarchy);
	nk_glfw3_shutdown();
	return 0;
}

int DebugScene_update(struct DebugScene* scene, struct RenderInput* input)
{
	glfwPollEvents();
	FPSCamera_update(&scene->camera, input);
}

int DebugScene_render(struct DebugScene* scene, struct RenderInput* input)
{
	if (glfwGetKey(input->window, GLFW_KEY_SPACE))
	{
		/*float timer = scene->test_chunk.timer;
		UMC_Chunk_destroy(&scene->test_chunk);
		UMC_Chunk_init(&scene->test_chunk, 63, 1, 1);
		scene->test_chunk.timer = timer;*/
		if (glfwGetKey(input->window, GLFW_KEY_LEFT_SHIFT))
			scene->test_chunk.timer += 0.1f;
		else
			scene->test_chunk.timer += 0.01f;
		//UMC_Chunk_run(&scene->test_chunk, 0, 0,);
	}

	glClearColor(0.02f, 0.02f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(scene->shader_program);
	FPSCamera_set_shader(&scene->camera, scene->shader_projection, scene->shader_view);
	//glBindVertexArray(scene->test_chunk.vao);

	if (scene->fillmode == FILL_MODE_FILL || scene->fillmode == FILL_MODE_BOTH)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform3f(scene->shader_mul_clr, 0.0f, 0.0f, 0.05f);

		uint32_t safety_counter = 0;
		struct TetrahedronNode* next_node = scene->hierarchy.first_leaf;

		while (safety_counter++ < 1000000 && next_node)
		{
			if (next_node->p_count > 0)
			{
				glBindVertexArray(next_node->vao);
				glDrawElements(GL_TRIANGLES, next_node->p_count, GL_UNSIGNED_INT, 0);
			}
			next_node = next_node->next;
		}

		//glDrawElements(GL_TRIANGLES, scene->test_chunk.p_count, GL_UNSIGNED_INT, 0);

		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	if (scene->fillmode == FILL_MODE_WIRE)
	{
		glLineWidth(1.5f);
		glPolygonOffset(0.0f, GL_POLYGON_OFFSET_LINE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform3f(scene->shader_mul_clr, 1, 1, 1);
		glDrawElements(GL_TRIANGLES, scene->test_chunk.p_count, GL_UNSIGNED_INT, 0);
	}
	if (scene->fillmode == FILL_MODE_BOTH)
	{
		glLineWidth(scene->line_width);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform3f(scene->shader_mul_clr, 1.0f, 0.6f, 0);

		uint32_t safety_counter = 0;
		struct TetrahedronNode* next_node = scene->hierarchy.first_leaf;

		while (safety_counter++ < 1000000 && next_node)
		{
			if (next_node->p_count > 0)
			{
				glBindVertexArray(next_node->vao);
				glDrawElements(GL_TRIANGLES, next_node->p_count, GL_UNSIGNED_INT, 0);
			}
			next_node = next_node->next;
		}
	}

	glUseProgram(scene->outline_sp);
	FPSCamera_set_shader(&scene->camera, scene->outline_shader_projection, scene->outline_shader_view);
	glBindVertexArray(scene->hierarchy.outline_vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f(scene->outline_shader_mul_clr, 1, 0.9f, 0.1f);
	glPointSize(3.0f);
	//glDrawElements(GL_LINES, scene->hierarchy.outline_p_count, GL_UNSIGNED_INT, 0);

	glfwPollEvents();

	nk_glfw3_new_frame();
	if (nk_begin(scene->nkc, "Options", nk_rect(50, 50, 300, 330),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
		NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		char lbl[64];

		nk_layout_row_dynamic(scene->nkc, 90, 1);
		if (nk_group_begin(scene->nkc, "Results", 0))
		{
			nk_layout_row_dynamic(scene->nkc, 14, 1);
			char c_abbr = 0;
			int i_abbr = 0;
			abbreviate_int(scene->hierarchy.v_count * 24, &i_abbr, &c_abbr);
			sprintf(lbl, "Vertices: %i (%i%cB)", scene->hierarchy.v_count, i_abbr, c_abbr);
			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);

			abbreviate_int(scene->hierarchy.p_count * 4, &i_abbr, &c_abbr);
			sprintf(lbl, "Prims: %i (%i%cB)", scene->hierarchy.p_count / 3, i_abbr, c_abbr);

			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);
			sprintf(lbl, "Leaves: %i", scene->hierarchy.leaf_count);

			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);

			sprintf(lbl, "Time: %ims", scene->hierarchy.last_extract_time);
			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);

			nk_group_end(scene->nkc);
		}

		nk_layout_row_dynamic(scene->nkc, 74, 1);
		if (nk_group_begin(scene->nkc, "View", 0))
		{
			nk_layout_row_dynamic(scene->nkc, 14, 1);
			sprintf(lbl, "Speed: %g", scene->camera.speed);
			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);

			sprintf(lbl, "Pos: %.2f, %.2f, %.2f", scene->camera.position[0], scene->camera.position[1], scene->camera.position[2]);
			nk_label(scene->nkc, lbl, NK_TEXT_LEFT);

			nk_layout_row_dynamic(scene->nkc, 20, 2);
			nk_value_float(scene->nkc, "Line Width", scene->line_width);
			nk_slider_float(scene->nkc, 1.0f, &scene->line_width, 5.0f, 0.5f);

			nk_group_end(scene->nkc);
		}

		nk_layout_row_dynamic(scene->nkc, 64, 1);
		if (nk_group_begin(scene->nkc, "Snapping", 0))
		{
			nk_layout_row_dynamic(scene->nkc, 20, 1);
			scene->hierarchy.pem = nk_option_label(scene->nkc, "Enable Snapping", scene->hierarchy.pem);

			nk_layout_row_dynamic(scene->nkc, 20, 2);
			nk_value_float(scene->nkc, "Threshold", scene->hierarchy.snap_threshold);
			nk_slider_float(scene->nkc, 0.0f, &scene->hierarchy.snap_threshold, 0.707f, 0.025f);
			nk_group_end(scene->nkc);
		}

		nk_layout_row_dynamic(scene->nkc, 30, 1);
		if (nk_button_text(scene->nkc, "Extract all", 11))
		{
			THierarchy_extract_all_leaves(&scene->hierarchy);
		}
	}
	nk_end(scene->nkc);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
	glPopAttrib();

	glfwSwapBuffers(input->window);

	if (glfwGetKey(input->window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(input->window, 1);
	}

	return 0;
}

void abbreviate_int(int d, int* i_out, char* c_out)
{
	if (d > 1024 * 1024)
	{
		*c_out = 'M';
		*i_out = d / (1024 * 1024);
	}
	else if (d > 1024)
	{
		*c_out = 'K';
		*i_out = d / 1024;
	}
	else
	{
		*c_out = 'K';
		*i_out = 1;
	}
}
