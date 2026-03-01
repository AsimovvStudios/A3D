#pragma once

#include <stdbool.h>
#include <SDL3/SDL.h>

typedef struct a3d a3d;
typedef struct a3d_mesh a3d_mesh;
typedef struct a3d_vertex a3d_vertex;
typedef struct a3d_texture a3d_texture;
typedef Uint32 a3d_index;

typedef enum {
	A3D_TOPO_TRIANGLES
} a3d_topology;

typedef enum {
	A3D_BACKEND_VULKAN,
	A3D_BACKEND_OPENGL
} a3d_backend;

typedef struct a3d_gfx_vtbl {
	bool        (*pre_window)(a3d* e, SDL_WindowFlags* flags);
	bool        (*init)(a3d* e);
	void        (*shutdown)(a3d* e);
	bool        (*draw_frame)(a3d* e);
	bool        (*recreate_or_resize)(a3d* e);
	void        (*set_clear_colour)(a3d* e, float r, float g, float b, float a);
	void        (*wait_idle)(a3d* e);
	bool        (*texture_load)(a3d* e, a3d_texture* texture, const char* path, bool srgb);
	void        (*texture_destroy)(a3d* e, a3d_texture* texture);
	/* mesh stff */
	bool        (*mesh_upload)(
		a3d* e,
		a3d_mesh* mesh,
		const a3d_vertex* vertices,
		Uint32 vertex_count,
		const a3d_index* indices,
		Uint32 index_count,
		a3d_topology topology
	);
	bool        (*mesh_init_triangle)(a3d* e, a3d_mesh* mesh);
	void        (*mesh_destroy)(a3d* e, a3d_mesh* mesh);
} a3d_gfx_vtbl;

typedef struct a3d_gfx {
	const a3d_gfx_vtbl* v;
	void*       ctx;
} a3d_gfx;

