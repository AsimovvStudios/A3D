#include <SDL3/SDL_stdinc.h>

#include "a3d.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_mesh.h"

void a3d_destroy_mesh(a3d* e, a3d_mesh* mesh)
{
	if (e && e->gfx.v && e->gfx.v->mesh_destroy)
		e->gfx.v->mesh_destroy(e, mesh);
}

bool a3d_init_triangle(a3d* e, a3d_mesh* mesh)
{
	if (e && e->gfx.v && e->gfx.v->mesh_init_triangle)
		return e->gfx.v->mesh_init_triangle(e, mesh);

	A3D_LOG_ERROR("no backend initialised for mesh creation");
	return false;
}
