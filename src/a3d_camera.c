#include <math.h>
#include <string.h>

#include <cglm/cglm.h>

#include "a3d_camera.h"
#define A3D_LOG_TAG "CAM"
#include "a3d_logging.h"

static void a3d_perspective_vk(mat4 out, float fovy_rad, float aspect, float zn, float zf);

void a3d_camera_init(a3d_camera* c)
{
	if (!c)
		return;

	memset(c, 0, sizeof(*c));
	c->pos[0] = 0.0f;
	c->pos[1] = 0.0f;
	c->pos[2] = 3.0f;
	c->yaw = -(3.14159265358979323846f) / 2.0f;
	c->pitch = 0.0f;
	c->fov_deg = 70.0f;
	c->near_z = 0.1f;
	c->far_z = 100.0f;
	glm_mat4_identity(c->view);
	glm_mat4_identity(c->proj);
}

void a3d_camera_move_local(a3d_camera* c, float forward, float right, float up)
{
	if (!c)
		return;

	vec3 fwd;
	fwd[0] = cosf(c->yaw) * cosf(c->pitch);
	fwd[1] = sinf(c->pitch);
	fwd[2] = sinf(c->yaw) * cosf(c->pitch);

	{
		float l = sqrtf(fwd[0]*fwd[0] + fwd[1]*fwd[1] + fwd[2]*fwd[2]);
		if (l > 0.0f) {
			fwd[0] /= l;
			fwd[1] /= l;
			fwd[2] /= l;
		}
	}

	vec3 world_up = {0.0f, 1.0f, 0.0f};
	vec3 right_v;
	glm_vec3_cross(fwd, world_up, right_v);

	{
		float l = sqrtf(right_v[0]*right_v[0] + right_v[1]*right_v[1] + right_v[2]*right_v[2]);
		if (l > 0.0f) {
			right_v[0] /= l;
			right_v[1] /= l;
			right_v[2] /= l;
		}
	}

	vec3 move = {0.0f, 0.0f, 0.0f};
	vec3 tmp;

	glm_vec3_scale(fwd, forward, tmp);
	glm_vec3_add(move, tmp, move);

	glm_vec3_scale(right_v, right, tmp);
	glm_vec3_add(move, tmp, move);

	glm_vec3_scale(world_up, up, tmp);
	glm_vec3_add(move, tmp, move);

	glm_vec3_add(c->pos, move, c->pos);
}

void a3d_camera_rebuild_view(a3d_camera* c)
{
	if (!c)
		return;

	vec3 forward;
	forward[0] = cosf(c->yaw) * cosf(c->pitch);
	forward[1] = sinf(c->pitch);
	forward[2] = sinf(c->yaw) * cosf(c->pitch);

	vec3 target;
	glm_vec3_add(c->pos, forward, target);

	vec3 up = {0.0f, 1.0f, 0.0f};
	glm_lookat(c->pos, target, up, c->view);
}

void a3d_camera_set_perspective(a3d_camera* c, float aspect, a3d_backend backend)
{
	if (!c)
		return;

	if (backend == A3D_BACKEND_VULKAN)
		a3d_perspective_vk(c->proj, glm_rad(c->fov_deg), aspect, c->near_z, c->far_z);
	else
		glm_perspective(glm_rad(c->fov_deg), aspect, c->near_z, c->far_z, c->proj);
}

static void a3d_perspective_vk(mat4 out, float fovy_rad, float aspect, float zn, float zf)
{
	const float f = 1.0f / tanf(fovy_rad * 0.5f);

	glm_mat4_zero(out);

	out[0][0] = f / aspect;
	out[1][1] = -f;
	out[2][2] = zf / (zn - zf);
	out[2][3] = -1.0f;
	out[3][2] = (zn * zf) / (zn - zf);
}