#include <stdlib.h>

#include "a3d_renderer.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"


bool a3d_renderer_draw_mesh(a3d_renderer* r, const a3d_mesh* mesh, const a3d_mvp* mvp)
{

	if (!r) {
		A3D_LOG_ERROR("renderer not initialised");
		return false;
	}

	if (!r->frame_active)
		A3D_LOG_WARN("a3d_renderer_draw_mesh called outside begin/end_frame");

	if (r->count >= A3D_RENDERER_MAX_DRAW_CALLS) {
		A3D_LOG_WARN("renderer queue full; dropping draw call");
		return false;
	}

	if (!mesh || !mvp) {
		A3D_LOG_ERROR("renderer_draw_mesh: bad args");
		return false;
	}

	r->items[r->count].mesh = mesh;
	r->items[r->count].mvp = *mvp;
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
	r->frame_active = true;
}

void a3d_renderer_frame_end(a3d_renderer* r)
{
	if (!r) {
		A3D_LOG_ERROR("a3d_renderer_frame_end called without renderer");
		return;
	}

	r->frame_active = false;
}

void a3d_renderer_get_draw_items(a3d_renderer* r, const a3d_draw_item** out_items, Uint32* out_count)
{
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
