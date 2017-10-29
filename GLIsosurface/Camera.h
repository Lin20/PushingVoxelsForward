#pragma once

#include <gl/glew.h>
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <cglm\cglm.h>

struct FPSCamera
{
	int last_1 : 1;
	int last_2 : 2;
	float speed;
	vec3 position;
	vec3 rot;
	vec3 velocity;
	vec3 turn_velocity;

	double last_x;
	double last_y;
	int last_modifiers[3];

	mat4 projection;
	mat4 view;
	mat4 m_rotation;
};

void FPSCamera_init(struct FPSCamera* camera, uint32_t width, uint32_t height, GLuint shader_proj, GLuint shader_view, struct RenderInput* render_input);
void FPSCamera_update(struct FPSCamera* camera, struct RenderInput* render_input);
void FPSCamera_set_shader(struct FPSCamera* camera, GLuint shader_proj, GLuint shader_view);
