#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

#include "a3d_gfx.h"

typedef struct a3d a3d;
typedef struct a3d_mesh a3d_mesh;
typedef struct a3d_texture a3d_texture;

void a3d_gl_destroy_mesh(a3d* e, a3d_mesh* mesh);
bool a3d_gl_draw_frame(a3d* e);
bool a3d_gl_init(a3d* e);
bool a3d_gl_init_triangle(a3d* e, a3d_mesh* mesh);
bool a3d_gl_mesh_upload(a3d* e, a3d_mesh* mesh,
	const a3d_vertex* vertices, Uint32 vertex_count,
	const a3d_index* indices, Uint32 index_count,
	a3d_topology topology
);
bool a3d_gl_pre_window(a3d* e, SDL_WindowFlags* flags);
bool a3d_gl_resize(a3d* e);
void a3d_gl_set_clear_colour(a3d* e, float r, float g, float b, float a);
bool a3d_gl_texture_load(a3d* e, a3d_texture* texture, const char* path, bool srgb);
void a3d_gl_texture_destroy(a3d* e, a3d_texture* texture);
void a3d_gl_shutdown(a3d* e);
void a3d_gl_wait_idle(a3d* e);

