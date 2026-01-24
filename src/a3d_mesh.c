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

bool a3d_mesh_upload(a3d* e, a3d_mesh* mesh,
	const a3d_vertex* v, Uint32 vcount,
	const Uint16* idx, Uint32 icount,
	a3d_topology topo
)
{
	if (!e) {
		A3D_LOG_ERROR("a3d_mesh_upload: engine is NULL");
		return false;
	}

	if (!e->gfx.v || !e->gfx.v->mesh_upload) {
		A3D_LOG_ERROR("backend does not support mesh_upload");
		return false;
	}

	return e->gfx.v->mesh_upload(e, mesh, v, vcount, idx, icount, topo);
}
