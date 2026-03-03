#pragma once

#include <stdbool.h>
#include <SDL3/SDL_stdinc.h>

#include "a3d_handles.h"
#include "a3d_transform.h"

#define A3D_RENDERER_MAX_DRAW_CALLS 1024 /* change later */
#define A3D_RENDERER_MAX_INSTANCES 8192

typedef struct a3d_draw_item {
	a3d_mesh_handle mesh;
	a3d_material_handle material;
	a3d_mvp     mvp;
	a3d_shader_handle sort_shader;
	a3d_texture_handle sort_texture;
	Uint32      instance_offset;
	Uint32      instance_count;
} a3d_draw_item;

struct a3d_renderer {
	a3d_draw_item items[A3D_RENDERER_MAX_DRAW_CALLS];
	mat4        instance_mvp_pool[A3D_RENDERER_MAX_INSTANCES];
	Uint32      count;
	Uint32      instance_mvp_count;
	bool        frame_active;
};

bool a3d_renderer_draw_mesh(a3d_renderer* r, a3d_mesh_handle mesh, const a3d_mvp* mvp);
bool a3d_renderer_draw_mesh_material(
	a3d_renderer* r,
	a3d_mesh_handle mesh,
	const a3d_mvp* mvp,
	a3d_material_handle material,
	a3d_shader_handle sort_shader,
	a3d_texture_handle sort_texture
);
bool a3d_renderer_draw_mesh_material_instanced(
	a3d_renderer* r,
	a3d_mesh_handle mesh,
	a3d_material_handle material,
	a3d_shader_handle sort_shader,
	a3d_texture_handle sort_texture,
	const a3d_mvp* instances,
	Uint32 instance_count
);
void a3d_renderer_frame_begin(a3d_renderer* r);
void a3d_renderer_frame_end(a3d_renderer* r);
void a3d_renderer_get_draw_items(a3d_renderer* r, const a3d_draw_item** out_items, Uint32* out_count);
bool a3d_renderer_init(a3d_renderer* r);
void a3d_renderer_shutdown(a3d_renderer* r);

