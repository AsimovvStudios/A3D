#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "a3d.h"
#include "a3d_camera.h"
#include "a3d_flycam.h"
#define A3D_LOG_TAG "TEST"
#include "a3d_logging.h"
#include "a3d_map.h"
#include "a3d_scene.h"

typedef struct a3d_test_prog
{
	a3d_backend backend;
	a3d_map map;
	a3d_scene scene;
	a3d_camera camera;
	float aspect;
	int prev_w;
	int prev_h;
	Uint64 scene_source_mtime_ns;
} a3d_test_prog;

static bool a3d_test_scene_handle_selfcheck(void)
{
	a3d_scene scene;
	a3d_scene_init(&scene);

	a3d_scene_spawn_desc desc = {
	    .mesh = 1u,
	    .material = A3D_ASSET_INVALID_HANDLE,
	    .position = {0.0f, 0.0f, 0.0f},
	    .rotation_deg = {0.0f, 0.0f, 0.0f},
	    .scale = {1.0f, 1.0f, 1.0f},
	};

	a3d_entity first = a3d_scene_spawn(&scene, &desc);
	if (first == A3D_ENTITY_INVALID)
		return false;

	if (!a3d_scene_destroy(&scene, first))
		return false;

	if (a3d_scene_destroy(&scene, first))
		return false;

	a3d_entity second = a3d_scene_spawn(&scene, &desc);
	if (second == A3D_ENTITY_INVALID || second == first)
		return false;

	return true;
}

static bool a3d_test_scene_rebuild_from_map(a3d_test_prog* app)
{
	if (!app)
		return false;

	if (!a3d_map_scene_build(&app->map, &app->scene))
		return false;

	Uint32 valid_mesh_count = 0;
	for (Uint32 i = 0; i < app->map.entity_count; i++)
	{
		if (app->map.entities[i].mesh != A3D_ASSET_INVALID_HANDLE)
			valid_mesh_count++;
	}

	if (app->scene.live_count != valid_mesh_count)
	{
		A3D_LOG_ERROR("scene/map mismatch: scene=%u map=%u", app->scene.live_count, valid_mesh_count);
		return false;
	}

	app->scene_source_mtime_ns = app->map.source_mtime_ns;
	return true;
}

static void a3d_test_update_projection(a3d* engine, a3d_test_prog* app)
{
	if (!engine || !app)
		return;

	int width = 0;
	int height = 0;
	a3d_get_window_size(engine, &width, &height);
	if (width <= 0 || height <= 0)
		return;

	if (width == app->prev_w && height == app->prev_h)
		return;

	app->prev_w = width;
	app->prev_h = height;
	app->aspect = (float)width / (float)height;
	a3d_camera_set_perspective(&app->camera, app->aspect, app->backend);
}

static void a3d_test_tick(a3d* engine, void* user)
{
	if (!engine || !user)
		return;

	a3d_test_prog* app = user;

	if (a3d_key_pressed(&engine->input, A3D_KEY(ESCAPE)))
	{
		engine->running = false;
		return;
	}

	if (a3d_key_pressed(&engine->input, A3D_KEY(TAB)))
	{
		bool lock_mouse = !a3d_is_mouse_locked(engine);
		(void)a3d_set_mouse_locked(engine, lock_mouse);
	}

	a3d_flycam_update(&app->camera, &engine->input, a3d_dt(engine));
	a3d_test_update_projection(engine, app);

	if (a3d_map_reload_if_changed(engine, &app->map) && app->scene_source_mtime_ns != app->map.source_mtime_ns)
	{
		if (!a3d_test_scene_rebuild_from_map(app))
		{
			engine->running = false;
			return;
		}
	}

	a3d_scene_submit(engine, &app->scene, app->camera.view, app->camera.proj);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	a3d_backend backend = A3D_BACKEND;
	if (!a3d_test_scene_handle_selfcheck())
	{
		A3D_LOG_ERROR("scene handle self-check failed");
		return EXIT_FAILURE;
	}

	a3d engine;
	if (!a3d_init_backend(&engine, backend, "a3d test", 1024, 768))
	{
		A3D_LOG_ERROR("engine initialisation failed");
		return EXIT_FAILURE;
	}

	a3d_test_prog app;
	memset(&app, 0, sizeof(app));
	app.backend = backend;
	a3d_map_init(&app.map);
	a3d_scene_init(&app.scene);
	if (!a3d_map_load(&engine, &app.map, "assets/scene.a3dmap"))
	{
		A3D_LOG_ERROR("failed to load assets/scene.a3dmap");
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	if (!a3d_test_scene_rebuild_from_map(&app))
	{
		A3D_LOG_ERROR("failed to build scene runtime from map");
		a3d_map_clear(&engine, &app.map);
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	a3d_camera_init(&app.camera);

	int width = 0;
	int height = 0;
	a3d_get_window_size(&engine, &width, &height);
	app.aspect = (width > 0 && height > 0) ? (float)width / (float)height : 16.0f / 9.0f;
	app.prev_w = width;
	app.prev_h = height;
	a3d_camera_set_perspective(&app.camera, app.aspect, backend);
	a3d_camera_rebuild_view(&app.camera);

	(void)a3d_set_mouse_locked(&engine, true);

	a3d_run(&engine, a3d_test_tick, &app);

	if (a3d_is_mouse_locked(&engine))
		(void)a3d_set_mouse_locked(&engine, false);

	a3d_wait_idle(&engine);
	a3d_scene_clear(&app.scene);
	a3d_map_clear(&engine, &app.map);
	a3d_quit(&engine);
	return EXIT_SUCCESS;
}
