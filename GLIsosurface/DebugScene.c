#include <memory.h>
#include <cglm\cglm.h>

#include "DebugScene.h"
#include "Core.h"

#define FILL_MODE_FILL 0
#define FILL_MODE_BOTH 1
#define FILL_MODE_WIRE 2

int DebugScene_init(struct DebugScene* out, struct RenderInput* render_input)
{
	memset(out, 0, sizeof(struct DebugScene));
	out->fillmode = FILL_MODE_BOTH;

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
	FPSCamera_init(&out->camera, render_input->width, render_input->height, out->shader_projection, out->shader_view, render_input);

	UMC_Chunk_init(&out->test_chunk, 63, 1, 1);
	UMC_Chunk_run(&out->test_chunk);

	return 0;
}

int DebugScene_cleanup(struct DebugScene* scene)
{
	UMC_Chunk_destroy(&scene->test_chunk);
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
		UMC_Chunk_run(&scene->test_chunk);
	}

	glClearColor(0.02f, 0.02f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(scene->shader_program);
	FPSCamera_set_shader(&scene->camera, scene->shader_projection, scene->shader_view);
	glBindVertexArray(scene->test_chunk.vao);

	if (scene->fillmode == FILL_MODE_FILL || scene->fillmode == FILL_MODE_BOTH)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform3f(scene->shader_mul_clr, 0, 0.2f, 0.4f);
		glDrawElements(GL_TRIANGLES, scene->test_chunk.p_count, GL_UNSIGNED_INT, 0);
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
		glLineWidth(2.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUniform3f(scene->shader_mul_clr, 0, 0.75f, 0.9f);
		glDrawElements(GL_TRIANGLES, scene->test_chunk.p_count, GL_UNSIGNED_INT, 0);
	}

	glfwPollEvents();
	glfwSwapBuffers(input->window);

	if (glfwGetKey(input->window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(input->window, 1);
	}

	return 0;
}
