#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_assets.h"
#include "a3d_event.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_window.h"
#include "a3d_renderer.h"

/* backends */
#if defined(BACKEND_VK)
#include <SDL3/SDL_vulkan.h>
#include "vulkan/a3d_vulkan.h"
#endif
#if defined(BACKEND_GL)
#include "opengl/a3d_opengl.h"
#endif

#if defined(BACKEND_VK)
extern const a3d_gfx_vtbl a3d_vk_vtbl;
#endif
#if defined(BACKEND_GL)
extern const a3d_gfx_vtbl a3d_gl_vtbl;
#endif

float a3d_dt(const a3d* e)
{
	if (!e)
		return 0.0f;
	return e->dt;
}

void a3d_frame(a3d* e)
{
	if (!e)
		return;

	/* timing */
	if (e->last_ticks) {
		Uint64 now = SDL_GetTicksNS();
		Uint64 delta = now - e->last_ticks;
		/* ns->seconds */
		e->dt = (float)delta / 1e9f;
		/* clamping avoids jumps */
		if (e->dt > 0.1f) e->dt = 0.1f;
		e->last_ticks = now;
	}
	else {
		e->dt = 0.0f;
		e->last_ticks = SDL_GetTicksNS();
	}

	/* input */
	a3d_event_pump(e);

	if (!e->running)
		return;

	/* handle resize */
	if (e->fb_resized) {
		if (e->gfx.v && e->gfx.v->recreate_or_resize)
			e->gfx.v->recreate_or_resize(e);
		e->fb_resized = false;
	}

	/* render */
	if (e->gfx.v && e->gfx.v->draw_frame)
		e->gfx.v->draw_frame(e);
}

void a3d_frame_begin(a3d* e)
{
	if (!e) {
		A3D_LOG_ERROR("a3d_frame_begin: engine is NULL");
		return;
	}

	if (!e->renderer) {
		A3D_LOG_ERROR("a3d_frame_begin: renderer is NULL");
		return;
	}

	a3d_renderer_frame_begin(e->renderer);
}

void a3d_frame_end(a3d* e)
{
	if (!e) {
		A3D_LOG_ERROR("a3d_frame_end: engine is NULL");
		return;
	}

	if (!e->renderer) {
		A3D_LOG_ERROR("a3d_frame_end: renderer is NULL");
		return;
	}

	a3d_renderer_frame_end(e->renderer);
}

bool a3d_init(a3d* e, const char* title, int width, int height)
{
	return a3d_init_backend(e, A3D_BACKEND, title, width, height);
}

