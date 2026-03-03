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

static void a3d_default_handles(
    const a3d* e, a3d_material_handle* out_material, a3d_shader_handle* out_shader, a3d_texture_handle* out_texture
)
{
	if (out_material)
		*out_material = A3D_ASSET_INVALID_HANDLE;
	if (out_shader)
		*out_shader = A3D_ASSET_INVALID_HANDLE;
	if (out_texture)
		*out_texture = A3D_ASSET_INVALID_HANDLE;

#if defined(BACKEND_GL)
	if (e && e->backend == A3D_BACKEND_OPENGL)
	{
		if (out_material)
			*out_material = e->gl.default_material;
		if (out_shader)
			*out_shader = e->gl.default_shader;
		if (out_texture)
			*out_texture = e->gl.default_texture;
	}
#else
	(void)e;
#endif
}

static void a3d_resolve_material_keys(
    const a3d* e,
    a3d_material_handle requested_material,
    a3d_material_handle* out_material,
    a3d_shader_handle* out_shader,
    a3d_texture_handle* out_texture
)
{
	/* defaults */
	a3d_material_handle fallback_material = A3D_ASSET_INVALID_HANDLE;
	a3d_shader_handle fallback_shader = A3D_ASSET_INVALID_HANDLE;
	a3d_texture_handle fallback_texture = A3D_ASSET_INVALID_HANDLE;
	a3d_default_handles(e, &fallback_material, &fallback_shader, &fallback_texture);

	/* resolve material */
	a3d_material_handle resolved_material = requested_material;
	if (resolved_material == A3D_ASSET_INVALID_HANDLE)
		resolved_material = fallback_material;

	/* resolve shader, texture from mat (fallback if invalid) */
	const a3d_material* material = NULL;
	if (e && e->assets && resolved_material != A3D_ASSET_INVALID_HANDLE)
		material = a3d_assets_get_material(e->assets, resolved_material);
	if (!material && e && e->assets && fallback_material != A3D_ASSET_INVALID_HANDLE)
	{
		resolved_material = fallback_material;
		material = a3d_assets_get_material(e->assets, resolved_material);
	}

	a3d_shader_handle shader = fallback_shader;
	a3d_texture_handle texture = fallback_texture;
	if (material)
	{
		if (material->shader != A3D_ASSET_INVALID_HANDLE)
			shader = material->shader;
		if (material->albedo != A3D_ASSET_INVALID_HANDLE)
			texture = material->albedo;
	}

	if (out_material)
		*out_material = resolved_material;
	if (out_shader)
		*out_shader = shader;
	if (out_texture)
		*out_texture = texture;
}

static void a3d_update_timing(a3d* e)
{
	if (!e)
		return;

	if (e->last_ticks)
	{
		Uint64 now = SDL_GetTicksNS();
		Uint64 delta = now - e->last_ticks;
		/* ns->seconds */
		e->dt = (float)delta / 1e9f;
		/* clamp extreme spikes */
		if (e->dt > 0.1f)
			e->dt = 0.1f;
		e->last_ticks = now;
	}
	else
	{
		e->dt = 0.0f;
		e->last_ticks = SDL_GetTicksNS();
	}
}

static bool a3d_prepare_frame(a3d* e)
{
	if (!e)
		return false;

	/* clear per-frame input transitions before polling new events */
	a3d_input_begin_frame(&e->input);
	a3d_event_pump(e);

	if (!e->running)
		return false;

	if (e->fb_resized)
	{
		if (e->gfx.v && e->gfx.v->recreate_or_resize)
			e->gfx.v->recreate_or_resize(e);
		e->fb_resized = false;
	}

	return true;
}

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

	a3d_update_timing(e);
	if (!a3d_prepare_frame(e))
		return;

	/* render */
	if (e->gfx.v && e->gfx.v->draw_frame)
		e->gfx.v->draw_frame(e);
}

void a3d_run(a3d* e, a3d_update_fn tick, void* user)
{
	if (!e)
	{
		A3D_LOG_ERROR("a3d_run: engine is NULL");
		return;
	}

	while (e->running)
	{
		a3d_update_timing(e);
		if (!a3d_prepare_frame(e))
			continue;

		a3d_frame_begin(e);
		if (tick)
			tick(e, user);
		a3d_frame_end(e);

		if (!e->running)
			continue;

		if (e->gfx.v && e->gfx.v->draw_frame)
			e->gfx.v->draw_frame(e);
	}
}

void a3d_frame_begin(a3d* e)
{
	if (!e)
	{
		A3D_LOG_ERROR("a3d_frame_begin: engine is NULL");
		return;
	}

	if (!e->renderer)
	{
		A3D_LOG_ERROR("a3d_frame_begin: renderer is NULL");
		return;
	}

	a3d_renderer_frame_begin(e->renderer);
}

