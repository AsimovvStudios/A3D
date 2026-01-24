#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <cglm/mat4.h>
#include <SDL3/SDL.h>

#include "a3d.h"
#include "a3d_event.h"
#define A3D_LOG_TAG "CORE"
#include "a3d_logging.h"
#include "a3d_mesh.h"
#include "a3d_renderer.h"
#include "a3d_camera.h"

#ifndef USE_OPENGL
#define DEFAULT_BACKEND A3D_BACKEND_VULKAN
#else
#define DEFAULT_BACKEND A3D_BACKEND_OPENGL
#endif

static void on_key_down(a3d* engine, const SDL_Event* ev);

static void on_key_down(a3d* engine, const SDL_Event* ev)
{
	(void)engine;
	A3D_LOG_INFO("key pressed: %s", SDL_GetKeyName(ev->key.key));
}


int main(int argc, char** argv)
{
	a3d_backend backend = DEFAULT_BACKEND;

	int prev_w = 0;
	int prev_h = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--vulkan") == 0 || strcmp(argv[i], "-v") == 0) {
			backend = A3D_BACKEND_VULKAN;
		}
		else if (strcmp(argv[i], "--opengl") == 0 || strcmp(argv[i], "-g") == 0) {
			backend = A3D_BACKEND_OPENGL;
		}
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			A3D_LOG_INFO("Usage: %s [--vulkan|-v] [--opengl|-g]", argv[0]);
			A3D_LOG_INFO("\t--vulkan, -v  use Vulkan backend");
			A3D_LOG_INFO("\t--opengl, -g  use OpenGL backend");
			return EXIT_SUCCESS;
		}
	}

	A3D_LOG_INFO("selected backend: %s", backend == A3D_BACKEND_VULKAN ? "Vulkan" : "OpenGL");

	a3d engine;
	if (!a3d_init_backend(&engine, backend, "Asimotive3D Test", 800, 600)) {
		A3D_LOG_ERROR("engine initialisation failed");
		return EXIT_FAILURE;
	}

	a3d_event_add_handler(&engine, SDL_EVENT_KEY_DOWN, on_key_down);

	/* create a cube mesh */
	a3d_mesh cube;

	a3d_vertex cube_verts[] = {
		/* +x face */
		{{1, -1, -1}, {1,0,0}},
		{{1, -1,  1}, {1,0,0}},
		{{1,  1,  1}, {1,0,0}},
		{{1,  1, -1}, {1,0,0}},
		/* -x face */
		{{-1, -1,  1}, {0,1,0}},
		{{-1, -1, -1}, {0,1,0}},
		{{-1,  1, -1}, {0,1,0}},
		{{-1,  1,  1}, {0,1,0}},
		/* +y face */
		{{-1, 1, -1}, {0,0,1}},
		{{1, 1, -1}, {0,0,1}},
		{{1, 1,  1}, {0,0,1}},
		{{-1, 1,  1}, {0,0,1}},
		/* -y face */
		{{-1, -1,  1}, {1,1,0}},
		{{1, -1,  1}, {1,1,0}},
		{{1, -1, -1}, {1,1,0}},
		{{-1, -1, -1}, {1,1,0}},
		/* +z face */
		{{-1, -1, 1}, {1,0,1}},
		{{-1,  1, 1}, {1,0,1}},
		{{1,  1, 1}, {1,0,1}},
		{{1, -1, 1}, {1,0,1}},
		/* -z face */
		{{1, -1, -1}, {0,1,1}},
		{{1,  1, -1}, {0,1,1}},
		{{-1,  1, -1}, {0,1,1}},
		{{-1, -1, -1}, {0,1,1}},
	};

	Uint16 cube_idx[] = {
		0,2,1,  0,3,2,
		4,6,5,  4,7,6,
		8,10,9,  8,11,10,
		12,14,13, 12,15,14,
		16,18,17, 16,19,18,
		20,22,21, 20,23,22
	};

	if (!a3d_mesh_upload(&engine, &cube, cube_verts, 24, cube_idx, 36, A3D_TOPO_TRIANGLES)) {
		A3D_LOG_ERROR("failed to upload cube mesh");
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	/* camera and mvp */
	a3d_camera cam;
	a3d_camera_init(&cam);

	a3d_mvp mvp;
	int w;
	int h;
	SDL_GetWindowSize(engine.window, &w, &h);
	glm_mat4_identity(mvp.model);
	glm_mat4_identity(mvp.view);
	glm_mat4_identity(mvp.proj);

	a3d_camera_set_perspective(&cam, (float)w/(float)h, backend);
	a3d_camera_rebuild_view(&cam);

	float t = 0.0f;

	A3D_LOG();
	A3D_LOG("TEST LOG");
	A3D_LOG_INFO("TEST INFO");
	A3D_LOG_DEBUG("TEST DEBUG");
	A3D_LOG_ERROR("TEST ERROR");

	A3D_LOG();
#if A3D_VK_VALIDATION
	A3D_LOG_INFO("CREATING DEBUG");
#else
	A3D_LOG_ERROR("NO DEBUG");
#endif
	A3D_LOG();

	for (;;) {
		if (!engine.running)
			break;

		t += a3d_dt(&engine);
		float r = 0.5f * sinf(t) + 0.5f;
		a3d_set_clear_colour(&engine, r, 0.0f, 0.4f, 1.0f);

		/* camera controls */
		const bool* keys = SDL_GetKeyboardState(NULL);
		float speed = 3.0f;
		float move = speed * a3d_dt(&engine);
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

		a3d_camera_move_local(&cam, forward, right, up);

		float look = 1.5f * a3d_dt(&engine);
		if (keys[SDL_SCANCODE_LEFT])
			cam.yaw -= look;
		if (keys[SDL_SCANCODE_RIGHT])
			cam.yaw += look;
		if (keys[SDL_SCANCODE_UP])
			cam.pitch += look;
		if (keys[SDL_SCANCODE_DOWN])
			cam.pitch -= look;
		/* clamp pitch */
		if (cam.pitch > 1.55f)
			cam.pitch = 1.55f;
		if (cam.pitch < -1.55f)
			cam.pitch = -1.55f;

		a3d_camera_rebuild_view(&cam);

		/* model transform */
		glm_mat4_identity(mvp.model);
		glm_translate(mvp.model, (vec3){0.0f, 0.0f, 0.0f});
		glm_rotate(mvp.model, t, (vec3){0.0f, 1.0f, 0.0f});

		/* set view/proj from camera */
		glm_mat4_copy(cam.view, mvp.view);
		glm_mat4_copy(cam.proj, mvp.proj);

		/* submit cube */
		a3d_frame_begin(&engine);
		a3d_submit_mesh(&engine, &cube, &mvp);
		a3d_frame_end(&engine);

		a3d_frame(&engine);

		/* handle resize */
		int nw;
		int nh;
		SDL_GetWindowSizeInPixels(engine.window, &nw, &nh);
		if (nw != prev_w || nh != prev_h) {
			prev_w = nw;
			prev_h = nh;
			float aspect = (nw && nh) ? (float)nw / (float)nh : 1.0f;
			a3d_camera_set_perspective(&cam, aspect, backend);
		}

		SDL_Delay(1);
	}

	a3d_wait_idle(&engine);

	/* cleanup in one place */
	a3d_destroy_mesh(&engine, &cube);
	a3d_quit(&engine);

	return EXIT_SUCCESS;
}
