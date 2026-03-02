#include <stdlib.h>
#include <stdint.h>

#include "a3d_renderer.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"

static int a3d_renderer_compare_draw_items(const void* lhs_ptr, const void* rhs_ptr);

bool a3d_renderer_draw_mesh(a3d_renderer* r, const a3d_mesh* mesh, const a3d_mvp* mvp)
{
	return a3d_renderer_draw_mesh_material(r, mesh, mvp, NULL);
}

bool a3d_renderer_draw_mesh_material(
	a3d_renderer* r,
	const a3d_mesh* mesh,
	const a3d_mvp* mvp,
	const a3d_material* material
)
{
	if (!r) {
		A3D_LOG_ERROR("renderer not initialised");
		return false;
	}

	if (!r->frame_active) {
		A3D_LOG_WARN("a3d_renderer_draw_mesh called outside begin/end_frame");
		return false;
	}

	if (r->count >= A3D_RENDERER_MAX_DRAW_CALLS) {
		A3D_LOG_WARN("renderer queue full; dropping draw call");
		return false;
	}

	if (!mesh || !mvp) {
		A3D_LOG_ERROR("renderer_draw_mesh: bad args");
		return false;
	}

	r->items[r->count].mesh = mesh;
	r->items[r->count].material = material;
	r->items[r->count].mvp = *mvp;
	r->items[r->count].instance_offset = 0;
	r->items[r->count].instance_count = 0;
	r->count++;

	return true;
}

bool a3d_renderer_draw_mesh_material_instanced(
	a3d_renderer* r,
	const a3d_mesh* mesh,
	const a3d_material* material,
	const a3d_mvp* instances,
	Uint32 instance_count
)
{
	if (!r) {
		A3D_LOG_ERROR("renderer not initialised");
		return false;
	}

	if (!mesh || !instances || instance_count == 0) {
		A3D_LOG_ERROR("a3d_renderer_draw_mesh_material_instanced: bad args");
		return false;
	}

	if (!r->frame_active) {
		A3D_LOG_WARN("a3d_renderer_draw_mesh_material_instanced called outside begin/end_frame");
		return false;
	}

	if (r->count >= A3D_RENDERER_MAX_DRAW_CALLS) {
		A3D_LOG_WARN("renderer queue full; dropping instanced draw call");
		return false;
	}

	if (r->instance_mvp_count + instance_count > A3D_RENDERER_MAX_INSTANCES) {
		A3D_LOG_WARN("renderer instance pool full; dropping instanced draw call");
		return false;
	}

	Uint32 offset = r->instance_mvp_count;
	for (Uint32 i = 0; i < instance_count; i++)
		a3d_mvp_compose(r->instance_mvp_pool[offset + i], &instances[i]);

	r->instance_mvp_count += instance_count;
	r->items[r->count].mesh = mesh;
	r->items[r->count].material = material;
	r->items[r->count].mvp = instances[0];
	r->items[r->count].instance_offset = offset;
	r->items[r->count].instance_count = instance_count;
	r->count++;

	return true;
}

void a3d_renderer_frame_begin(a3d_renderer* r)
{
	if (!r) {
		A3D_LOG_ERROR("a3d_renderer_frame_begin called without renderer");
		return;
	}

	r->count = 0;
	r->instance_mvp_count = 0;
	r->frame_active = true;
}

void a3d_renderer_frame_end(a3d_renderer* r)
{
	if (!r) {
		A3D_LOG_ERROR("a3d_renderer_frame_end called without renderer");
		return;
	}

	if (r->count > 1)
		qsort(r->items, r->count, sizeof(r->items[0]), a3d_renderer_compare_draw_items);

	r->frame_active = false;
}

void a3d_renderer_get_draw_items(a3d_renderer* r, const a3d_draw_item** out_items, Uint32* out_count)
{
	if (!out_items || !out_count)
		return;

	if (!r) {
		*out_items = NULL;
		*out_count = 0;
		return;
	}

	*out_items = r->items;
	*out_count = r->count;
}

bool a3d_renderer_init(a3d_renderer* r)
{
	if (!r) {
		A3D_LOG_ERROR("a3d_renderer_init called with NULL");
		return false;
	}

	A3D_LOG_INFO("initialising renderer");

	r->count = 0;
	r->instance_mvp_count = 0;
	r->frame_active = false;

	A3D_LOG_INFO("initialised renderer");
	return true;
}

void a3d_renderer_shutdown(a3d_renderer* r)
{
	if (!r)
		return;

	A3D_LOG_INFO("shutting down renderer");
}

static int a3d_renderer_compare_draw_items(const void* lhs_ptr, const void* rhs_ptr)
{
	const a3d_draw_item* lhs = lhs_ptr;
	const a3d_draw_item* rhs = rhs_ptr;
	unsigned int lhs_shader = 0;
	unsigned int rhs_shader = 0;
	const a3d_texture* lhs_texture = NULL;
	const a3d_texture* rhs_texture = NULL;
	const a3d_material* lhs_material = lhs->material;
	const a3d_material* rhs_material = rhs->material;

	/* items with the same shader and texture are grouped together */
	if (lhs->material) {
		lhs_shader = lhs->material->shader;
		lhs_texture = lhs->material->albedo;
	}
	if (rhs->material) {
		rhs_shader = rhs->material->shader;
		rhs_texture = rhs->material->albedo;
	}

	/* sort by shader, material, texture, then mesh pointer */
	if (lhs_shader != rhs_shader)
		return (lhs_shader < rhs_shader) ? -1 : 1;
	if (lhs_material != rhs_material)
		return ((uintptr_t)lhs_material < (uintptr_t)rhs_material) ? -1 : 1;
	if (lhs_texture != rhs_texture)
		return ((uintptr_t)lhs_texture < (uintptr_t)rhs_texture) ? -1 : 1;

	if (lhs->mesh != rhs->mesh)
		return ((uintptr_t)lhs->mesh < (uintptr_t)rhs->mesh) ? -1 : 1;

	return 0;
}

