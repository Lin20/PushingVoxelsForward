#include "Camera.h"
#include "Util.h"
#include "Core.h"

#define UPDATE_UNITS (1.0f / 60.0f)
const float Smoothness = 0.85f;
const float RotationSpeed = 0.001f;

void FPSCamera_init(struct FPSCamera* camera, uint32_t width, uint32_t height, GLuint shader_proj, GLuint shader_view, struct RenderInput* render_input)
{
	camera->last_1 = 0;
	camera->last_2 = 0;
	camera->lock_cursor = 0;
	camera->last_mb = 0;
	camera->speed = 0.1f;
	vec3_set(camera->position, 0, 0, -3);
	vec3_set(camera->rot, 0, 0, 0);
	vec3_zero(camera->velocity);
	vec3_zero(camera->turn_velocity);
	glm_euler(camera->rot, camera->m_rotation);
	glfwSetCursorPos(render_input->window, render_input->width * 0.5, render_input->height * 0.5);
	glfwGetCursorPos(render_input->window, &camera->last_x, &camera->last_y);

	glm_perspective(PI / 3.0f, (float)width / (float)height, 0.004f, 1000.0f, camera->projection);
	FPSCamera_update(camera, render_input);
	FPSCamera_set_shader(camera, shader_proj, shader_view);
}

void FPSCamera_update(struct FPSCamera* camera, struct RenderInput* render_input)
{
	//render_input->delta = 1.0f / 120.0f;
	/*float percent = 1.0f - fmaxf(0.0f, render_input->delta / UPDATE_UNITS);
	float speed = camera->speed * render_input->delta * UPDATE_UNITS;
	float delta_smoothness = Smoothness + (1.0f - Smoothness) * percent;*/

	if (glfwGetKey(render_input->window, GLFW_KEY_1) && !camera->last_1)
		camera->speed *= 2.0f;
	if (glfwGetKey(render_input->window, GLFW_KEY_2) && !camera->last_2)
		camera->speed *= 0.5f;
	camera->last_1 = glfwGetKey(render_input->window, GLFW_KEY_1);
	camera->last_2 = glfwGetKey(render_input->window, GLFW_KEY_2);

	if (glfwGetMouseButton(render_input->window, GLFW_MOUSE_BUTTON_MIDDLE) && !camera->last_mb)
		camera->lock_cursor = !camera->lock_cursor;
	camera->last_mb = glfwGetMouseButton(render_input->window, GLFW_MOUSE_BUTTON_MIDDLE);

	float speed = camera->speed;
	float delta_smoothness = Smoothness;
	if (glfwGetKey(render_input->window, GLFW_KEY_LEFT_SHIFT))
		speed *= 5.0f;

	// Smooth camera support only
	vec3 temp_move;
	if (glfwGetKey(render_input->window, GLFW_KEY_W))
	{
		vec3_set(temp_move, 0.0f, 0.0f, 1.0f * speed);
		glm_mat4_mulv3(camera->m_rotation, temp_move, temp_move);
		glm_vec_add(camera->velocity, temp_move, camera->velocity);
	}
	if (glfwGetKey(render_input->window, GLFW_KEY_S))
	{
		vec3_set(temp_move, 0.0f, 0.0f, -1.0f * speed);
		glm_mat4_mulv3(camera->m_rotation, temp_move, temp_move);
		glm_vec_add(camera->velocity, temp_move, camera->velocity);
	}
	if (glfwGetKey(render_input->window, GLFW_KEY_D))
	{
		vec3_set(temp_move, -1.0f * speed, 0.0f, 0.0f);
		glm_mat4_mulv3(camera->m_rotation, temp_move, temp_move);
		glm_vec_add(camera->velocity, temp_move, camera->velocity);
	}
	if (glfwGetKey(render_input->window, GLFW_KEY_A))
	{
		vec3_set(temp_move, 1.0f * speed, 0.0f, 0.0f);
		glm_mat4_mulv3(camera->m_rotation, temp_move, temp_move);
		glm_vec_add(camera->velocity, temp_move, camera->velocity);
	}

	glm_vec_add(camera->position, camera->velocity, camera->position);
	glm_vec_mulv(camera->velocity, (vec3) {
		delta_smoothness, delta_smoothness, delta_smoothness
	}, camera->velocity);

	// Rotate the camera
	float dx, dy;
	if (glfwGetWindowAttrib(render_input->window, GLFW_FOCUSED) && (camera->lock_cursor || glfwGetMouseButton(render_input->window, GLFW_MOUSE_BUTTON_RIGHT) || glfwGetKey(render_input->window, GLFW_KEY_LEFT_CONTROL)))
	{
		glfwSetInputMode(render_input->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		double current_x, current_y;
		glfwGetCursorPos(render_input->window, &current_x, &current_y);
		if (camera->lock_cursor)
			glfwSetCursorPos(render_input->window, render_input->width * 0.5, render_input->height * 0.5);
		else
			glfwSetCursorPos(render_input->window, camera->last_x, camera->last_y);
		dx = (float)(current_x - camera->last_x);
		dy = (float)(current_y - camera->last_y);
		glfwGetCursorPos(render_input->window, &camera->last_x, &camera->last_y);
	}
	else
	{
		dx = 0;
		dy = 0;
		glfwGetCursorPos(render_input->window, &camera->last_x, &camera->last_y);
		glfwSetInputMode(render_input->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	camera->turn_velocity[0] += RotationSpeed * dy;
	camera->turn_velocity[1] -= RotationSpeed * dx;
	glm_vec_add(camera->rot, camera->turn_velocity, camera->rot);
	camera->turn_velocity[0] *= Smoothness;
	camera->turn_velocity[1] *= Smoothness;


	// Update the camera's transformation matrices
	vec3 target, up;

	glm_euler(camera->rot, camera->m_rotation);

	// Determine target
	vec3_forward(temp_move);
	glm_mat4_mulv3(camera->m_rotation, temp_move, temp_move);
	glm_vec_add(camera->position, temp_move, target);

	// Determine up
	vec3_up(temp_move);
	glm_mat4_mulv3(camera->m_rotation, temp_move, up);

	glm_lookat(camera->position, target, up, camera->view);
}

void FPSCamera_set_shader(struct FPSCamera* camera, GLuint shader_proj, GLuint shader_view)
{
	glUniformMatrix4fv(shader_proj, 1, GL_FALSE, camera->projection);
	glUniformMatrix4fv(shader_view, 1, GL_FALSE, camera->view);
}


