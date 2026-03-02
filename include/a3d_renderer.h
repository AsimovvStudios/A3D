#pragma once

#include <stdbool.h>
#include <SDL3/SDL_stdinc.h>

#include "a3d_material.h"
#include "a3d_mesh.h"
#include "a3d_transform.h"

#define A3D_RENDERER_MAX_DRAW_CALLS 1024 /* change later */
#define A3D_RENDERER_MAX_INSTANCES 8192

typedef struct a3d_draw_item {
	const a3d_mesh* mesh;
	const a3d_material* material;
	a3d_mvp     mvp;
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

bool a3d_renderer_draw_mesh(a3d_renderer* r, const a3d_mesh* mesh, const a3d_mvp* mvp);
bool a3d_renderer_draw_mesh_material(
	a3d_renderer* r,
	const a3d_mesh* mesh,
	const a3d_mvp* mvp,
	const a3d_material* material
);
bool a3d_renderer_draw_mesh_material_instanced(
	a3d_renderer* r,
	const a3d_mesh* mesh,
	const a3d_material* material,
	const a3d_mvp* instances,
	Uint32 instance_count
);
void a3d_renderer_frame_begin(a3d_renderer* r);
void a3d_renderer_frame_end(a3d_renderer* r);
void a3d_renderer_get_draw_items(a3d_renderer* r, const a3d_draw_item** out_items, Uint32* out_count);
bool a3d_renderer_init(a3d_renderer* r);
void a3d_renderer_shutdown(a3d_renderer* r);

