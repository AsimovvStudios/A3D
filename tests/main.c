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

#ifndef USE_OPENGL
#define DEFAULT_BACKEND A3D_BACKEND_VULKAN
#else
#define DEFAULT_BACKEND A3D_BACKEND_OPENGL
#endif

static void create_projection(mat4 proj, float fov_deg, float aspect, float near, float far, a3d_backend backend);
static void on_key_down(a3d* engine, const SDL_Event* ev);

static void on_key_down(a3d* engine, const SDL_Event* ev)
{
	(void)engine;
	A3D_LOG_INFO("key pressed: %s", SDL_GetKeyName(ev->key.key));
}

static void create_projection(mat4 proj, float fov_deg, float aspect, float near, float far, a3d_backend backend)
{
	glm_mat4_identity(proj);

	if (backend == A3D_BACKEND_VULKAN) {
		float fov = glm_rad(fov_deg);
		float f = 1.0f / tanf(fov / 2.0f);

		proj[0][0] = f / aspect;
		proj[1][1] = f;
		proj[2][2] = far / (near - far);
		proj[2][3] = -1.0f;
		proj[3][2] = (near * far) / (near - far);
		proj[3][3] = 0.0f;

		/* flip y */
		proj[1][1] *= -1.0f;
	}
	else {
		glm_perspective(glm_rad(fov_deg), aspect, near, far, proj);
	}
}

int main(int argc, char** argv)
{
	a3d_backend backend = DEFAULT_BACKEND;

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

	/* create a test triangle mesh owned by the app */
	a3d_mesh triangle;
	if (!a3d_init_triangle(&engine, &triangle)) {
		A3D_LOG_ERROR("failed to create triangle mesh");
		a3d_quit(&engine);
		return EXIT_FAILURE;
	}

	/* camera */
	a3d_mvp mvp;
	int w;
	int h;
	SDL_GetWindowSize(engine.window, &w, &h);

	glm_mat4_identity(mvp.model);
	glm_mat4_identity(mvp.view);
	glm_mat4_identity(mvp.proj);

	create_projection(mvp.proj, 70.0f, (float)w/(float)h, 0.1f, 100.0f, backend);

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

		/* animate clear colour */
		t += 0.016f;
		float r = 0.5f * sinf(t) + 0.5f;
		a3d_set_clear_colour(&engine, r, 0.0f, 0.4f, 1.0f);

		float x = sinf(t) * 2.0f;

		glm_mat4_identity(mvp.model);
		glm_translate(mvp.model, (vec3){x, pow(x, 3)-0.0f, -5.0f});

		/* build render queue for this frame: two triangles at different Z to test depth */
		a3d_frame_begin(&engine);

		/* closer triangle (z = -4.2) */
		a3d_mvp mvp_close = mvp;
		glm_translate(mvp_close.model, (vec3){0.0f, 0.0f, 0.8f}); /* -5.0 + 0.8 = -4.2 */
		glm_rotate(mvp_close.model, t, (vec3){0.0f, 0.0f, 1.0f});
		a3d_submit_mesh(&engine, &triangle, &mvp_close);

		/* farther triangle (z = -5.6) */
		a3d_mvp mvp_far = mvp;
		glm_translate(mvp_far.model, (vec3){0.0f, 0.0f, -0.6f}); /* -5.0 - 0.6 = -5.6 */
		glm_rotate(mvp_far.model, t, (vec3){0.0f, 0.0f, 1.0f});
		a3d_submit_mesh(&engine, &triangle, &mvp_far);

		a3d_frame_end(&engine);

		a3d_frame(&engine);
		SDL_Delay(16);
	}

	a3d_wait_idle(&engine);

	/* cleanup in one place */
	a3d_destroy_mesh(&engine, &triangle);
	a3d_quit(&engine);

	return EXIT_SUCCESS;
}
