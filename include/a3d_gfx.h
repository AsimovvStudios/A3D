#pragma once

#include <stdbool.h>

typedef struct a3d a3d;
typedef struct a3d_mesh a3d_mesh;

typedef enum {
	A3D_BACKEND_VULKAN,
	A3D_BACKEND_OPENGL
} a3d_backend;

typedef struct a3d_gfx_vtbl {
	bool        (*init)(a3d* e);
	void        (*shutdown)(a3d* e);
	bool        (*draw_frame)(a3d* e);
	bool        (*recreate_or_resize)(a3d* e);
	void        (*set_clear_colour)(a3d* e, float r, float g, float b, float a);
	void        (*wait_idle)(a3d* e);
	/* mesh stff */
	bool        (*mesh_init_triangle)(a3d* e, a3d_mesh* mesh);
	void        (*mesh_destroy)(a3d* e, a3d_mesh* mesh);
} a3d_gfx_vtbl;

typedef struct a3d_gfx {
	const a3d_gfx_vtbl* v;  /* vtable */
	void*       ctx;        /* backend-specific context (TODO) */
} a3d_gfx;

