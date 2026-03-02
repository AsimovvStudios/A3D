#include <stdbool.h>
#include <stdlib.h>

#include <cglm/cglm.h>
#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_camera.h"
#include "a3d_event.h"
#define A3D_LOG_TAG "TEST"
#include "a3d_logging.h"
#include "a3d_map.h"

static void on_key_down(a3d* engine, const SDL_Event* ev);
static void update_camera(a3d* engine, a3d_camera* cam);

static void on_key_down(a3d* engine, const SDL_Event* ev)
{
	if (!engine || !ev)
		return;

	if (ev->key.key == SDLK_ESCAPE)
		engine->running = false;
}

static void update_camera(a3d* engine, a3d_camera* cam)
{
	const bool* keys = SDL_GetKeyboardState(NULL);
	float dt = a3d_dt(engine);
	float speed = 3.5f;
	float move = speed * dt;
	float forward = 0.0f;
	float right = 0.0f;
	float up = 0.0f;

	if (keys[SDL_SCANCODE_W])
		forward += move;
	if (keys[SDL_SCANCODE_S])
		forward -= move;
	if (keys[SDL_SCANCODE_D])
		right += move;
	if (keys[SDL_SCANCODE_A])
		right -= move;
	if (keys[SDL_SCANCODE_E])
		up += move;
	if (keys[SDL_SCANCODE_Q])
		up -= move;

	a3d_camera_move_local(cam, forward, right, up);

	float look = 1.8f * dt;
	if (keys[SDL_SCANCODE_LEFT])
		cam->yaw -= look;
	if (keys[SDL_SCANCODE_RIGHT])
		cam->yaw += look;
	if (keys[SDL_SCANCODE_UP])
		cam->pitch += look;
	if (keys[SDL_SCANCODE_DOWN])
		cam->pitch -= look;

	if (cam->pitch > 1.55f)
		cam->pitch = 1.55f;
	if (cam->pitch < -1.55f)
		cam->pitch = -1.55f;

	a3d_camera_rebuild_view(cam);
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	a3d_backend backend = A3D_BACKEND;
	a3d engine;
	if (!a3d_init_backend(&engine, backend, "a3d test", 1024, 768)) {
		A3D_LOG_ERROR("engine initialisation failed");
		return EXIT_FAILURE;
	}

	a3d_event_add_handler(&engine, SDL_EVENT_KEY_DOWN, on_key_down);

	a3d_map map;
	a3d_map_init(&map);
	if (!a3d_map_load(&engine, &map, "assets/scene.a3dmap")) {
		A3D_LOG_ERROR("failed to load assets/scene.a3dmap");
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	a3d_camera camera;
	a3d_camera_init(&camera);

	int width = 0;
	int height = 0;
	SDL_GetWindowSizeInPixels(engine.window, &width, &height);
	float aspect = (width > 0 && height > 0) ? (float)width / (float)height : 16.0f / 9.0f;
	a3d_camera_set_perspective(&camera, aspect, backend);
	a3d_camera_rebuild_view(&camera);

	int prev_w = width;
	int prev_h = height;

	while (engine.running) {
		update_camera(&engine, &camera);

		a3d_map_reload_if_changed(&engine, &map);

		a3d_frame_begin(&engine);
		a3d_map_submit(&engine, &map, camera.view, camera.proj);
		a3d_frame_end(&engine);
		a3d_frame(&engine);

		int nw = 0;
		int nh = 0;
		SDL_GetWindowSizeInPixels(engine.window, &nw, &nh);
		if (nw != prev_w || nh != prev_h) {
			prev_w = nw;
			prev_h = nh;
			aspect = (nw > 0 && nh > 0) ? (float)nw / (float)nh : aspect;
			a3d_camera_set_perspective(&camera, aspect, backend);
		}

		SDL_Delay(1);
	}

	a3d_wait_idle(&engine);
	a3d_map_clear(&engine, &map);
	a3d_quit(&engine);
	return EXIT_SUCCESS;
}