void a3d_frame_end(a3d* e)
{
	if (!e)
	{
		A3D_LOG_ERROR("a3d_frame_end: engine is NULL");
		return;
	}

	if (!e->renderer)
	{
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
	a3d_input_init(&e->input);

	e->backend = backend;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		A3D_LOG_ERROR("failed to init SDL: %s", SDL_GetError());
		return false;
	}

	/* choose window flags */
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
	switch (backend)
	{
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

	if (e->gfx.v && e->gfx.v->pre_window)
	{
		if (!e->gfx.v->pre_window(e, &window_flags))
		{
			A3D_LOG_ERROR("backend pre-window setup failed");
			SDL_Quit();
			return false;
		}
	}

	e->window = a3d_create_window(title, width, height, window_flags);
	if (!e->window)
	{
		A3D_LOG_ERROR("failed to create window");
		SDL_Quit();
		return false;
	}

	/* avoid crash if window is minimised at launch */
	width = height = 0;
	while (width == 0 || height == 0)
	{
		SDL_GetWindowSize(e->window, &width, &height);
		SDL_Delay(50);
	}

	/* init graphics backend */
	if (!e->gfx.v->init(e))
	{
		A3D_LOG_ERROR("graphics backend initialisation failed");
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	/* init renderer */
	e->renderer = malloc(sizeof *e->renderer);
	if (!e->renderer)
	{
		A3D_LOG_ERROR("failed to allocate renderer");
		if (e->gfx.v && e->gfx.v->shutdown)
			e->gfx.v->shutdown(e);
		SDL_DestroyWindow(e->window);
		SDL_Quit();
		return false;
	}

	if (!a3d_renderer_init(e->renderer))
	{
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
	if (!e->assets)
	{
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

	if (!a3d_assets_init(e->assets))
	{
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
	if (e->backend == A3D_BACKEND_OPENGL)
	{
		const float white_tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		e->gl.default_shader = a3d_assets_register_shader(e->assets, "default", e->gl.program);
		if (e->gl.default_shader == A3D_ASSET_INVALID_HANDLE)
		{
			A3D_LOG_ERROR("failed to register default shader in asset manager");
			a3d_quit(e);
			return false;
		}
		e->gl.default_texture = a3d_assets_load_texture(e, e->assets, NULL, false);
		if (e->gl.default_texture == A3D_ASSET_INVALID_HANDLE)
		{
			A3D_LOG_ERROR("failed to create fallback checker texture");
			a3d_quit(e);
			return false;
		}
		e->gl.default_material =
		    a3d_assets_create_textured_material(e->assets, e->gl.default_shader, e->gl.default_texture, white_tint);
		if (e->gl.default_material == A3D_ASSET_INVALID_HANDLE)
		{
			A3D_LOG_ERROR("failed to create default fallback material");
			a3d_quit(e);
			return false;
		}
	}
#endif
	/* TODO vulkan */
	return true;
}

void a3d_quit(a3d* e)
{
	if (e->assets)
	{
		a3d_assets_shutdown(e, e->assets);
		free(e->assets);
		e->assets = NULL;
	}

	if (e->renderer)
	{
		a3d_renderer_shutdown(e->renderer);
		free(e->renderer);
		e->renderer = NULL;
	}

	if (e->gfx.v && e->gfx.v->shutdown)
		e->gfx.v->shutdown(e);

	if (e->window)
	{
		SDL_DestroyWindow(e->window);
		e->window = NULL;
	}
	e->running = false;
	SDL_Quit();
}

void a3d_set_clear_colour(a3d* e, float r, float g, float b, float a)
{
	if (e && e->gfx.v && e->gfx.v->set_clear_colour)
		e->gfx.v->set_clear_colour(e, r, g, b, a);
}

bool a3d_submit_mesh_material_handle(a3d* e, a3d_mesh_handle mesh, a3d_material_handle material, const a3d_mvp* mvp)
{
	if (!e || !e->renderer || !e->assets || !mvp)
		return false;
	if (!a3d_assets_get_mesh(e->assets, mesh))
	{
		A3D_LOG_WARN("submit skipped: invalid mesh handle=%u", mesh);
		return false;
	}

	/* get material to shader, texture (fallback if invalid) */
	a3d_material_handle resolved_material = A3D_ASSET_INVALID_HANDLE;
	a3d_shader_handle sort_shader = A3D_ASSET_INVALID_HANDLE;
	a3d_texture_handle sort_texture = A3D_ASSET_INVALID_HANDLE;
	a3d_resolve_material_keys(e, material, &resolved_material, &sort_shader, &sort_texture);

	return a3d_renderer_draw_mesh_material(e->renderer, mesh, mvp, resolved_material, sort_shader, sort_texture);
}

bool a3d_submit_mesh_handle(a3d* e, a3d_mesh_handle mesh, const a3d_mvp* mvp)
{
	return a3d_submit_mesh_material_handle(e, mesh, A3D_ASSET_INVALID_HANDLE, mvp);
}

bool a3d_draw_instanced(
    a3d* e, a3d_mesh_handle mesh, a3d_material_handle material, const a3d_mvp* instances, Uint32 instance_count
)
{
	if (!e || !e->renderer || !e->assets || !instances || instance_count == 0)
		return false;
	if (!a3d_assets_get_mesh(e->assets, mesh))
	{
		A3D_LOG_WARN("instanced submit skipped: invalid mesh handle=%u", mesh);
		return false;
	}

	a3d_material_handle resolved_material = A3D_ASSET_INVALID_HANDLE;
	a3d_shader_handle sort_shader = A3D_ASSET_INVALID_HANDLE;
	a3d_texture_handle sort_texture = A3D_ASSET_INVALID_HANDLE;
	a3d_resolve_material_keys(e, material, &resolved_material, &sort_shader, &sort_texture);

	return a3d_renderer_draw_mesh_material_instanced(
	    e->renderer, mesh, resolved_material, sort_shader, sort_texture, instances, instance_count
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
