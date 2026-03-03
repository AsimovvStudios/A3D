#include <stdlib.h>

#include "a3d_renderer.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"

static int a3d_renderer_compare_draw_items(const void* lhs_ptr, const void* rhs_ptr);

bool a3d_renderer_draw_mesh(a3d_renderer* r, a3d_mesh_handle mesh, const a3d_mvp* mvp)
{
	return a3d_renderer_draw_mesh_material(
		r,
		mesh,
		mvp,
		A3D_ASSET_INVALID_HANDLE,
		A3D_ASSET_INVALID_HANDLE,
		A3D_ASSET_INVALID_HANDLE
	);
}

bool a3d_renderer_draw_mesh_material(
	a3d_renderer* r,
	a3d_mesh_handle mesh,
	const a3d_mvp* mvp,
	a3d_material_handle material,
	a3d_shader_handle sort_shader,
	a3d_texture_handle sort_texture
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

	if (!mvp || mesh == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("renderer_draw_mesh: bad args");
		return false;
	}

	r->items[r->count].mesh = mesh;
	r->items[r->count].material = material;
	r->items[r->count].mvp = *mvp;
	r->items[r->count].sort_shader = sort_shader;
	r->items[r->count].sort_texture = sort_texture;
	r->items[r->count].instance_offset = 0;
	r->items[r->count].instance_count = 0;
	r->count++;

	return true;
}

bool a3d_renderer_draw_mesh_material_instanced(
	a3d_renderer* r,
	a3d_mesh_handle mesh,
	a3d_material_handle material,
	a3d_shader_handle sort_shader,
	a3d_texture_handle sort_texture,
	const a3d_mvp* instances,
	Uint32 instance_count
)
{
	if (!r) {
		A3D_LOG_ERROR("renderer not initialised");
		return false;
	}

	if (!instances || instance_count == 0 || mesh == A3D_ASSET_INVALID_HANDLE) {
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
	r->items[r->count].sort_shader = sort_shader;
	r->items[r->count].sort_texture = sort_texture;
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

	if (lhs->sort_shader != rhs->sort_shader)
		return (lhs->sort_shader < rhs->sort_shader) ? -1 : 1;
	if (lhs->sort_texture != rhs->sort_texture)
		return (lhs->sort_texture < rhs->sort_texture) ? -1 : 1;
	if (lhs->mesh != rhs->mesh)
		return (lhs->mesh < rhs->mesh) ? -1 : 1;

	return 0;
}

