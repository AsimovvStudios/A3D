#pragma once

#include <cglm/cglm.h>
#include "a3d_gfx.h"

typedef struct a3d_camera
{
	vec3 pos;
	float yaw;
	float pitch;
	float fov_deg;
	float near_z;
	float far_z;
	mat4 view;
	mat4 proj;
} a3d_camera;

void a3d_camera_init(a3d_camera* c);
void a3d_camera_move_local(a3d_camera* c, float forward, float right, float up);
void a3d_camera_rebuild_view(a3d_camera* c);
void a3d_camera_set_perspective(a3d_camera* c, float aspect, a3d_backend backend);
