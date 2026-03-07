#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "a3d.h"
#include "a3d_camera.h"
#include "a3d_flycam.h"
#define A3D_LOG_TAG "TEST"
#include "a3d_logging.h"
#include "a3d_map.h"

typedef struct a3d_test_prog
{
	a3d_backend backend;
	a3d_map map;
	a3d_camera camera;
	float aspect;
	int prev_w;
	int prev_h;
} a3d_test_prog;

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
	a3d_map_reload_if_changed(engine, &app->map);
	a3d_map_submit(engine, &app->map, app->camera.view, app->camera.proj);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	a3d_backend backend = A3D_BACKEND;
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
	if (!a3d_map_load(&engine, &app.map, "assets/scene.a3dmap"))
	{
		A3D_LOG_ERROR("failed to load assets/scene.a3dmap");
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
	a3d_map_clear(&engine, &app.map);
	a3d_quit(&engine);
	return EXIT_SUCCESS;
}