bool a3d_init_backend(a3d* e, a3d_backend backend, const char* title, int width, int height)
{
	/* zero engine */
	memset(e, 0, sizeof(*e));

	e->backend = backend;

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		A3D_LOG_ERROR("failed to init SDL: %s", SDL_GetError());
		return false;
	}

	/* choose window flags */
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
	switch (backend) {
#if defined(BACKEND_VK)
	case A3D_BACKEND_VULKAN:
		window_flags |= SDL_WINDOW_VULKAN;
		e->gfx.v = &a3d_vk_vtbl;
		A3D_LOG_INFO("using Vulkan backend");
		break;
#endif
#if defined(BACKEND_GL)
	case A3D_BACKEND_OPENGL:
		window_flags |= SDL_WINDOW_OPENGL;
		e->gfx.v = &a3d_gl_vtbl;
		A3D_LOG_INFO("using OpenGL backend");
		break;
#endif
	default:
		A3D_LOG_ERROR("unsupported backend: %d", backend);
		SDL_Quit();
		return false;
	}

	if (e->gfx.v && e->gfx.v->pre_window) {
		if (!e->gfx.v->pre_window(e, &window_flags)) {
			A3D_LOG_ERROR("backend pre-window setup failed");
			SDL_Quit();
			return false;
		}
	}

	e->window = a3d_create_window(title, width, height, window_flags);
	if (!e->window) {
		A3D_LOG_ERROR("failed to create window");
		SDL_Quit();
		return false;
	}

	/* avoid crash if window is minimised at launch */
	width = height = 0;
	while (width == 0 || height == 0) {
		SDL_GetWindowSize(e->window, &width, &height);
		SDL_Delay(50);
	}

	/* init graphics backend */
	if (!e->gfx.v->init(e)) {
		A3D_LOG_ERROR("graphics backend initialisation failed");
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	/* init renderer */
	e->renderer = malloc(sizeof *e->renderer);
	if (!e->renderer) {
		A3D_LOG_ERROR("failed to allocate renderer");
		if (e->gfx.v && e->gfx.v->shutdown)
			e->gfx.v->shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	if (!a3d_renderer_init(e->renderer)) {
		A3D_LOG_ERROR("renderer initialisation failed");
		free(e->renderer);
		e->renderer = NULL;
		if (e->gfx.v && e->gfx.v->shutdown)
			e->gfx.v->shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	e->assets = malloc(sizeof(*e->assets));
	if (!e->assets) {
		A3D_LOG_ERROR("failed to allocate asset manager");
		a3d_renderer_shutdown(e->renderer);
		free(e->renderer);
		e->renderer = NULL;
		if (e->gfx.v && e->gfx.v->shutdown)
			e->gfx.v->shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	if (!a3d_assets_init(e->assets)) {
		A3D_LOG_ERROR("asset manager initialisation failed");
		free(e->assets);
		e->assets = NULL;
		a3d_renderer_shutdown(e->renderer);
		free(e->renderer);
		e->renderer = NULL;
		if (e->gfx.v && e->gfx.v->shutdown)
			e->gfx.v->shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	e->running = true;
	e->handlers_count = 0;

	/* initialise timing state */
	e->last_ticks = SDL_GetTicksNS();
	e->dt = 0.0f;

	/* sane defaults */
	a3d_event_add_handler(e, SDL_EVENT_QUIT, a3d_event_on_quit);
	a3d_event_add_handler(e, SDL_EVENT_WINDOW_CLOSE_REQUESTED, a3d_event_on_close_requested);
	a3d_event_add_handler(e, SDL_EVENT_WINDOW_RESIZED, a3d_event_on_resize);
	a3d_event_add_handler(e, SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED, a3d_event_on_resize);

#if defined(BACKEND_GL)
	if (e->backend == A3D_BACKEND_OPENGL &&
		a3d_assets_register_shader(e->assets, "default", e->gl.program) == A3D_ASSET_INVALID_HANDLE) {
		A3D_LOG_ERROR("failed to register default shader in asset manager");
	}
#endif

	return true;
}

void a3d_quit(a3d *e)
{
	if (e->assets) {
		a3d_assets_shutdown(e, e->assets);
		free(e->assets);
		e->assets = NULL;
	}

	if (e->renderer) {
		a3d_renderer_shutdown(e->renderer);
		free(e->renderer);
		e->renderer = NULL;
	}

	if (e->gfx.v && e->gfx.v->shutdown)
		e->gfx.v->shutdown(e);

	SDL_DestroyWindow(e->window);
	SDL_Quit();
}

void a3d_set_clear_colour(a3d* e, float r, float g, float b, float a)
{
	if (e && e->gfx.v && e->gfx.v->set_clear_colour)
		e->gfx.v->set_clear_colour(e, r, g, b, a);
}

bool a3d_submit_mesh(a3d* e, const a3d_mesh* mesh, const a3d_mvp* mvp)
{
	/* TODO */
	return a3d_submit_mesh_material(e, mesh, mvp, NULL);
}

bool a3d_submit_mesh_material(
	a3d* e,
	const a3d_mesh* mesh,
	const a3d_mvp* mvp,
	const a3d_material* material
)
{
	if (!e || !e->renderer)
		return false;

	return a3d_renderer_draw_mesh_material(e->renderer, mesh, mvp, material);
}

bool a3d_draw_instanced(
	a3d* e,
	const a3d_mesh* mesh,
	const a3d_material* material,
	const a3d_mvp* instances,
	Uint32 instance_count
)
{
	if (!e || !e->renderer)
		return false;

	return a3d_renderer_draw_mesh_material_instanced(
		e->renderer,
		mesh,
		material,
		instances,
		instance_count
	);
}

a3d_assets* a3d_get_assets(a3d* e)
{
	if (!e)
		return NULL;

	return e->assets;
}

const a3d_assets* a3d_get_assets_const(const a3d* e)
{
	if (!e)
		return NULL;

	return e->assets;
}

void a3d_wait_idle(a3d* e)
{
	if (e && e->gfx.v && e->gfx.v->wait_idle)
		e->gfx.v->wait_idle(e);
}

